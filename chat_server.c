/*
 * Copyright 2014
 *
 * Author: 		Dinux
 * Description:		Simple chatroom in C using SSL
 * Version:		1.0
 *
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define MAX_CLIENTS	100
#define TCP_PORT	5000

#define ERROR(s) { \
    fprintf(stderr, \
        "%s\n", s); \
        exit(1); \
    }

static unsigned int cli_count = 0;
static int uid = 10;

/* Client structure */
typedef struct {
	struct sockaddr_in addr;	/* Client remote address */
	SSL *ssl;			/* SSL Connection file descriptor */
	int uid;			/* Client unique identifier */
	char name[32];			/* Client name */
} client_t;

client_t *clients[MAX_CLIENTS];

/* Add client to queue */
void queue_add(client_t *cl){
	int i;
	for(i=0;i<MAX_CLIENTS;i++)
		if(!clients[i]){
			clients[i] = cl;
			return;
		}
}

/* Delete client from queue */
void queue_delete(int uid){
	int i;
	for(i=0;i<MAX_CLIENTS;i++)
		if(clients[i])
			if(clients[i]->uid == uid){
				clients[i] = NULL;
				return;
			}
}

/* Send message to all clients but the sender */
void send_message(char *s, int uid){
	int i;
	for(i=0;i<MAX_CLIENTS;i++)
		if(clients[i])
			if(clients[i]->uid != uid)
				SSL_write(clients[i]->ssl, s, strlen(s));
}

/* Send message to all clients */
void send_message_all(char *s){
	int i;
	for(i=0;i<MAX_CLIENTS;i++)
		if(clients[i])
			SSL_write(clients[i]->ssl, s, strlen(s));
}

/* Send message to client */
void send_message_client(char *s, int uid){
	int i;
	for(i=0;i<MAX_CLIENTS;i++)
		if(clients[i])
			if(clients[i]->uid == uid)
				SSL_write(clients[i]->ssl, s, strlen(s));
}

/* Send list of active clients */
void send_active_clients(int uid){
	int i;
	char s[64];
	for(i=0;i<MAX_CLIENTS;i++){
		if(clients[i]){
			sprintf(s, "<<CLIENT %d | %s\r\n", clients[i]->uid, clients[i]->name);
			send_message_client(s, uid);
		}
	}
}

/* Strip CRLF */
void strip_newline(char *s){
	while(*s != '\0'){
		if(*s == '\r' || *s == '\n')
			*s = '\0';
		s++;
	}
}

void strtolower(char *s){
   while(*s != '\0'){
      *s = tolower(*s);
      s++;
   }
}

/* Print ip address */
void print_client_addr(struct sockaddr_in addr){
	printf("%d.%d.%d.%d",
		addr.sin_addr.s_addr & 0xFF,
		(addr.sin_addr.s_addr & 0xFF00)>>8,
		(addr.sin_addr.s_addr & 0xFF0000)>>16,
		(addr.sin_addr.s_addr & 0xFF000000)>>24);
}

/* Handle all communication with the client */
void *handle_client(void *arg){
	char buff_out[1024];
	char buff_in[1024];
	int rlen;

	cli_count++;
	client_t *cli = (client_t *)arg;

	if(SSL_accept(cli->ssl) == -1)
		ERROR("Client connection faild"); //ERR_print_errors_fp(stderr);

	printf("<<ACCEPT ");
	print_client_addr(cli->addr);
	printf(" REFERENCED BY %d\n", cli->uid);

	sprintf(buff_out, "<<JOIN, HELLO %s\r\n", cli->name);
	send_message_all(buff_out);

	send_message_client("Type \\HELP for a list of commands\r\n", cli->uid);

	while((rlen = SSL_read(cli->ssl, buff_in, sizeof(buff_in)-1)) > 0){
	        buff_in[rlen] = '\0';
	        buff_out[0] = '\0';
		strip_newline(buff_in);

		/* Ignore empty buffer */
		if(!strlen(buff_in)){
			continue;
		}
	
		/* Special options */
		if(buff_in[0] == '\\'){
			char *command, *param;
			command = strtok(buff_in," ");
			command++;
			strtolower(command);
			if(!strcmp(command, "quit")){
				break;
			}else if(!strcmp(command, "ping")){
				send_message_client("<<PONG\r\n", cli->uid);
			}else if(!strcmp(command, "name")){
				param = strtok(NULL, " ");
				if(param){
					char *old_name = strdup(cli->name);
					strcpy(cli->name, param);
					sprintf(buff_out, "<<RENAME, %s TO %s\r\n", old_name, cli->name);
					free(old_name);
					send_message_all(buff_out);
				}else{
					send_message_client("<<NAME CANNOT BE NULL\r\n", cli->uid);
				}
			}else if(!strcmp(command, "private")){
				param = strtok(NULL, " ");
				if(param){
					int uid = atoi(param);
					param = strtok(NULL, " ");
					if(param){
						sprintf(buff_out, "[PM][%s]", cli->name);
						while(param != NULL){
							strcat(buff_out, " ");
							strcat(buff_out, param);
							param = strtok(NULL, " ");
						}
						strcat(buff_out, "\r\n");
						send_message_client(buff_out, uid);
					}else{
						send_message_client("<<MESSAGE CANNOT BE NULL\r\n", cli->uid);
					}
				}else{
					send_message_client("<<REFERENCE CANNOT BE NULL\r\n", cli->uid);
				}
			}else if(!strcmp(command, "active")){
				sprintf(buff_out, "<<CLIENTS %d\r\n", cli_count);
				send_message_client(buff_out, cli->uid);
				send_active_clients(cli->uid);
			}else if(!strcmp(command, "help")){
				strcat(buff_out, "\\QUIT     Quit chatroom\r\n");
				strcat(buff_out, "\\PING     Server test\r\n");
				strcat(buff_out, "\\NAME     <name> Change nickname\r\n");
				strcat(buff_out, "\\PRIVATE  <reference> <message> Send private message\r\n");
				strcat(buff_out, "\\ACTIVE   Show active clients\r\n");
				strcat(buff_out, "\\HELP     Show help\r\n");
				send_message_client(buff_out, cli->uid);
			}else{
				send_message_client("<<UNKOWN COMMAND\r\n", cli->uid);
			}
		}else{
			/* Send message */
			sprintf(buff_out, "[%s] %s\r\n", cli->name, buff_in);
			send_message(buff_out, cli->uid);
		}
	}

	/* Close connection */
	sprintf(buff_out, "<<LEAVE, BYE %s\r\n", cli->name);
	send_message_all(buff_out);
	int sd = SSL_get_fd(cli->ssl);
	SSL_free(cli->ssl);
	close(sd);

	/* Delete client from queue and yeild thread */
	queue_delete(cli->uid);
	printf("<<LEAVE ");
	print_client_addr(cli->addr);
	printf(" REFERENCED BY %d\n", cli->uid);
	free(cli);
	cli_count--;
	pthread_detach(pthread_self());
	
	return 0;
}

/* Load the SSL certificate and private key */
void load_certificate(SSL_CTX *ctx, const char *cert, const char *key){
	if(SSL_CTX_use_certificate_file(ctx, cert, SSL_FILETYPE_PEM) <= 0)
		ERROR("Certificate is invalid");

	if(SSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM) <= 0 )
		 ERROR("Private key is invalid");

	if(!SSL_CTX_check_private_key(ctx))
		ERROR("Key pair does not match");
}

int main(int argc, char *argv[]){
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	pthread_t tid;
	SSL_CTX *ctx = NULL;
	SSL *ssl = NULL;
	const char cert[] = "cert.pem";
	const char key[] = "key.pem";

	/* Socket settings */
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(TCP_PORT); 

	/* Initialize SSL */
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();
	SSL_library_init();
	ctx = SSL_CTX_new(SSLv3_server_method());
	load_certificate(ctx, cert, key);
	
	/* Bind */
	if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
		perror("Socket binding failed");
		return 1;
	}

	/* Listen */
	if(listen(listenfd, 10) < 0){
		perror("Socket listening failed");
		return 1;
	}

	printf("<[SERVER STARTED]>\n");

	/* Accept clients */
	while(1){
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

		/* Check if max clients is reached */
		if((cli_count+1) == MAX_CLIENTS){
			printf("<<MAX CLIENTS REACHED\n");
			printf("<<REJECT ");
			print_client_addr(cli_addr);
			printf("\n");
			close(connfd);
			continue;
		}

		ssl = SSL_new(ctx);
		SSL_set_fd(ssl, connfd);

		/* Client settings */
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->addr = cli_addr;
		cli->ssl = ssl;
		cli->uid = uid++;
		sprintf(cli->name, "%d", cli->uid);

		/* Add client to the queue and fork thread */
		queue_add(cli);
		pthread_create(&tid, NULL, &handle_client, (void*)cli);

		/* Reduce CPU usage */
		sleep(1);
	}

	SSL_CTX_free(ctx);
	return 0;
}
