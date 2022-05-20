#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "NET.h"

#define STRING_LENGTH 256

void construct_message(char *, int *);

int main(int argc, char **argv)
{
	int sock;
	int err;
	int upload_method;
	int i;
	char file_name[STRING_MAX_LENGTH];
	int arg_run = 0;
	char *buffer = (char *)malloc(STRING_MAX_LENGTH * sizeof(char));

	if (argc < 2)
	{
		printf("Arguments missing !\n");
		return 1;
	}

	if (argc == 4)
	{
		int type = atoi(argv[2]);
		upload_method = atoi(argv[3]);
		if (upload_method > 0 && upload_method < 3 && type > 0 && type < 4)
		{
			strcpy(buffer, CLIENT_DECLERATION);

			//File type
			strcat(buffer, "-");

			switch (type)
			{
			case 1:
				strcat(buffer, PDF_TYPE);
				break;
			case 2:
				strcat(buffer, WORD_TYPE);
				break;
			case 3:
				strcat(buffer, EXCEL_TYPE);
				break;
			}

			//Upload Type
			strcat(buffer, "-");

			//Write the appropriate file type
			switch (upload_method)
			{
			case 1:
				strcat(buffer, DCTS_UPLOAD);
				break;
			case 2:
				strcat(buffer, ICTS_UPLOAD);
				break;
			}

			arg_run = 1;
		}
	}

	//File to send
	strcpy(file_name, argv[1]);

	//Check if the file exists
	{
		int filefd = open(file_name, O_RDONLY);
		if (filefd == -1)
		{
			perror("Could't open File !\n");
			exit(EXIT_FAILURE);
		}
		close(filefd);
	}

	if (err = connect_to_server(&sock, HOST, SECRETARY_PORT) < 0)
	{
		return err;
	}
	if (sock < 0)
	{
		printf("Socket Failed !");
		return 1;
	}

	if (arg_run == 0)
	{
		construct_message(buffer, &upload_method);
	}

	//Send request
	send_message(buffer, sock);

	//Get answer
	read_measege(buffer, sock);

	if (strcmp(buffer, CONFIRMATION) == 0)
	{
		//Active
		if (upload_method == 1)
		{
			//Read number of servers
			read_measege(buffer, sock);
			int number_of_servers = atoi(buffer);
			if (number_of_servers > 0)
			{
				server_contact server_array[number_of_servers];
				//Catch all server addresses and ports
				for (i = 0; i < number_of_servers; i++)
				{
					read_measege(server_array[i].address, sock);
					read_measege(server_array[i].port, sock);
					printf("Server %d -> Address : %s Port : %s\n", i, server_array[i].address, server_array[i].port);
				}

				//Send file to server
				for (i = 0; i < number_of_servers; i++)
				{
					int transfer_sock;
					if (err = connect_to_server(&transfer_sock, server_array[i].address, atoi(server_array[i].port)) < 0)
					{
						printf("Failed to connect to Server %d -> Address : %s Port : %s\n", i, server_array[i].address, server_array[i].port);
						continue;
					}
					printf("Sending %s to Address : %s Port : %s\n", file_name, server_array[i].address, server_array[i].port);
					send_message(DCTS_UPLOAD, transfer_sock);
					send_file(file_name, file_name, transfer_sock);
				}
			}
		}
		//Passive
		else if (upload_method == 2)
		{
			//Grab information from the first server
			int transfer_sock;
			server_contact server_inf;
			read_measege(server_inf.address, sock);
			read_measege(server_inf.port, sock);
			printf("Server %d -> Address : %s Port : %s\n", i, server_inf.address, server_inf.port);

			if (err = connect_to_server(&transfer_sock, server_inf.address, atoi(server_inf.port)) < 0)
			{
				printf("Failed to connect to Server -> Address : %s Port : %s\n", server_inf.address, server_inf.port);
				send_message(HANG_UP, transfer_sock);
			}
			else
			{
				printf("Sending %s to Address ICTS : %s Port : %s\n", file_name, server_inf.address, server_inf.port);
				send_message(ICTS_UPLOAD, transfer_sock);
				send_file(file_name, file_name, transfer_sock);
			}
			
		}
	}

	/* close socket */
	close(sock);
	free(buffer);
	return 0;
}

void construct_message(char *message, int *upload_method)
{
	int option;

	strcpy(message, CLIENT_DECLERATION);

	//File type
	strcat(message, "-");
	printf("What type is the file you want to move ?");
	do
	{
		printf("\n1) PDF\n2) Word\n3) Excel\n\n>");
		scanf("%d", &option);
	} while (option < 1 || option > 3);
	//Write the appropriate file type
	switch (option)
	{
	case 1:
		strcat(message, PDF_TYPE);
		break;
	case 2:
		strcat(message, WORD_TYPE);
		break;
	case 3:
		strcat(message, EXCEL_TYPE);
		break;
	}

	//Upload Type
	strcat(message, "-");
	do
	{
		printf("\n1) Direct Client To Server(DCTS)\n2) Indirect Client To Server(ICTS)\n\n>");
		scanf("%d", &option);
	} while (option < 1 || option > 2);
	*upload_method = option;
	//Write the appropriate file type
	switch (option)
	{
	case 1:
		strcat(message, DCTS_UPLOAD);
		break;
	case 2:
		strcat(message, ICTS_UPLOAD);
		break;
	}
	//Clear input buffer
	while (getchar() != '\n')
		;
}