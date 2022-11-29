/*
 * socket-server.c
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

#include <arpa/inet.h>
#include <netinet/in.h>

#include "socket-common.h"
#include <crypto/cryptodev.h>

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
unsigned char *encrypt(int cryptoDeviceFd, unsigned char input[], struct session_op session_op_val)
{
    struct crypt_op crypt_op_val;
    unsigned char encrypted[DATA_SIZE];
    /*clear crypt_op*/
    memset(&crypt_op_val, 0, sizeof(&crypt_op_val));
    memset(encrypted, '\0', sizeof(encrypted));

    crypt_op_val.ses = session_op_val.ses;
    crypt_op_val.op = COP_ENCRYPT;
    crypt_op_val.len = sizeof(input);
    crypt_op_val.src = input;
    crypt_op_val.dst = encrypted;
    crypt_op_val.iv = iv_val;
    if (ioctl(cryptoDeviceFd, CIOCCRYPT, &crypt_op_val))
    {
        perror("ioctl(CIOCCRYPT)");
        exit(1);
    }
    return encrypted;
}
unsigned char *decrypt(int cryptoDeviceFd, unsigned char input[], struct session_op session_op_val)
{
    struct crypt_op crypt_op_val;
    unsigned char decrypted[DATA_SIZE];
    /*clear crypt_op*/
    memset(&crypt_op_val, 0, sizeof(&crypt_op_val));
    memset(decrypted, '\0', sizeof(decrypted));

    crypt_op_val.ses = session_op_val.ses;
    crypt_op_val.op = COP_DECRYPT;
    crypt_op_val.len = sizeof(input);
    crypt_op_val.src = input;
    crypt_op_val.dst = decrypted;
    crypt_op_val.iv = iv_val;
    if (ioctl(cryptoDeviceFd, CIOCCRYPT, &crypt_op_val))
    {
        perror("ioctl(CIOCCRYPT)");
        exit(1);
    }
    return decrypted;
}

int main(void)
{
    char buf[100];
    char addrstr[INET_ADDRSTRLEN];
    int sd, newsd;
    ssize_t n;
    socklen_t len;
    struct sockaddr_in sa;

    /* Make sure a broken connection doesn't kill us */
    signal(SIGPIPE, SIG_IGN);

    /* Create TCP/IP socket, used as main chat channel */
    if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        exit(1);
    }
    fprintf(stderr, "Created TCP socket\n");

    /* Bind to a well-known port */
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(TCP_PORT);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sd, (struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
        perror("bind");
        exit(1);
    }
    fprintf(stderr, "Bound TCP socket to port %d\n", TCP_PORT);

    /* Listen for incoming connections */
    if (listen(sd, TCP_BACKLOG) < 0)
    {
        perror("listen");
        exit(1);
    }
    int cryptoDeviceFd = open("/dev/crypto", O_RDWR);
    if (cryptoDeviceFd < 0)
    {
        printf("Error in opening crypto device");
        exit(1);
    }

    unsigned char key[KEY_SIZE];
    if (fill_urandom_buf(key, KEY_SIZE) < 0)
    {
        printf("Error in getting data from /dev/urandom\n");
        return 1;
    }

    /*open here the session for AES128*/
    struct session_op session_op_val;

    session_op_val.cipher = CRYPTO_AES_CBC;
    session_op_val.keylen = KEY_SIZE;
    session_op_val.key = key;

    /*start a session*/
    if (ioctl(cryptoDeviceFd, CIOCGSESSION, &session_op_val))
    {
        printf("Error in ioctl(CIOCGSESSION)");
        exit(1);
    }



    /* Loop forever, accept()ing connections */
    for (;;)
    {
        fprintf(stderr, "Waiting for an incoming connection...\n");

        /* Accept an incoming connection */
        len = sizeof(struct sockaddr_in);
        if ((newsd = accept(sd, (struct sockaddr *)&sa, &len)) < 0)
        {
            perror("accept");
            exit(1);
        }
        if (!inet_ntop(AF_INET, &sa.sin_addr, addrstr, sizeof(addrstr)))
        {
            perror("could not format IP address");
            exit(1);
        }
        fprintf(stderr, "Incoming connection from %s:%d\n",
                addrstr, ntohs(sa.sin_port));

        /* We break out of the loop when the remote peer goes away */
        for (;;)
        {
            n = read(newsd, buf, sizeof(buf));
            if (n <= 0)
            {
                if (n < 0)
                    perror("read from remote peer failed");
                else
                    fprintf(stderr, "Peer went away\n");
                break;
            }
            toupper_buf(buf, n);
            if (insist_write(newsd, buf, n) != n)
            {
                perror("write to remote peer failed");
                break;
            }
        }
        /* Make sure we don't leak open files */
        if (close(newsd) < 0)
            perror("close");
    }

    /* This will never happen */
    return 1;
}
