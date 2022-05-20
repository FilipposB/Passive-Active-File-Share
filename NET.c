#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include "NET.h"
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

void read_measege(char *buffer, int socket)
{
	clear_buffer(buffer);
	read(socket, buffer, STRING_MAX_LENGTH);
}

void send_message(char *message, int socket)
{
	write(socket, message, STRING_MAX_LENGTH);
}

void send_file(char *file_name, char *actual_file_name, int sockfd)
{
	int filefd = open(actual_file_name, O_RDONLY);
	char buffer[BUFSIZ];

	//send file name
	send_message(file_name, sockfd);

	if (filefd == -1)
	{
		perror("open");
		exit(EXIT_FAILURE);
	}
	while (1)
	{
		ssize_t read_return = read(filefd, buffer, BUFSIZ);
		if (read_return == 0)
			break;
		if (read_return == -1)
		{
			perror("read");
			exit(EXIT_FAILURE);
		}
		if (write(sockfd, buffer, read_return) == -1)
		{
			perror("write");
			exit(EXIT_FAILURE);
		}
	}
	close(filefd);
}

int receive_file(char* path, char* extra, char* filename, int sockfd)
{
	char buf[STRING_MAX_LENGTH] = "";
	char final_path[STRING_MAX_LENGTH] = "";
	//read name
	read_measege(filename, sockfd);
	strcpy(final_path, path);
	strcat(final_path, "/");
	strcat(final_path, extra);
	strcat(final_path, "_");
	strcat(final_path, filename);
// after extra is used we can transfer the name that the file is saved for future use
	strcpy(extra, final_path);

	printf("PATH : %s\n", final_path);
	
	int filefd = open(final_path,O_WRONLY | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR);
	char buffer[BUFSIZ];
	ssize_t read_return;

	if (filefd == -1)
	{
		perror("open");
		return 1;
	}
	do
	{
		read_return = read(sockfd, buffer, BUFSIZ);
		if (read_return == -1)
		{
			perror("read");
			return 2;
		}
		if (write(filefd, buffer, read_return) == -1)
		{
			perror("write");
			return 3;
		}
	} while (read_return > 0);
	close(filefd);
	return 0;
}

int connect_to_server(int *sock, char *address_to_connect, int port)
{
	struct sockaddr_in address;
	struct hostent *host;
	int len;

	/* connect to server */
	address.sin_family = AF_INET;
	address.sin_port = htons(port); /* create socket */
	*sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (*sock <= 0)
	{
		fprintf(stderr, "error: cannot create socket\n");
		return -3;
	}
	host = gethostbyname(address_to_connect);
	if (!host)
	{
		fprintf(stderr, "error: unknown host \n");
		return -4;
	}
	memcpy(&address.sin_addr, host->h_addr_list[0], host->h_length);
	if (connect(*sock, (struct sockaddr *)&address, sizeof(address)))
	{
		fprintf(stderr, "error: cannot connect to host\n");
		return -5;
	}
}

void clear_buffer(char *buffer)
{
	strcpy(buffer, "");
}