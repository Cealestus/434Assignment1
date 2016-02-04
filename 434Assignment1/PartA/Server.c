/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "30005"  // the port users will be connecting to

#define BACKLOG 10     // how many pending connections queue will hold

#define MAXDATASIZE 1024

typedef struct{
	char *key;
	char *value;
}pair;

pair pairList[10];
int numUsed;
int cont;

void printPairList(int new_fd){
	char sent[MAXDATASIZE];
	int i;
	for(i = 0; i < 10; i++){
		sprintf(sent, "%s, %s\n", pairList[i].key, pairList[i].value);
		send(new_fd, sent, strlen(sent), 0);
	}
}

void add(char *key, char *value, int new_fd){
	int i = 0;
	if(numUsed < 9){
		for(i = 0; i < 10; i++){
			if(pairList[i].key != NULL && strcmp(pairList[i].key, key) == 0){
				char sent[MAXDATASIZE];
				sprintf(sent, "Key: %s already in the server at position %i, which contains: %s\n", key, i, pairList[i].key);
				send(new_fd, sent, strlen(sent), 0);
				return;
			}
			else if(pairList[i].key == NULL){
				pairList[i].key = (char*)malloc(sizeof(key));
				pairList[i].value = (char*)malloc(sizeof(value));
				pairList[i].key = key;
				pairList[i].value = value;
				numUsed++;
				char sent[MAXDATASIZE];
				sprintf(sent, "Key: %s, Value: %s, added to the server, currently have %i values in the server\n", pairList[i].key, pairList[i].value, numUsed);
				send(new_fd, sent, strlen(sent), 0);
				return;
			}
		}
	}
	else{
		char sent[MAXDATASIZE];
		sprintf(sent, "Server full\n");
		send(new_fd, sent, strlen(sent), 0);
		return;
	}
}

void getValue(char *key, int new_fd){
	int i = 0;
	if(numUsed > 0){
		for(i = 0; i < numUsed; i++){
			if(strcmp(pairList[i].key , key) == 0){
				char sent[MAXDATASIZE];
				sprintf(sent, "Value for key: %s, is: %s\n", key, pairList[i].value);
				send(new_fd, sent, strlen(sent), 0);
				return;
			}
		}
	}else{
		char sent[MAXDATASIZE];
		sprintf(sent, "Server is currently empty, no data to retrieve\n");
		send(new_fd, sent, strlen(sent), 0);
		return;
	}
	char sent[MAXDATASIZE];
	sprintf(sent, "Value for key: %s, was not found\n", key);
	send(new_fd, sent, strlen(sent), 0);
	return;
}

pair *getAll(){
	return NULL;
}

/*void removePair(char *key){*/
/*}*/

void sigchld_handler()
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    char buf[MAXDATASIZE];
    int numBytes;
    char *messages;
    char *tok[MAXDATASIZE];
    int currentMessages;
    int i;

    numUsed = 0;
    messages = (char*)malloc(MAXDATASIZE);
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        cont = 1;
        while(cont){
        	numBytes = recv(new_fd, buf, MAXDATASIZE, 0);
        	if (numBytes == -1) {
        		perror("recv");
        		exit(1);
        	}
        	else{
        		currentMessages = 0;
        		messages = NULL;
        		messages = strtok(buf, " ");
        		while(messages != NULL){
        			tok[currentMessages] = messages;
        			currentMessages++;
        			messages = strtok(NULL, " ");
        		}
        		tok[0] = strsep(&tok[0],"\n");
        		tok[1] = strsep(&tok[1], "\n");
        		tok[2] = strsep(&tok[2], "\n");
        	}
        	if(strcmp(tok[0], "add") == 0){
        		add(tok[1], tok[2], new_fd);
        		printPairList(new_fd);
        	}
        	else if(strcmp(tok[0], "getvalue") == 0){
        		getValue(tok[1], new_fd);
        		printPairList(new_fd);
        	}
        	else if(strcmp(tok[0], "getall") == 0){
        		printPairList(new_fd);
        	}
        	else if(strcmp(tok[0], "exit") == 0){
        		cont = 0;
        	}
        	for(i = 0; i < MAXDATASIZE; i++){
        		tok[i] = " ";
        	}
        }
        break;
    }

    return 0;
}
