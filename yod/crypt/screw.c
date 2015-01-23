/*
 * php_screw
 * (C) 2007, Kunimasa Noda/PM9.com, Inc. <http://www.pm9.com,  kuni@pm9.com>
 * see file LICENSE for license details
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"

#if PHP_YOD_CRYPT
#include "php_yod.h"
#include "screw.h"
#include "screw_key.h"

/** {{{ char *yod_crypt_screw_dec(char *data, int data_len, int *retval_len)
*/
char *yod_crypt_screw_dec(char *data, int data_len, int *retval_len) {
	int status;
	char *retval;
	int coda_len, total_len = 0;

	yod_crypt_z.zalloc = Z_NULL;
	yod_crypt_z.zfree = Z_NULL;
	yod_crypt_z.opaque = Z_NULL;

	yod_crypt_z.next_in = Z_NULL;
	yod_crypt_z.avail_in = 0;
	
	inflateInit(&yod_crypt_z);

	yod_crypt_z.next_out = yod_crypt_zbuf;
	yod_crypt_z.avail_out = YOD_CRYPT_ZBUF_SIZE;
	yod_crypt_z.next_in = data;
	yod_crypt_z.avail_in = data_len;

	retval = malloc(YOD_CRYPT_ZBUF_SIZE);

	while (1) {
		status = inflate(&yod_crypt_z, Z_NO_FLUSH);

		if (status == Z_STREAM_END) {
			break;
		}

		if (status != Z_OK) {
			inflateEnd(&yod_crypt_z);
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

	inflateEnd(&yod_crypt_z);
	*retval_len = total_len;
	return retval;
}
/* }}} */

/** {{{ FILE *yod_crypt_screw_fopen(FILE *fp)
*/
FILE *yod_crypt_screw_fopen(FILE *fp) {
	struct	stat	stat_buf;
	char	*data, *temp;
	int	data_len, temp_len;
	int	key_len = sizeof yod_crypt_key / 2;
	int	i;

	fstat(fileno(fp), &stat_buf);
	data_len = stat_buf.st_size - YOD_CRYPT_SCREW_LEN;
	data = (char*)malloc(data_len);
	fread(data, data_len, 1, fp);
	fclose(fp);

	for(i=0; i<data_len; i++) {
		data[i] = (char)yod_crypt_key[(data_len - i) % key_len] ^ (~(data[i]));
	}

	temp = yod_crypt_screw_dec(data, data_len, &temp_len);

	fp = tmpfile();
	fwrite(temp, temp_len, 1, fp);

	free(data);
	free(temp);

	rewind(fp);
	return fp;
}
/* }}} */

/** {{{ zend_op_array *yod_crypt_compile_file(zend_file_handle *file_handle, int type TSRMLS_DC)
*/
zend_op_array *yod_crypt_compile_file(zend_file_handle *file_handle, int type TSRMLS_DC) {
	FILE	*fp;
	char	buf[YOD_CRYPT_SCREW_LEN + 1];
	char	fname[32];

	memset(fname, 0, sizeof fname);
	if (zend_is_executing(TSRMLS_C)) {
		if (get_active_function_name(TSRMLS_C)) {
			strncpy(fname, get_active_function_name(TSRMLS_C), sizeof fname - 2);
		}
	}
	if (fname[0]) {
		if ( strcasecmp(fname, "show_source") == 0 || strcasecmp(fname, "highlight_file") == 0) {
			return NULL;
		}
	}

	fp = fopen(file_handle->filename, "r");
	if (!fp) {
		return yod_zend_compile_file(file_handle, type);
	}

	fread(buf, YOD_CRYPT_SCREW_LEN, 1, fp);
	if (memcmp(buf, YOD_CRYPT_SCREW, YOD_CRYPT_SCREW_LEN) != 0) {
		fclose(fp);
		return yod_zend_compile_file(file_handle, type);
	}

	if (file_handle->type == ZEND_HANDLE_FP) {
		fclose(file_handle->handle.fp);
	}

	if (file_handle->type == ZEND_HANDLE_FD) {
		close(file_handle->handle.fd);
	}

	file_handle->handle.fp = yod_crypt_screw_fopen(fp);
	file_handle->type = ZEND_HANDLE_FP;
	file_handle->opened_path = expand_filepath(file_handle->filename, NULL TSRMLS_CC);

	return yod_zend_compile_file(file_handle, type);
}
/* }}} */

#endif

/*
* Local variables:
* tab-width: 4
* c-basic-offset: 4
* End:
* vim600: noet sw=4 ts=4 fdm=marker
* vim<600: noet sw=4 ts=4
*/
