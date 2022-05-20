#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/in.h>
#include <unistd.h>

#include "NET.h"

#define MAX_CONNECTED_SERVERS 50

typedef struct
{
	int sock;
	struct sockaddr address;
	int addr_len;

	int thread_id;
} connection_t;

typedef struct
{
	int active;
	connection_t *connection;
	server_contact contact_info;
} server;

server active_servers[3][MAX_CONNECTED_SERVERS];
int active_server_count[3];
pthread_mutex_t active_server_lock;

void *process(void *ptr);
void init();
int manage_client(connection_t *, int, int);
void empty_server(server *server_to_clear);
int manage_server(connection_t *, int);
int next_available_server(int, int);

int main(int argc, char **argv)
{
	int sock = -1;
	struct sockaddr_in address;
	connection_t *connection;
	pthread_t thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	if (pthread_mutex_init(&active_server_lock, NULL) != 0)
	{
		printf("\n mutex init has failed\n");
		return 1;
	}

	/* create socket */
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock <= 0)
	{
		fprintf(stderr, "%s: error: cannot create socket\n", argv[0]);
		return -3;
	}
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

	/* bind socket to port */
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(SECRETARY_PORT);
	if (bind(sock, (struct sockaddr *)&address, sizeof(struct sockaddr_in)) < 0)
	{
		fprintf(stderr, "%s: error: cannot bind socket to port %d\n", argv[0], SECRETARY_PORT);
		return -4;
	}

	/* listen on port */
	if (listen(sock, 5) < 0)
	{
		fprintf(stderr, "%s: error: cannot listen on port\n", argv[0]);
		return -5;
	}

	printf("%s: ready and listening\n", argv[0]);

	int thread_count = 0;

	while (1)
	{
		/* accept incoming connections */
		connection = (connection_t *)malloc(sizeof(connection_t));
		connection->sock = accept(sock, &connection->address, &connection->addr_len);
		connection->thread_id = thread_count++;
		if (connection->sock <= 0)
		{
			free(connection);
		}
		else
		{
			/* start a new thread but do not wait for it */
			pthread_create(&thread, &attr, process, (void *)connection);
		}
	}
	pthread_attr_destroy(&attr);
	pthread_mutex_destroy(&active_server_lock);
	return 0;
}

void init()
{
	int i;
	int j;

	active_server_count[0] = 0;
	active_server_count[1] = 0;
	active_server_count[2] = 0;

	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < MAX_CONNECTED_SERVERS; j++)
		{
			empty_server(&active_servers[i][j]);
		}
	}
}

void empty_server(server *server_to_clear)
{
	server_to_clear->connection = NULL;
	server_to_clear->active = -1;
	strcpy(server_to_clear->contact_info.address, "");
	strcpy(server_to_clear->contact_info.port, "");
}

void *process(void *ptr)
{
	char *buffer = (char *)malloc(STRING_MAX_LENGTH * sizeof(char));
	char *quit_message = (char *)malloc(STRING_MAX_LENGTH * sizeof(char));
	strcpy(quit_message, HANG_UP);

	connection_t *conn;
	if (!ptr)
		pthread_exit(0);
	conn = (connection_t *)ptr;

	read_measege(buffer, conn->sock);
	//Check if buffer has appropriate text
	if (strlen(buffer) < 1)
	{
		strcpy(quit_message, "Invalid Input !");
	}
	else
	{
		const char s[2] = "-";
		char *token;
		printf("Client said : %s\n", buffer);

		/* get the first token */
		token = strtok(buffer, s);
		if (token != NULL)
		{
			if (strcmp(token, CLIENT_DECLERATION) == 0)
			{
				//Client
				int type = 0;
				int upload_method = 0;
				token = strtok(NULL, s);
				if (token != NULL)
				{
					if (strcmp(token, PDF_TYPE) == 0)
					{
						type = 1;
					}
					else if (strcmp(token, WORD_TYPE) == 0)
					{
						type = 2;
					}
					else if (strcmp(token, EXCEL_TYPE) == 0)
					{
						type = 3;
					}
					else
					{
						strcpy(quit_message, "Type is invalid!");
					}
					if (type > 0 && type < 4)
					{
						token = strtok(NULL, s);
						if (token != NULL)
						{
							if (strcmp(token, DCTS_UPLOAD) == 0)
							{
								upload_method = 1;
							}
							else if (strcmp(token, ICTS_UPLOAD) == 0)
							{
								upload_method = 2;
							}
							else
							{
								strcpy(quit_message, "Upload Method is invalid!");
							}
							if (upload_method > 0 && upload_method < 3)
							{
								//Client support
								int err = manage_client(conn, type, upload_method);
								if (err == 1)
								{
									strcpy(quit_message, "Invalid request!");
								}
								else if (err == 2)
								{
									strcpy(quit_message, "No servers of this file type are online !");
								}
							}
						}
						else
						{
							strcpy(quit_message, "Upload Method is required !");
						}
					}
				}
				else
				{
					strcpy(quit_message, "Type is required !");
				}
			}
			//Server(Client)
			else if (strcmp(token, SERVER_DECLERATION) == 0)
			{
				token = strtok(NULL, s);
				int type = 0;
				if (token != NULL)
				{
					if (strcmp(token, PDF_TYPE) == 0)
					{
						type = 1;
					}
					else if (strcmp(token, WORD_TYPE) == 0)
					{
						type = 2;
					}
					else if (strcmp(token, EXCEL_TYPE) == 0)
					{
						type = 3;
					}
					else
					{
						strcpy(quit_message, "Type is invalid!");
					}
					if (type > 0 && type < 4)
					{
						int err = manage_server(conn, type);
						if (err == 1)
						{
							strcpy(quit_message, "Invalid request!");
						}
						else if (err == 2)
						{
							strcpy(quit_message, SERVER_IS_FULL);
						}
					}
				}
				else
				{
					strcpy(quit_message, "Type is required !");
				}
			}
		}
		else
		{
			strcpy(quit_message, "Error nothing to read !");
		}
	}
	/* close socket and clean up */
	send_message(quit_message, conn->sock);
	printf("Call with %d finished !\n", conn->thread_id);
	close(conn->sock);
	free(buffer);
	free(quit_message);
	free(conn);
	pthread_exit(0);
}

int manage_client(connection_t *connection, int file_type, int upload_type)
{
	if (file_type < 1 || file_type > 3 || upload_type < 1 || upload_type > 2)
	{
		return 1;
	}

	file_type -= 1;

	pthread_mutex_lock(&active_server_lock);
	//No connected servers
	printf("Active %d\n", active_server_count[file_type]);
	if (active_server_count[file_type] == 0)
	{
		pthread_mutex_unlock(&active_server_lock);
		return 2;
	}

	char *buffer = (char *)malloc(STRING_MAX_LENGTH * sizeof(char));

	send_message(CONFIRMATION, connection->sock);

	//Active
	if (upload_type == 1)
	{
		int i = next_available_server(file_type, -1);
		sprintf(buffer, "%d", active_server_count[file_type]);
		send_message(buffer, connection->sock);
		while (i != -1)
		{
			send_message(active_servers[file_type][i].contact_info.address, connection->sock);
			send_message(active_servers[file_type][i].contact_info.port, connection->sock);
			printf("PORT %s\n", active_servers[file_type][i].contact_info.port);
			i = next_available_server(file_type, i);
		}
	}
	//Passive
	else if (upload_type == 2)
	{
		//Send the address of the first server
		int i = next_available_server(file_type, -1);
		send_message(active_servers[file_type][i].contact_info.address, connection->sock);
		send_message(active_servers[file_type][i].contact_info.port, connection->sock);
	}
	pthread_mutex_unlock(&active_server_lock);
	return 0;
}

int manage_server(connection_t *connection, int file_type)
{
	if (file_type < 1 || file_type > 3)
	{
		return 1;
	}

	file_type -= 1;
	pthread_mutex_lock(&active_server_lock);
	//No connected servers
	if (active_server_count[file_type] > MAX_CONNECTED_SERVERS)
	{
		return 2;
	}

	int next_available_spot = 0;
	for (next_available_spot = 0; next_available_spot < MAX_CONNECTED_SERVERS; next_available_spot++)
	{
		if (active_servers[file_type][next_available_spot].active < 1)
		{
			break;
		}
	}
	active_servers[file_type][next_available_spot].connection = connection;
	active_servers[file_type][next_available_spot].active = 1;

	//Read Port
	read_measege(active_servers[file_type][next_available_spot].contact_info.port, connection->sock);
	read_measege(active_servers[file_type][next_available_spot].contact_info.address, connection->sock);

	printf("Port received : %s\n", active_servers[file_type][next_available_spot].contact_info.port);

	active_server_count[file_type]++;
	pthread_mutex_unlock(&active_server_lock);

	char *buffer = (char *)malloc(STRING_MAX_LENGTH * sizeof(char));

	send_message(CONFIRMATION, connection->sock);

	do
	{
		read_measege(buffer, connection->sock);

		if (strcmp(buffer, NEXT_CONTACT) == 0){
			int next = next_available_server(file_type, next_available_spot);
			if (next == -1){
				send_message(DECLINE, connection->sock);
			}
			else
			{
				send_message(CONFIRMATION, connection->sock);
				send_message(active_servers[file_type][next].contact_info.address, connection->sock);
				send_message(active_servers[file_type][next].contact_info.port, connection->sock);
			}
			
		}

		if (strcmp(buffer, HANG_UP) == 0)
		{
			break;
		}

	} while (1);
	active_servers[file_type][next_available_spot].active = 0;
	empty_server(&active_servers[file_type][next_available_spot]);
	active_server_count[file_type]--;
	printf("Server deactivated !\n");
	free(buffer);
	return 0;
}

//Should have locked mutex
int next_available_server(int type, int current)
{
	int i;
	for (i = current + 1; i < MAX_CONNECTED_SERVERS; i++)
	{
		if (active_servers[type][i].active == 1)
		{
			return i;
		}
	}
	return -1;
}
