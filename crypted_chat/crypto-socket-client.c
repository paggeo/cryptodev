/*
 * socket-client.c
 * Simple TCP/IP communication using sockets
 *
 * Vangelis Koukis <vkoukis@cslab.ece.ntua.gr>
 */

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "socket-common.h"
#include <crypto/cryptodev.h>

//#include <linux/ioctl.h>
#include <sys/ioctl.h>
#define DATA_SIZE 256
#define BLOCK_SIZE 16
#define KEY_SIZE 16 /* AES128 */

unsigned char key[]={0xbe,0x91,0xd3,0xb6,0xb8,0x49,0xbe,0xde,0x39,0x4c,0xad,0x2e,0x89,0x24,0x75,0x2c};
//unsigned char iv_val[]={0x80,0x48,0x43,0xb0,0x16,0xbe,0x46,0xc5,0x33,0x8e,0xa4,0xa6,0xa7,0x78,0x69,0x03};
unsigned char iv_val[] = "qghgftrgfbvgfhy";
struct session_op session_op_val;
unsigned char encrypted[DATA_SIZE];
unsigned char decrypted[DATA_SIZE];
/*when we write we use encrypt, but when we read we use decrypt*/

/* Insist until all of the data has been written */
ssize_t insist_write(int fd, const void *buf, size_t cnt)
{
    ssize_t ret;
    size_t orig_cnt = cnt;

    while (cnt > 0)
    {
        ret = write(fd, buf, cnt);
        if (ret < 0)
            return ret;
        buf += ret;
        cnt -= ret;
    }

    return orig_cnt;
}

/*if this function return true we have the buf encrypted |else false*/

int encrypt(int cryptoDeviceFd, unsigned char  * input)
{
    struct crypt_op crypt_op_val;
    
    /*clear crypt_op*/
    memset(&crypt_op_val, 0, sizeof(crypt_op_val));
    memset(encrypted, '\0', sizeof(unsigned char));

	//unsigned char buf[DATA_SIZE];   
	crypt_op_val.ses = session_op_val.ses;
        crypt_op_val.len = sizeof(DATA_SIZE);
        crypt_op_val.src = input;
        crypt_op_val.dst = encrypted;
        crypt_op_val.iv = iv_val;
        crypt_op_val.op = COP_ENCRYPT;

    if (ioctl(cryptoDeviceFd,CIOCCRYPT,&crypt_op_val))
    {
        perror("ioctl(CIOCCRYPT1)");
        exit(1);
    }
    return 0;
}

int decrypt(int cryptoDeviceFd, unsigned char  * input)
{
    struct crypt_op crypt_op_val;
    /*clear crypt_op*/
    memset(&crypt_op_val, 0, sizeof(crypt_op_val));
    memset(decrypted, '\0', sizeof(unsigned char));

    crypt_op_val.ses = session_op_val.ses;
    crypt_op_val.op = COP_DECRYPT;
    crypt_op_val.len = sizeof(unsigned char);
    crypt_op_val.src = input;
    crypt_op_val.dst = decrypted;
    crypt_op_val.iv = iv_val;
    if (ioctl(cryptoDeviceFd,CIOCCRYPT,&crypt_op_val))
    {
        perror("ioctl(CIOCCRYPT2)");
        exit(1);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    /*int sd, port;
    ssize_t n;
    char buf_in[100], buf_out[100];
    char *hostname;
    struct hostent *hp;
    struct sockaddr_in sa;

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s hostname port\n", argv[0]);
        exit(1);
    }
    hostname = argv[1];
    port = atoi(argv[2]);*/ /* Needs better error checking */

    /* Create TCP/IP socket, used as main chat channel */
    /*if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        exit(1);
    }
    fprintf(stderr, "Created TCP socket\n");
	*/
    /* Look up remote hostname on DNS */
    /*if (!(hp = gethostbyname(hostname)))
    {
        printf("DNS lookup failed for host %s\n", hostname);
        exit(1);
    }*/

    /* Connect to remote TCP port */
  /*  sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    memcpy(&sa.sin_addr.s_addr, hp->h_addr, sizeof(struct in_addr));
    fprintf(stderr, "Connecting to remote host... ");
    fflush(stderr);
    if (connect(sd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
        perror("connect");
        exit(1);
    }
    fprintf(stderr, "Connected.\n");
*/
    /*open crypto device*/

    int cryptoDeviceFd = open("/dev/crypto", O_RDWR);
    if (cryptoDeviceFd < 0)
    {
        printf("Error in opening crypto device");
        exit(1);
    }

    
    /*open here the session for AES128*/
    

    session_op_val.cipher = CRYPTO_AES_CBC;
    session_op_val.keylen = KEY_SIZE;
    session_op_val.key = key;

    /*start a session*/
    if (ioctl(cryptoDeviceFd, CIOCGSESSION, &session_op_val))
    {
        printf("Error in ioctl(CIOCGSESSION)");
        exit(1);
    }
	
	int i;
	unsigned char buf[DATA_SIZE];
	memset(buf,0,sizeof(buf));
	printf("This is input data: \n");
	for(i=0;i<DATA_SIZE;i++)
		printf("%x",buf[i]);
	printf("\n");
	
	encrypt(cryptoDeviceFd,buf);

	printf("This is encrypted data: \n");
        for(i=0;i<DATA_SIZE;i++)
                printf("%x",encrypted[i]);
        printf("\n");
	
	decrypt(cryptoDeviceFd,buf);
	printf("This is decrypt data: \n");
        for(i=0;i<DATA_SIZE;i++)
                printf("%x",decrypted[i]);
        printf("\n");
    /*int pid = fork();
    if (pid == -1)
    {
        perror("fork");
        exit(1);
    }
    else if (pid == 0)
    {
        while (1)
        {
            fgets(buf_out, sizeof(buf_out), stdin);
            if (insist_write(sd, buf_out, strlen(buf_out)) != strlen(buf_out))
            {
                perror("write");
                exit(1);
            }
        }
    }
    else
    {
        while (1)
        {
            memset(buf_in, 0, sizeof(buf_in));
            n = read(sd, buf_in, sizeof(buf_in));
            if (n < 0)
            {
                perror("read");
                exit(1);
            }
            else if (n == 0)
                continue;
            printf("Server> %s", buf_in);
        }
    }*/

    /*close every thing*/

    /*if (shutdown(sd, SHUT_WR) < 0)
    {
        perror("shutdown");
        exit(1);
    }*/
    /*close socket*/
    /*if (close(sd) < 0)
    {
        perror("close");
        exit(1);
    }*/
    /*close ioctl session*/
	
    if (ioctl(cryptoDeviceFd, CIOCFSESSION, &session_op_val.ses))
    {
        perror("ioctl(CIOCFSESSION)");
        exit(1);
    }
    /*close /dev/crypto*/
    if (close(cryptoDeviceFd) < 0)
    {
        perror("close(cryptoDeviceFd)");
        exit(1);
    }

    return 0;
}
