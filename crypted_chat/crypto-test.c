/*
 * test_crypto.c
 * 
 * Performs a simple encryption-decryption 
 * of random data from /dev/urandom with the 
 * use of the cryptodev device.
 *
 * Stefanos Gerangelos <sgerag@cslab.ece.ntua.gr>
 * Vangelis Koukis <vkoukis@cslab.ece.ntua.gr>
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <crypto/cryptodev.h>

#define DATA_SIZE 256
#define BLOCK_SIZE 16
#define KEY_SIZE 16 /* AES128 */

/* Insist until all of the data has been read */
unsigned char encrypted[DATA_SIZE];
unsigned char iv_val[] = "qghgftrgfbvgfhy";
struct session_op sess;
int encrypt(int cryptoDeviceFd, unsigned char input[])
{
    struct crypt_op crypt_op_val;
    /*clear crypt_op*/
    memset(&crypt_op_val, 0, sizeof(crypt_op_val));
    memset(encrypted, 0, sizeof(encrypted));

        //unsigned char buf[DATA_SIZE];
        crypt_op_val.ses = sess.ses;
        crypt_op_val.len = sizeof(input);
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
ssize_t insist_read(int fd, void *buf, size_t cnt)
{
	ssize_t ret;
	size_t orig_cnt = cnt;

	while (cnt > 0)
	{
		ret = read(fd, buf, cnt);
		if (ret < 0)
			return ret;
		buf += ret;
		cnt -= ret;
	}

	return orig_cnt;
}

static int fill_urandom_buf(unsigned char *buf, size_t cnt)
{
	int crypto_fd;
	int ret = -1;

	crypto_fd = open("/dev/urandom", O_RDONLY);
	if (crypto_fd < 0)
		return crypto_fd;

	ret = insist_read(crypto_fd, buf, cnt);
	close(crypto_fd);

	return ret;
}

static int test_crypto(int cfd)
{
	int i = -1;
//	struct session_op sess;
	struct crypt_op cryp;
	struct
	{
		unsigned char in[DATA_SIZE],
			encrypted[DATA_SIZE],
			decrypted[DATA_SIZE],
			iv[BLOCK_SIZE],
			key[KEY_SIZE];
	} data;

	memset(&sess, 0, sizeof(sess));
	memset(&cryp, 0, sizeof(cryp));

	/*
	 * Use random values for the encryption key,
	 * the initialization vector (IV), and the
	 * data to be encrypted
	 */
	/*if (fill_urandom_buf(data.in, DATA_SIZE) < 0)
	{
		perror("getting data from /dev/urandom\n");
		return 1;
	}*/
	memset(&data.in,0,DATA_SIZE);
	printf("\nData.IN\n");
        for(i=0;i<DATA_SIZE;i++)
                printf("IN[%d]:%x",i,data.in[i]);
        printf("\n");

	/*if (fill_urandom_buf(data.iv, BLOCK_SIZE) < 0)
	{
		perror("getting data from /dev/urandom\n");
		return 1;
	}*/

	if (fill_urandom_buf(data.key, KEY_SIZE) < 0)
	{
		perror("getting data from /dev/urandom\n");
		return 1;
	}
	
	
	data.in[0]='H';
	data.in[1]='e';
	data.in[2]='l';
	data.in[3]='l';
	data.in[4]='o';
	data.in[5]='_';
	data.in[6]='W';
	data.in[7]='o';
	printf("\nOriginal data:\n");
	//for (i = 0; i < DATA_SIZE; i++)
	//	printf("%s", data.in[i]);
	//printf("\n");

	/*
	 * Get crypto session for AES128
	 */
	sess.cipher = CRYPTO_AES_CBC;
	sess.keylen = KEY_SIZE;
	sess.key = data.key;

	if (ioctl(cfd, CIOCGSESSION, &sess))
	{
		perror("ioctl(CIOCGSESSION)");
		return 1;
	}
	encrypt(cfd,data.in);
	printf("\nEncrypted data:\n");
        for (i = 0; i < DATA_SIZE; i++)
                printf("%x", encrypted[i]);
        printf("\n");

	/*
	 * Encrypt data.in to data.encrypted
	 */
	/*cryp.ses = sess.ses;
	cryp.len = sizeof(data.in);
	cryp.src = data.in;
	cryp.dst = data.encrypted;
	cryp.iv = data.iv;
	cryp.op = COP_ENCRYPT;

	if (ioctl(cfd, CIOCCRYPT, &cryp))
	{
		perror("ioctl(CIOCCRYPT)");
		return 1;
	}

	printf("\nEncrypted data:\n");
	for (i = 0; i < DATA_SIZE; i++)
	{
		printf("%x", data.encrypted[i]);
	}
	printf("\n");
*/
	/*
	 * Decrypt data.encrypted to data.decrypted
	 */
	/*cryp.src = data.encrypted;
	cryp.dst = data.decrypted;
	cryp.op = COP_DECRYPT;
	if (ioctl(cfd, CIOCCRYPT, &cryp))
	{
		perror("ioctl(CIOCCRYPT)");
		return 1;
	}

	printf("\nDecrypted data:\n");
	for (i = 0; i < DATA_SIZE; i++)
	{
		printf("%x", data.decrypted[i]);
	}
	printf("\n");
*/
	/* Verify the result */
	if (memcmp(data.in, data.decrypted, sizeof(data.in)) != 0)
	{
		fprintf(stderr, "\nFAIL: Decrypted and original data differ.\n");
		return 1;
	}
	else
		fprintf(stderr, "\nTest passed.\n");

	/* Finish crypto session */
	if (ioctl(cfd, CIOCFSESSION, &sess.ses))
	{
		perror("ioctl(CIOCFSESSION)");
		return 1;
	}

	return 0;
}

int main(void)
{
	int fd;

	fd = open("/dev/crypto", O_RDWR);
	if (fd < 0)
	{
		perror("open(/dev/crypto)");
		return 1;
	}

	if (test_crypto(fd) < 0)
	{
		return 1;
	}

	if (close(fd) < 0)
	{
		perror("close(fd)");
		return 1;
	}

	return 0;
}
