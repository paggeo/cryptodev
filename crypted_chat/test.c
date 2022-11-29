 #include <stdio.h>
 #include <fcntl.h>
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
 
 #include <sys/ioctl.h>
 #include <sys/stat.h>

 #include <arpa/inet.h>
 #include <netinet/in.h>
 #include <crypto/cryptodev.h>

 #include "socket-common.h"

 #define DATA_SIZE       256
 #define BLOCK_SIZE      16
 #define KEY_SIZE        16  /* AES128 */

unsigned char buf[256];
unsigned char key[] = {0xbe, 0x91, 0xd3, 0xb6, 0xb8, 0x49, 0xbe, 0xde, 0x39, 0x4c, 0xad, 0x2e, 0x89, 0x24, 0x75, 0x2c};
unsigned char iv_val[] = {0x80, 0x48, 0x43, 0xb0, 0x16, 0xbe, 0x46, 0xc5, 0x33, 0x8e, 0xa4, 0xa6, 0xa7, 0x78, 0x69, 0x03};
struct session_op session_op_val;
unsigned char encrypted[DATA_SIZE];
unsigned char decrypted[DATA_SIZE];
/*Insist until all of the data has been written*/
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

int encrypt(int cfd,unsigned char * input)
{
       
        struct crypt_op crypt_op_val;
        memset(&crypt_op_val, 0, sizeof(crypt_op_val));
	memset(encrypted,0,sizeof(buf));
        /*Encrypt input  to encrypted*/
        crypt_op_val.ses = session_op_val.ses;
        crypt_op_val.len = sizeof(encrypted);
        crypt_op_val.src = input;
        crypt_op_val.dst = encrypted;
        crypt_op_val.iv = iv_val;
        crypt_op_val.op = COP_ENCRYPT;

        if (ioctl(cfd, CIOCCRYPT, &crypt_op_val)) {
                perror("ioctl(CIOCCRYPT)");
                return 1;
        }

       

        return 0;
}
int decrypt(int cfd,unsigned char * input)
{

        struct crypt_op crypt_op_val;
        memset(&crypt_op_val, 0, sizeof(crypt_op_val));
        memset(decrypted,0,sizeof(buf));
        /*Encrypt input  to encrypted*/
        crypt_op_val.ses = session_op_val.ses;
        crypt_op_val.len = sizeof(decrypted);
        crypt_op_val.src = input;
        crypt_op_val.dst = decrypted;
        crypt_op_val.iv = iv_val;
        crypt_op_val.op = COP_DECRYPT;

        if (ioctl(cfd, CIOCCRYPT, &crypt_op_val)) {
                perror("ioctl(CIOCCRYPT)");
                return 1;
        }



        return 0;
}

int main(int argc, char **argv)
{	
	int cfd;


        /*open crypto device*/
        cfd = open("/dev/crypto", O_RDWR);
        if (cfd < 0) {
            perror("open(/dev/crypto)");
            return 1;
        }

        /*Get crypto session for AES128*/
        session_op_val.cipher = CRYPTO_AES_CBC;
        session_op_val.keylen = KEY_SIZE;
        session_op_val.key = key;

        if (ioctl(cfd, CIOCGSESSION, &session_op_val)) {
            perror("ioctl(CIOCGSESSION)");
            return 1;
        }
	buf[0]='X';
	buf[1]='W';
	printf("This the uncrypted: \n");
	int i;
	for(i=0;i<DATA_SIZE;i++){
		printf("%x",buf[i]);
	}
	printf("\n");
	encrypt(cfd,buf);
	printf("This is the encrypted: \n");
	for(i=0;i<DATA_SIZE;i++){
                printf("%x",encrypted[i]);
        }
        printf("\n");
	decrypt(cfd,encrypted);
	printf("This is decrypted:\n");
	for(i=0;i<DATA_SIZE;i++){
                printf("%x",decrypted[i]);
        }
        printf("\n");

        /*Finish crypto session*/
        if (ioctl(cfd, CIOCFSESSION, &session_op_val.ses)) {
            perror("ioctl(CIOCFSESSION)");
            return 1;
        }

        /*close cryto device*/
        if (close(cfd) < 0) {
            perror("close(cfd)");
            return 1;
        }
    
	return 1;

   
}
