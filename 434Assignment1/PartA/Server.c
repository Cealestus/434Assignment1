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

#define MAXDATASIZE 512

typedef struct{
	char *key;
	char *value;
}pair;

pair pairList[10];
int numUsed;
int cont;

void add(char *key, char *value){
	int i = 0;
	for(i = 0; i < 10; i++){
		if(strcmp(pairList[i].key, key) == 0){

		}
		else if(pairList[i].key == NULL){
			pairList[i].key = (char*) malloc(sizeof(key));
			pairList[i].value = (char*) malloc(sizeof(value));
			numUsed++;
		}
	}
}

char *getValue(char *key){
	int i = 0;
	for(i = 0; i < 10; i++){
		if(strcmp(pairList[i].key , key) == 0){
			return pairList[i].value;
		}
	}
	return NULL;
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
    char *messages[MAXDATASIZE];
    char *tok;
    int currentMessages;

    numUsed = 0;
    tok = (char*) malloc(MAXDATASIZE);
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
        		while((tok = strtok(buf, " ")) != NULL){
        			printf("about to malloc\n");
        			messages[currentMessages] = (char*)malloc(sizeof(tok));
        			printf("Malloc'd properly\n");
        			messages[currentMessages] = tok;
        			printf("Adding tok to the messages list\n");
        			printf("%s\n", messages[currentMessages]);
        			currentMessages++;
        			tok = NULL;
        		}
        	}
        	if(strcmp(messages[0], "add") == 0){
        		add(messages[1], messages[2]);
        		printf("Successfully added a key value pair\n");
        	}
        }
    }

    return 0;
}
