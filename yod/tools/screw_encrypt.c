/*
 * php_screw
 * (C) 2007, Kunimasa Noda/PM9.com, Inc. <http://www.pm9.com,  kuni@pm9.com>
 * see file LICENSE for license details
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../crypt/screw.h"
#include "../crypt/screw_key.h"

/** {{{ char *yod_crypt_screw_enc(char *data, int data_len, int *retval_len)
*/
char *yod_crypt_screw_enc(char *data, int data_len, int *retval_len) {
	int status;
	char *retval;
	int coda_len, total_len = 0;

	yod_crypt_z.zalloc = Z_NULL;
	yod_crypt_z.zfree = Z_NULL;
	yod_crypt_z.opaque = Z_NULL;

	yod_crypt_z.next_in = Z_NULL;
	yod_crypt_z.avail_in = 0;
	
	deflateInit(&yod_crypt_z, 1);

	yod_crypt_z.next_out = yod_crypt_zbuf;
	yod_crypt_z.avail_out = YOD_CRYPT_ZBUF_SIZE;
	yod_crypt_z.next_in = data;
	yod_crypt_z.avail_in = data_len;

	retval = malloc(YOD_CRYPT_ZBUF_SIZE);

	while (1) {
		status = deflate(&yod_crypt_z, Z_FINISH);

		if (status == Z_STREAM_END) {
			break;
		}

		if (status != Z_OK) {
			deflateEnd(&yod_crypt_z);
			*retval_len = 0;
			return retval;
		}

		if (yod_crypt_z.avail_out == 0) {
			retval = realloc(retval, total_len + YOD_CRYPT_ZBUF_SIZE);
			memcpy(retval + total_len, yod_crypt_zbuf, YOD_CRYPT_ZBUF_SIZE);
			total_len += YOD_CRYPT_ZBUF_SIZE;
			yod_crypt_z.next_out = yod_crypt_zbuf;
			yod_crypt_z.avail_out = YOD_CRYPT_ZBUF_SIZE;
		}
	}

	if ((coda_len = YOD_CRYPT_ZBUF_SIZE - yod_crypt_z.avail_out) != 0) {
		retval = realloc(retval, total_len + YOD_CRYPT_ZBUF_SIZE);
		memcpy(retval + total_len, yod_crypt_zbuf, coda_len);
		total_len += coda_len;
	}

	deflateEnd(&yod_crypt_z);
	*retval_len = total_len;
	return retval;
}
/* }}} */

/** {{{ void main(int argc, char**argv)
*/
void main(int argc, char**argv) {
	FILE	*fp;
	struct	stat	stat_buf;
	char	*data, *temp;
	int	data_len, temp_len;
	int	key_len = sizeof yod_crypt_key / 2;
	char	filename[256];
	int	i;

	if (argc != 2) {
		fprintf(stderr, "Usage: filename.\n");
		exit(0);
	}
	fp = fopen(argv[1], "r");
	if (fp == NULL) {
		fprintf(stderr, "File not found(%s)\n", argv[1]);
		exit(0);
	}

	fstat(fileno(fp), &stat_buf);
	data_len = stat_buf.st_size;
	data = (char*)malloc(data_len + YOD_CRYPT_SCREW_LEN);
	fread(data, data_len, 1, fp);
	fclose(fp);

	sprintf(filename, "%s.bak", argv[1]);

	if (memcmp(data, YOD_CRYPT_SCREW, YOD_CRYPT_SCREW_LEN) == 0) {
		fprintf(stderr, "Already Crypted(%s)\n", argv[1]);
		exit(0);
	}

	fp = fopen(filename, "w");
	if (fp == NULL) {
		fprintf(stderr, "Can not create backup file(%s)\n", filename);
		exit(0);
	}
	fwrite(data, data_len, 1, fp);
	fclose(fp);

	temp = yod_crypt_screw_enc(data, data_len, &temp_len);

	for(i=0; i<temp_len; i++) {
		temp[i] = (char)yod_crypt_key[(temp_len - i) % key_len] ^ (~(temp[i]));
	}

	fp = fopen(argv[1], "w");
	if (fp == NULL) {
		fprintf(stderr, "Can not create crypt file(%s)\n", filename);
		exit(0);
	}
	fwrite(YOD_CRYPT_SCREW, YOD_CRYPT_SCREW_LEN, 1, fp);
	fwrite(temp, temp_len, 1, fp);
	fclose(fp);
	fprintf(stderr, "Success Crypting(%s)\n", argv[1]);
	free(temp);
	free(data);
}
/* }}} */

/*
* Local variables:
* tab-width: 4
* c-basic-offset: 4
* End:
* vim600: noet sw=4 ts=4 fdm=marker
* vim<600: noet sw=4 ts=4
*/
