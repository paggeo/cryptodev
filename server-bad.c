#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include<sys/wait.h>
#include <netdb.h>
#include <string.h>
#include "socket-common.h"

int client_count = 0;
struct client *clients[TCP_BACKLOG] = {NULL};

struct client{
    int id;
    int pid;
};

/* Insist until all of the data has been written */
ssize_t insist_write(int fd, const void *buf, size_t cnt)
{
	ssize_t ret;
	size_t orig_cnt = cnt;
	
	while (cnt > 0) {
	        ret = write(fd, buf, cnt);
	        if (ret < 0)
	                return ret;
	        buf += ret;
	        cnt -= ret;
	}

	return orig_cnt;
}

void handle_sigchld(int sig){
    int s, n, i;
    n = waitpid(-1, &s, WEXITED);

    client_count--;
    for(i = 0; i < TCP_BACKLOG; i++){
        if(clients[i] != NULL && clients[i]->pid == n){
            free(clients[i]);
            clients[i] = NULL;
        }
    }
}

int main(){
    struct sigaction siga;
    siga.sa_handler = &handle_sigchld;
    sigemptyset(&siga.sa_mask);
    siga.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &siga, 0) == -1)
    {
        perror(0);
        exit(1);
    }

    int sd = socket(PF_INET, SOCK_STREAM, 0);
    int newsd;
    socklen_t len;
    int num, pid, i;
    int client_id = 0;

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(TCP_PORT);
    sa.sin_addr.s_addr = htons(INADDR_ANY);
    if (bind(sd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		perror("bind");
		exit(1);
	}
	fprintf(stderr, "Bound TCP socket to port %d\n", TCP_PORT);

    /* Listen for incoming connections */
	if (listen(sd, TCP_BACKLOG) < 0) {
		perror("listen");
		exit(1);
	}

    while(1){
        len = sizeof(struct sockaddr_in);
        if ((newsd = accept(sd, (struct sockaddr *)&sa, &len)) < 0) {
			perror("accept");
			exit(1);
		}
        printf("ACCEPTED\n");
        
        /* Find available slot */
        for (i = 0; i < TCP_BACKLOG; i++)
        {
            if (clients[i] == NULL)
            {
                /* Assign available id to new client */
                client_id = i;
                clients[i] = (struct client *)malloc(sizeof(struct client));
                clients[i]->id = i;
                clients[i]->pid = pid;
            }
        }

        /* Every client is handled by a different process */
        int pid = fork();
        if(pid == -1) {
            close(newsd);
            perror("fork");
            exit(1);
        }

        /* Child is assigned to this socket */
        if(pid == 0){
            int socket_id = newsd;
            int own_id = client_id;
            char buf_in[100];
            char buf_out[100];

            while (1)
            {
                /* Clear buffer from previous contents */
                memset(buf_in,0,sizeof(buf_in));
                num = read(newsd, buf_in, sizeof(buf_in));
                if (num < 0)
                {
                    printf("RETRY\n");
                    continue;
                }
                if (num == 0)
                {
                    printf("BREAK\n");
                    exit(1);
                }

                printf("Client %d: %s", own_id, buf_in);

                /* Write back */
                sprintf(buf_out, "Roger Client %d", own_id);
                write(socket_id, buf_out, strlen(buf_out));
            }
        }

        /* Parent continues */
        else
        {
            close(newsd);
            
            if (client_count == TCP_BACKLOG){
                pause();
            }
        }
    }

    close(sd);
}