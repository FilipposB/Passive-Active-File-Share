#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/in.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <net/if.h>
#include <sys/stat.h>
#include <signal.h>

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
	char port[STRING_MAX_LENGTH];
	char ip[STRING_MAX_LENGTH];
	char type[STRING_MAX_LENGTH];
} server_info;

typedef struct
{
	long unsigned int total_pdf_files;
	long unsigned int total_word_files;
	long unsigned int total_excel_files;
} file_count;

void *client_process(void *);
void init();
int manage_client(connection_t *, int, int);
void *process(void *);
void save_file_count(file_count, char *);
void load_file_count(file_count *, char *);
void intHandler(int);
long unsigned int get_file_count(file_count, int);

char dir_name[STRING_MAX_LENGTH];
char save_dir_path[STRING_MAX_LENGTH];
file_count count_files;
int connected = 0;
int server_type = 0;

int quit_from_server = 0;

int request_next_server_contact = 0;
int next_server_exists = 0;
server_contact next_server;

pthread_mutex_t count_files_mutex;
pthread_mutex_t request_next_mutex;

int main(int argc, char **argv)
{
	int sock = -1;
	struct sockaddr_in address;
	connection_t *connection;
	pthread_t thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	server_info info;

	if (pthread_mutex_init(&count_files_mutex, NULL) != 0)
	{
		printf("\n mutex init has failed\n");
		return 1;
	}

	if (pthread_mutex_init(&request_next_mutex, NULL) != 0)
	{
		printf("\n mutex init has failed\n");
		return 1;
	}

	signal(SIGINT, intHandler);

	struct hostent *host_entry;
	int hostname;
	{
		char hostbuffer[256];
		hostname = gethostname(hostbuffer, sizeof(hostbuffer));

		host_entry = gethostbyname(hostbuffer);
	}

	char *IPbuffer;
	IPbuffer = inet_ntoa(*((struct in_addr *)host_entry->h_addr_list[0]));

	if (argc < 3)
	{
		printf("Not enough arguments ! {PORT} {TYPE}\n");
		return 1;
	}

	sprintf(dir_name, "Server_%s", argv[1]);
	strcpy(save_dir_path, dir_name);

	if (strcmp(argv[2], PDF_TYPE) == 0)
	{
		server_type = 1;
		strcat(save_dir_path, "/pdf");
	}
	else if (strcmp(argv[2], WORD_TYPE) == 0)
	{
		server_type = 2;
		strcat(save_dir_path, "/word");
	}
	else if (strcmp(argv[2], EXCEL_TYPE) == 0)
	{
		server_type = 3;
		strcat(save_dir_path, "/excel");
	}
	else
	{
		printf("Invalid type !");
		return -2;
	}

	//Create folder
	{
		struct stat st = {0};
		if (stat(dir_name, &st) == -1)
		{
			mkdir(dir_name, 0700);
		}

		if (stat(save_dir_path, &st) == -1)
		{
			mkdir(save_dir_path, 0700);
		}
	}

	strcpy(info.port, argv[1]);
	strcpy(info.type, argv[2]);
	strcpy(info.ip, IPbuffer);

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
	address.sin_port = htons(atoi(argv[1]));
	if (bind(sock, (struct sockaddr *)&address, sizeof(struct sockaddr_in)) < 0)
	{
		fprintf(stderr, "%s: error: cannot bind socket to port %s\n", argv[0], argv[1]);
		return -4;
	}

	/* listen on port */
	if (listen(sock, 5) < 0)
	{
		fprintf(stderr, "%s: error: cannot listen on port\n", argv[0]);
		return -5;
	}

	pthread_create(&thread, &attr, client_process, (void *)&info);

	while (connected < 1)
	{
		if (connected == -1)
		{
			return 1;
		}
	}

	printf("%s: ready !\n", argv[0]);

	int thread_count = 0;

	load_file_count(&count_files, dir_name);

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
	pthread_mutex_destroy(&count_files_mutex);
	pthread_mutex_destroy(&request_next_mutex);
	return 0;
}

//Call to SecretaryServer
void *client_process(void *ptr)
{
	server_info *info = (server_info *)ptr;

	char *buffer = (char *)malloc(STRING_MAX_LENGTH * sizeof(char));
	int sock;
	int err;
	if (err = connect_to_server(&sock, HOST, SECRETARY_PORT) < 0)
	{
		connected = -1;
		pthread_exit((void *)1);
	}
	if (sock < 0)
	{
		connected = -1;
		printf("Socket Failed !\n");
		pthread_exit((void *)1);
	}

	connected = 1;

	strcpy(buffer, SERVER_DECLERATION);
	strcat(buffer, "-");
	strcat(buffer, info->type);

	send_message(buffer, sock);

	send_message(info->port, sock);

	send_message(info->ip, sock);

	read_measege(buffer, sock);

	//Stand by call with the secretary
	while (1)
	{
		if (request_next_server_contact == 1)
		{

			send_message(NEXT_CONTACT, sock);
			read_measege(buffer, sock);
			if (strcmp(buffer, CONFIRMATION) == 0)
			{
				read_measege(next_server.address, sock);
				read_measege(next_server.port, sock);
				printf("Port %s\n", next_server.port);

				next_server_exists = 1;
			}
			else
			{
				next_server_exists = 0;
			}

			request_next_server_contact = 2;
		}

		if (quit_from_server == 1)
		{
			break;
		}
	}

	printf("bye\n");
	send_message(HANG_UP, sock);
	/* close socket */
	close(sock);
	quit_from_server = 2;
	pthread_exit(0);
}

void *process(void *ptr)
{
	char *buffer = (char *)malloc(STRING_MAX_LENGTH * sizeof(char));
	char *file_name = (char *)malloc(STRING_MAX_LENGTH * sizeof(char));
	char *extra = (char *)malloc(STRING_MAX_LENGTH * sizeof(char));
	char *quit_message = (char *)malloc(STRING_MAX_LENGTH * sizeof(char));
	int success = 1;
	strcpy(quit_message, HANG_UP);

	connection_t *conn;
	if (!ptr)
		pthread_exit(0);
	conn = (connection_t *)ptr;

	pthread_mutex_lock(&count_files_mutex);

	sprintf(extra, "%lx", get_file_count(count_files, server_type));

	switch (server_type)
	{
	case 1:
		count_files.total_pdf_files++;
		break;
	case 2:
		count_files.total_excel_files++;
		break;
	case 3:
		count_files.total_word_files++;
		break;
	}

	pthread_mutex_unlock(&count_files_mutex);

	read_measege(buffer, conn->sock);

	if (receive_file(save_dir_path, extra, file_name, conn->sock) != 0)
	{
		printf("receive_fail\n");
		success = 0;
	}
	
	if (success == 1)
	{
					

		pthread_mutex_lock(&count_files_mutex);

		save_file_count(count_files, dir_name);

		pthread_mutex_unlock(&count_files_mutex);

		//either reads a hang up or a ICTS_UPLOAD		


		if (strcmp(buffer, ICTS_UPLOAD) == 0)
		{
			//we talk with the secretary to tell us in which server to send the file
			pthread_mutex_lock(&request_next_mutex);
			request_next_server_contact = 1;
			while (request_next_server_contact != 2)
				;
			if (next_server_exists == 1)
			{
				int err;
				int transfer_sock;
				if (err = connect_to_server(&transfer_sock, next_server.address, atoi(next_server.port)) < 0)
				{
					printf("Failed to connect to Server -> Address : %s Port : %s\n", next_server.address, next_server.port);
				}
				else
				{
					printf("Sending %s to Address : %s Port : %s\n", file_name, next_server.address, next_server.port);
					printf("File %s (original name %s)\n", extra, file_name);
					send_message(ICTS_UPLOAD, transfer_sock);
					send_file(file_name, extra , transfer_sock);
				}
				close(transfer_sock);
			}
			request_next_server_contact = 0;
			pthread_mutex_unlock(&request_next_mutex);
		}
	}

	/* close socket and clean up */
	send_message(quit_message, conn->sock);
	printf("Call with %d finished !\n", conn->thread_id);
	close(conn->sock);
	free(buffer);
	free(quit_message);
	free(conn);
	free(extra);
	free(file_name);
	pthread_exit(0);
}

void load_file_count(file_count *count, char *dir)
{
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	int counter = 0;
	char save_file[STRING_MAX_LENGTH] = "";

	sprintf(save_file, "%s/save.bank", dir);

	count->total_excel_files = 0;
	count->total_pdf_files = 0;
	count->total_word_files = 0;

	fp = fopen(save_file, "r");
	if (fp == NULL)
	{
		fp = fopen(save_file, "w");
		fputs("0\n0\n0", fp);
		return;
	}

	while ((read = getline(&line, &len, fp)) != -1 && counter < 3)
	{
		switch (counter)
		{
		case 0:
			count->total_pdf_files = atol(line);
			break;

		case 1:
			count->total_excel_files = atol(line);
			break;

		case 2:
			count->total_word_files = atol(line);
			break;

		default:
			break;
		}
		counter++;
	}

	fclose(fp);
	if (line)
		free(line);
}

void save_file_count(file_count count, char *dir)
{
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	int counter = 0;
	char save_file[STRING_MAX_LENGTH] = "";
	char buffer[STRING_MAX_LENGTH] = "";

	sprintf(save_file, "%s/save.bank", dir);

	fp = fopen(save_file, "w");

	sprintf(buffer, "%ld\n", count.total_pdf_files);
	fputs(buffer, fp);
	sprintf(buffer, "%ld\n", count.total_excel_files);
	fputs(buffer, fp);
	sprintf(buffer, "%ld\n", count.total_word_files);
	fputs(buffer, fp);

	fclose(fp);
	if (line)
		free(line);
}

long unsigned int get_file_count(file_count count, int type)
{
	switch (type)
	{
	case 1:
		return count.total_pdf_files;
	case 2:
		return count.total_excel_files;
	case 3:
		return count.total_word_files;

	default:
		return 0;
	}
}

void intHandler(int dummy)
{
	save_file_count(count_files, dir_name);
	quit_from_server = 1;
	while (quit_from_server == 1)
		;
	//save_file_count(count_files, dir_name);

	exit(0);
}
