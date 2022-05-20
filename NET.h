#ifndef NET
#define NET

#define CLIENT_DECLERATION "client"
#define SERVER_DECLERATION "server"

#define HANG_UP "hangupcall"
#define CONFIRMATION "1confirm1"
#define DECLINE "declinemsg"

#define PDF_TYPE "pdf"
#define WORD_TYPE "word"
#define EXCEL_TYPE "excel"

#define SERVER_IS_FULL "serverfull"

#define NEXT_CONTACT "nextconctact"

//Direct Client To Server(DCTS) the client sends the file to each server
#define DCTS_UPLOAD "direct"
//Indirect Client To Server(ICTS) the clients send the file to one server
//the servers spreads the file to the other servers
#define ICTS_UPLOAD "indirect"

#define HOST "localhost"
#define SECRETARY_PORT 6070

#define STRING_MAX_LENGTH 1024

typedef struct {
    char address[STRING_MAX_LENGTH];
    char port[STRING_MAX_LENGTH];
}server_contact;


void read_measege(char *, int);
void send_message(char *, int);
void clear_buffer(char *);
int connect_to_server(int *, char *, int);
void send_file(char *, char*, int);
int receive_file(char *, char*, char*, int);
#endif 