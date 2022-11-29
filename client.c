#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include "socket-common.h"
#define PORT 49152
#define BACKLOG 10

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

int main(int argc, char *argv[]){
    int sd, port;
	ssize_t n;
	char buf_in[100], buf_out[100];
	char *hostname;
	struct hostent *hp;
	struct sockaddr_in sa;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s hostname port\n", argv[0]);
		exit(1);
	}
	hostname = argv[1];
	port = atoi(argv[2]); /* Needs better error checking */

	/* Create TCP/IP socket, used as main chat channel */
	if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}
	fprintf(stderr, "Created TCP socket\n");
	
	/* Look up remote hostname on DNS */
	if ( !(hp = gethostbyname(hostname))) {
		printf("DNS lookup failed for host %s\n", hostname);
		exit(1);
	}

	/* Connect to remote TCP port */
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	memcpy(&sa.sin_addr.s_addr, hp->h_addr, sizeof(struct in_addr));
	fprintf(stderr, "Connecting to remote host... "); fflush(stderr);
	if (connect(sd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
		perror("connect");
		exit(1);
	}
	fprintf(stderr, "Connected.\n");

	int pid = fork();

	if(pid == -1){
		perror("fork");
		exit(1);
	} 

	/* Child writes socket */
	else if(pid == 0){
		while (1)
		{
			//printf("READING: ");
			fgets(buf_out, sizeof(buf_out), stdin);

			if (write(sd, buf_out, strlen(buf_out)) != strlen(buf_out))
			{
				perror("write");
				exit(1);
			}
		}
	}

	/* Parent reads socket */
	else
	{
		while (1)
		{
			/* Clear buffer from previous contents */
            memset(buf_in,0,sizeof(buf_in));
			n = read(sd, buf_in, sizeof(buf_in));
			if (n < 0)
			{
				perror("read");
				exit(1);
			}

			else if (n == 0) continue;
			
			printf("SERVER> %s", buf_in);
		}
	}

	close(sd);
}