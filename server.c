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
#include <signal.h>
#include "socket-common.h"

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

int main(){
    //signal(SIGCHLD, SIG_IGN);

    int sd = socket(PF_INET, SOCK_STREAM, 0);
    int newsd;
    socklen_t len;
    int i, num, client_id = 0;
    int pid[2];
    char buf[100];

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
        if ((newsd = accept(sd, (struct sockaddr *)&sa, &len)) < 0)
        {
            perror("accept");
            exit(1);
        }
        printf("Accepted client\n");

        pid[0] = fork();
        if(pid[0] == -1) {
            perror("fork");
            exit(1);
        }

        /* Child is assigned to read from client */
        if(pid[0] == 0){
            int socket_id = newsd;
            int own_id = client_id;
            char own_buf[100];

            while (1)
            {
                /* Clear buffer from previous contents */
                memset(own_buf,0,sizeof(own_buf));
                num = read(newsd, own_buf, sizeof(own_buf));
                
                if (num <= 0)
                {
                    printf("BREAK\n");
                    exit(0);
                }

                printf("Client> %s", own_buf);
            }
        }
        
        pid[1] = fork();
        if(pid[1] == -1) {
            perror("fork");
            exit(1);
        }

        /* Child is assigned to write to client */
        if(pid[1] == 0){
            char buf_out[100];
            int socket_id = newsd;

            while (1)
            {
                //printf("READING> ");
                fgets(buf_out, sizeof(buf_out), stdin);

                if (write(socket_id, buf_out, strlen(buf_out)) != strlen(buf_out))
                {
                    perror("write");
                    exit(0);
                }
            }
        }
        
        wait(NULL);
        kill(pid[1], SIGKILL);
        wait(NULL);
        
        printf("LOST CONNECTION WITH CLIENT\n");
    }

    close(sd);
}