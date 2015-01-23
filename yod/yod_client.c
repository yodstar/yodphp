/*
  +----------------------------------------------------------------------+
  | Yod Framework as PHP extension                                       |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Baoqiang Su  <zmrnet@qq.com>                                 |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "Zend/zend_interfaces.h"
#include "Zend/zend_exceptions.h"
#include "ext/standard/url.h"
#include "ext/json/php_json.h"

#include "php_yod.h"
#include "yod_client.h"

#if PHP_YOD_DEBUG
#include "yod_debug.h"
#endif

#define smart_str_append_const(str, const) \
	smart_str_appendl(str, const, sizeof(const)-1)

zend_class_entry *yod_client_ce;

/** {{{ ARG_INFO
*/
ZEND_BEGIN_ARG_INFO_EX(yod_client_construct_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, url)
	ZEND_ARG_INFO(0, handle)
	ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_client_set_arginfo, 0, 0, 2)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_client_get_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_client_isset_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_client_unset_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_client_call_arginfo, 0, 0, 2)
	ZEND_ARG_INFO(0, method)
	ZEND_ARG_INFO(0, params)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_client_debug_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, debug)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_client_timeout_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_client_encrypt_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_client_decrypt_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()
/* }}} */

/** {{{ static int get_http_headers(php_stream *stream, char **response, int *out_size TSRMLS_DC)
*/
static int get_http_headers(php_stream *stream, char **response, int *out_size TSRMLS_DC)
{
	int done = 0;
	smart_str tmp_response = {0};
	char headerbuf[8192];

	while (!done) {
		if (!php_stream_gets(stream, headerbuf, sizeof(headerbuf))) {
			break;
		}

		if ((headerbuf[0] == '\r' && headerbuf[1] == '\n') ||
		    (headerbuf[0] == '\n')) {
			/* empty line marks end of headers */
			done = 1;
			break;
		}

		/* add header to collection */
		smart_str_appends(&tmp_response, headerbuf);
	}
	smart_str_0(&tmp_response);
	(*response) = tmp_response.c;
	(*out_size) = tmp_response.len;
	return done;
}
/* }}} */

/** {{{ static char *get_http_header_value(char *headers, char *type)
*/
static char *get_http_header_value(char *headers, char *type)
{
	char *pos, *tmp = NULL;
	int typelen, headerslen;

	if (!headers) {
		return NULL;
	}

	typelen = strlen(type);
	headerslen = strlen(headers);

	/* header `titles' can be lower case, or any case combination, according
	 * to the various RFC's. */
	pos = headers;
	do {
		/* start of buffer or start of line */
		if (strncasecmp(pos, type, typelen) == 0) {
			char *eol;

			/* match */
			tmp = pos + typelen;
			eol = strchr(tmp, '\n');
			if (eol == NULL) {
				eol = headers + headerslen;
			} else if (eol > tmp && *(eol-1) == '\r') {
				eol--;
			}
			return estrndup(tmp, eol - tmp);
		}

		/* find next line */
		pos = strchr(pos, '\n');
		if (pos) {
			pos++;
		}

	} while (pos);

	return NULL;
}
/* }}} */

/** {{{ static int get_http_body(php_stream *stream, int close, char *headers,  char **response, int *out_size TSRMLS_DC)
*/
static int get_http_body(php_stream *stream, int close, char *headers,  char **response, int *out_size TSRMLS_DC)
{
	char *header, *http_buf = NULL;
	int header_close = close, header_chunked = 0, header_length = 0, http_buf_size = 0;

	if (!close) {
		header = get_http_header_value(headers, "Connection: ");
		if (header) {
			if(!strncasecmp(header, "close", sizeof("close")-1)) header_close = 1;
			efree(header);
		}
	}
	header = get_http_header_value(headers, "Transfer-Encoding: ");
	if (header) {
		if(!strncasecmp(header, "chunked", sizeof("chunked")-1)) header_chunked = 1;
		efree(header);
	}
	header = get_http_header_value(headers, "Content-Length: ");
	if (header) {
		header_length = atoi(header);
		efree(header);
		if (!header_length && !header_chunked) {
			/* Empty response */
			http_buf = emalloc(1);
			http_buf[0] = '\0';
			(*response) = http_buf;
			(*out_size) = 0;
			return 1;
		}
	}

	if (header_chunked) {
		char ch, done, headerbuf[8192];

		done = 0;

		while (!done) {
			int buf_size = 0;

			php_stream_gets(stream, headerbuf, sizeof(headerbuf));
			if (sscanf(headerbuf, "%x", &buf_size) > 0 ) {
				if (buf_size > 0) {
					int len_size = 0;

					if (http_buf_size + buf_size + 1 < 0) {
						efree(http_buf);
						return 0;
					}
					http_buf = erealloc(http_buf, http_buf_size + buf_size + 1);

					while (len_size < buf_size) {
						int len_read = php_stream_read(stream, http_buf + http_buf_size, buf_size - len_size);
						if (len_read <= 0) {
							/* Error or EOF */
							done = 1;
						  break;
						}
						len_size += len_read;
	 					http_buf_size += len_read;
 					}

					/* Eat up '\r' '\n' */
					ch = php_stream_getc(stream);
					if (ch == '\r') {
						ch = php_stream_getc(stream);
					}
					if (ch != '\n') {
						/* Somthing wrong in chunked encoding */
						if (http_buf) {
							efree(http_buf);
						}
						return 0;
					}
 				}
			} else {
				/* Somthing wrong in chunked encoding */
				if (http_buf) {
					efree(http_buf);
				}
				return 0;
			}
			if (buf_size == 0) {
				done = 1;
			}
		}

		/* Ignore trailer headers */
		while (1) {
			if (!php_stream_gets(stream, headerbuf, sizeof(headerbuf))) {
				break;
			}

			if ((headerbuf[0] == '\r' && headerbuf[1] == '\n') ||
			    (headerbuf[0] == '\n')) {
				/* empty line marks end of headers */
				break;
			}
		}

		if (http_buf == NULL) {
			http_buf = emalloc(1);
		}

	} else if (header_length) {
		if (header_length < 0 || header_length >= INT_MAX) {
			return 0;
		}
		http_buf = safe_emalloc(1, header_length, 1);
		while (http_buf_size < header_length) {
			int len_read = php_stream_read(stream, http_buf + http_buf_size, header_length - http_buf_size);
			if (len_read <= 0) {
				break;
			}
			http_buf_size += len_read;
		}
	} else if (header_close) {
		do {
			int len_read;
			http_buf = erealloc(http_buf, http_buf_size + 4096 + 1);
			len_read = php_stream_read(stream, http_buf + http_buf_size, 4096);
			if (len_read > 0) {
				http_buf_size += len_read;
			}
		} while(!php_stream_eof(stream));
	} else {
		return 0;
	}

	http_buf[http_buf_size] = '\0';
	(*response) = http_buf;
	(*out_size) = http_buf_size;
	return 1;
}
/* }}} */

/** {{{ static int yod_client_http_request(zval *url, zval *request, long timeout, zval **response, char **errmsg TSRMLS_DC)
*/
static int yod_client_http_request(zval *url, zval *request, long timeout, zval **response, char **errmsg TSRMLS_DC) {
	smart_str http_request = {0};
	php_stream *stream = NULL;
	php_url *http_url;
	long request_len;

	struct timeval tv;
	char *hostname = NULL;
	long hostname_len;
	char *errstr = NULL;
	int errno;

	char *http_headers, *http_body, *http_version;
	int http_headers_size, http_body_size, http_close;
	char *connection;
	int http_1_1, http_status;


	if (!url || Z_TYPE_P(url) != IS_STRING) {
		spprintf(errmsg, 0, "Cannot connect to server");
		return 0;
	}

	http_url = php_url_parse_ex(Z_STRVAL_P(url), Z_STRLEN_P(url));
	if (!http_url || http_url->host == NULL) {
		spprintf(errmsg, 0, "Cannot connect to server");
		if (http_url) {
			php_url_free(http_url);
		}
		return 0;
	}

	if (http_url->port == 0) {
		if (http_url->scheme != NULL) {
			if (strncmp(http_url->scheme, "https", 5) == 0) {
				http_url->port = 443;
			} else if (strncmp(http_url->scheme, "http", 4) == 0) {
				http_url->port = 80;
			}
		}
	}

	if (http_url->port > 0) {
		hostname_len = spprintf(&hostname, 0, "%s:%ld", http_url->host, http_url->port);
	} else {
		hostname_len = strlen(http_url->host);
		hostname = http_url->host;
	}

	/* Set HTTP request timeout */
	tv.tv_sec = timeout;

	stream = php_stream_xport_create(hostname, hostname_len, REPORT_ERRORS,
			STREAM_XPORT_CLIENT | STREAM_XPORT_CONNECT, NULL, &tv, NULL, &errstr, &errno);

	if (http_url->port > 0) {
		efree(hostname);
	}

	if (stream == NULL) {
		spprintf(errmsg, 0, "Unable to connect to %s:%ld (%s)",
				http_url->host, http_url->port, errstr == NULL ? "Unknown error" : errstr);

		php_url_free(http_url);
		if (errstr) {
			efree(errstr);
		}
		return 0;
	}

	/* Set HTTP request data */
	smart_str_append_const(&http_request, "POST ");
	if (http_url->path) {
		smart_str_appends(&http_request, http_url->path);
	} else {
		smart_str_appendc(&http_request, '/');
	}
	if (http_url->query) {
		smart_str_appendc(&http_request, '?');
		smart_str_appends(&http_request, http_url->query);
	}
	if (http_url->fragment) {
		smart_str_appendc(&http_request, '#');
		smart_str_appends(&http_request, http_url->fragment);
	}
	smart_str_append_const(&http_request, " HTTP/1.0\r\n");
	smart_str_append_const(&http_request, "Host: ");
	smart_str_appends(&http_request, http_url->host);
	smart_str_appendc(&http_request, ':');
	smart_str_append_unsigned(&http_request, http_url->port);
	smart_str_append_const(&http_request, "\r\nConnection: close\r\n");
	smart_str_append_const(&http_request, "User-Agent: Yod_Client\r\n");
	smart_str_append_const(&http_request, "Content-Type: text/json; charset=UTF-8\r\n");
	smart_str_append_const(&http_request, "Content-Length: ");
	if (request && Z_TYPE_P(request) == IS_STRING) {
		smart_str_append_long(&http_request, Z_STRLEN_P(request));
		smart_str_append_const(&http_request, "\r\n\r\n");
		smart_str_appendl(&http_request, Z_STRVAL_P(request), Z_STRLEN_P(request));
	} else {
		smart_str_append_const(&http_request, "0\r\n\r\n");
	}
	smart_str_0(&http_request);
	php_url_free(http_url);

	/* Write-in HTTP request data */
	php_stream_auto_cleanup(stream);
	request_len = php_stream_write(stream, http_request.c, http_request.len);
	if (request_len != http_request.len) {
		spprintf(errmsg, 0, "HTTP request failed");
		smart_str_free(&http_request);
		php_stream_close(stream);
		return 0;
	}
	smart_str_free(&http_request);

	/* Get HTTP response headers */
	do {
		if (!get_http_headers(stream, &http_headers, &http_headers_size TSRMLS_CC)) {
			php_stream_close(stream);
			if (http_headers) {
				MAKE_STD_ZVAL(*response);
				ZVAL_STRINGL(*response, http_headers, http_headers_size, 1);
				efree(http_headers);
				return 1;
			}
			return 0;
		}

		/* Check to see what HTTP status was sent */
		http_1_1 = 0;
		http_status = 0;
		http_version = get_http_header_value(http_headers, "HTTP/");
		if (http_version) {
			char *tmp;

			if (!strncmp(http_version,"1.1", 3)) {
				http_1_1 = 1;
			}

			tmp = strstr(http_version," ");
			if (tmp != NULL) {
				tmp++;
				http_status = atoi(tmp);
			}
			tmp = strstr(tmp," ");
			if (tmp != NULL) {
				tmp++;
			}
			efree(http_version);

			/* Try and get headers again */
			if (http_status == 100) {
				efree(http_headers);
			}
		}
	} while (http_status == 100);

	/* See if the server requested a close */
	if (http_1_1) {
		http_close = 0;
		connection = get_http_header_value(http_headers, "Connection: ");
		if (connection) {
			if (strncasecmp(connection, "close", sizeof("close")-1) == 0) {
				http_close = 1;
			}
			efree(connection);
		}
	} else {
		http_close = 1;
		connection = get_http_header_value(http_headers, "Connection: ");
		if (connection) {
			if (strncasecmp(connection, "Keep-Alive", sizeof("Keep-Alive")-1) == 0) {
				http_close = 0;
			}
			efree(connection);
		}
	}

	/* Get HTTP response body */
	if (!get_http_body(stream, http_close, http_headers, &http_body, &http_body_size TSRMLS_CC)) {
		php_stream_close(stream);
		if (http_headers) {
			efree(http_headers);
		}
		return 0;
	}

	if (http_close) {
		php_stream_close(stream);
		stream = NULL;
	}

	/* Return HTTP response body */
	MAKE_STD_ZVAL(*response);
	ZVAL_STRINGL(*response, http_body, http_body_size, 1);
	efree(http_headers);
	efree(http_body);
	return 1;
}
/* }}} */

/** {{{ void yod_client_construct(yod_client_t *object, char *url, uint url_len, char *handle, uint handle_len, long timeout TSRMLS_DC)
*/
void yod_client_construct(yod_client_t *object, char *url, uint url_len, char *handle, uint handle_len, long timeout TSRMLS_DC) {
	zval *extra;

	if (!object || !url) {
		return;
	}

	zend_update_property_stringl(Z_OBJCE_P(object), object, ZEND_STRL("_url"), url, url_len TSRMLS_CC);
	if (handle_len > 0) {
		zend_update_property_stringl(Z_OBJCE_P(object), object, ZEND_STRL("_handle"), handle, handle_len TSRMLS_CC);
	}
	zend_update_property_long(Z_OBJCE_P(object), object, ZEND_STRL("_timeout"), timeout TSRMLS_CC);

	MAKE_STD_ZVAL(extra);
	array_init(extra);
	zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_extra"), extra TSRMLS_CC);
	zval_ptr_dtor(&extra);
}
/* }}} */

/** {{{ void yod_client_call(yod_client_t *object, char *method, uint method_len, zval *params, zval *result TSRMLS_DC)
*/
void yod_client_call(yod_client_t *object, char *method, uint method_len, zval *params, zval *result TSRMLS_DC) {
	zval *url, *extra, *handle, *timeout, *debug, **ppval;
	zval *decrypt, *response, *response1 = NULL;
	zval *request, *request1, *encrypt;
	char *errmsg = NULL;
	smart_str buf = {0};
	long timeout1 = 5;
	int status = 0;

	if (!object || !method || !params) {
		return;
	}

	url = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_url"), 1 TSRMLS_CC);
	if (!url || Z_TYPE_P(url) != IS_STRING) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Cannot connect to server");
		return;
	}

	extra = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_extra"), 1 TSRMLS_CC);
	if (!extra || Z_TYPE_P(extra) != IS_ARRAY) {
		MAKE_STD_ZVAL(extra);
		array_init(extra);
	}

	handle = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_handle"), 1 TSRMLS_CC);

	// request
	MAKE_STD_ZVAL(request);
	array_init(request);
	add_assoc_string(request, "client", "Yod_Client", 1);
	if (!handle || Z_TYPE_P(handle) != IS_STRING) {
		add_assoc_null(request, "handle");
	} else {
		add_assoc_stringl(request, "handle", Z_STRVAL_P(handle), Z_STRLEN_P(handle), 1);
	}
	add_assoc_stringl(request, "method", method, method_len, 1);
	Z_ADDREF_P(params);
	add_assoc_zval(request, "params", params);
	Z_ADDREF_P(extra);
	add_assoc_zval(request, "extra", extra);
#if PHP_API_VERSION > 20041225
	php_json_encode(&buf, request, 0 TSRMLS_CC);
#else
	php_json_encode(&buf, request TSRMLS_CC);
#endif

	// encrypt
	MAKE_STD_ZVAL(encrypt);
	ZVAL_STRINGL(encrypt, buf.c, buf.len, 1);
	zend_call_method_with_1_params(&object, Z_OBJCE_P(object), NULL, "encrypt", &request1, encrypt);
	zval_ptr_dtor(&encrypt);
	zval_ptr_dtor(&request);
	smart_str_free(&buf);

	if (!request1 || Z_TYPE_P(request1) != IS_STRING) {
		zend_throw_exception_ex(NULL, 0 TSRMLS_CC, "Data encrypt failed");
		if (request1) {
			zval_ptr_dtor(&request1);
		}
		return;
	}

	// timeout
	timeout = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_timeout"), 1 TSRMLS_CC);
	if (timeout && Z_TYPE_P(timeout) == IS_LONG) {
		timeout1 = Z_LVAL_P(timeout);
	}

	if (!yod_client_http_request(url, request1, timeout1, &response1, &errmsg TSRMLS_CC)) {
		if (errmsg) {
			zend_throw_exception_ex(NULL, 0 TSRMLS_CC, errmsg);
			efree(errmsg);
		} else {
			zend_throw_exception_ex(NULL, 0 TSRMLS_CC, "HTTP request failed");
		}
		if (response1) {
			zval_ptr_dtor(&response1);
		}
		zval_ptr_dtor(&request1);
		return;
	}

	if (request1) {
		zval_ptr_dtor(&request1);
	}

	if (!response1 || Z_TYPE_P(response1) != IS_STRING) {
		zend_throw_exception_ex(NULL, 0 TSRMLS_CC, "Empty response from server");
		if (response1) {
			zval_ptr_dtor(&response1);
		}
		return;
	}

	// decrypt
	zend_call_method_with_1_params(&object, Z_OBJCE_P(object), NULL, "decrypt", &decrypt, response1);
	zval_ptr_dtor(&response1);

	if (!decrypt || Z_TYPE_P(decrypt) != IS_STRING) {
		zend_throw_exception_ex(NULL, 0 TSRMLS_CC, "Data decrypt failed");
		if (decrypt) {
			zval_ptr_dtor(&decrypt);
		}
		return;
	}

	// debug
	debug = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_debug"), 1 TSRMLS_CC);
	if (debug && Z_TYPE_P(debug) == IS_BOOL && Z_BVAL_P(debug)) {
		php_printf("<fieldset style=\"width:75%%\"><legend><b>[DEBUG]</b></legend>%s</fieldset><br>\r\n", Z_STRVAL_P(decrypt));
	}

	if (strncmp(Z_STRVAL_P(decrypt), "{\"server\":\"Yod_Server\",", 23) != 0) {
		zend_throw_exception_ex(NULL, 0 TSRMLS_CC, "Incorrect data format");
		zval_ptr_dtor(&decrypt);
		return;
	}

	// json_decode
	MAKE_STD_ZVAL(response);
#if PHP_API_VERSION > 20041225
#ifndef PHP_JSON_OBJECT_AS_ARRAY
	php_json_decode(response, Z_STRVAL_P(decrypt), Z_STRLEN_P(decrypt), 1, 512 TSRMLS_CC);
#else
	php_json_decode(response, Z_STRVAL_P(decrypt), Z_STRLEN_P(decrypt), PHP_JSON_OBJECT_AS_ARRAY, 512 TSRMLS_CC);
#endif
#else
	php_json_decode(response, Z_STRVAL_P(decrypt), Z_STRLEN_P(decrypt), 1 TSRMLS_CC);
#endif
	zval_ptr_dtor(&decrypt);
	
	if (response && Z_TYPE_P(response) == IS_ARRAY) {
		// status
		if (zend_hash_find(Z_ARRVAL_P(response), ZEND_STRS("status"), (void **)&ppval) == SUCCESS) {
			if (ppval && Z_TYPE_PP(ppval) == IS_LONG) {
				status = Z_LVAL_PP(ppval);
			}
		}
		// data
		if (zend_hash_find(Z_ARRVAL_P(response), ZEND_STRS("data"), (void **)&ppval) != SUCCESS) {
			zend_throw_exception_ex(NULL, 0 TSRMLS_CC, "Unknown error");
			zval_ptr_dtor(&response);
			return;
		}
		if (!status) {
			convert_to_string_ex(ppval);
			zend_throw_exception_ex(NULL, 0 TSRMLS_CC, "%s", Z_STRVAL_PP(ppval));
			zval_ptr_dtor(&response);
			return;
		}
		ZVAL_ZVAL(result, *ppval, 1, 0);

		// extra
		if (zend_hash_find(Z_ARRVAL_P(response), ZEND_STRS("extra"), (void **)&ppval) == SUCCESS) {
			if (ppval && Z_TYPE_PP(ppval) == IS_ARRAY) {
				zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_extra"), *ppval TSRMLS_CC);
			}
		}
	}
	zval_ptr_dtor(&response);

}
/* }}} */

/** {{{ proto public Yod_Client::__construct($url)
*/
PHP_METHOD(yod_client, __construct) {
	char *url = NULL;
	uint url_len = 0;
	char *handle = NULL;
	uint handle_len = 0;
	long timeout = 5;

#if PHP_YOD_DEBUG
	yod_debugl(1 TSRMLS_CC);
	yod_debugf("yod_client_construct()");
#endif

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|sl", &url, &url_len, &handle, &handle_len, &timeout) == FAILURE) {
		return;
	}

	yod_client_construct(getThis(), url, url_len, handle, handle_len, timeout TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_Client::__set($name, $value)
*/
PHP_METHOD(yod_client, __set) {
	yod_client_t *object;
	zval *extra, *value = NULL;
	char *name = NULL;
	uint name_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &name, &name_len, &value) == FAILURE) {
		return;
	}

	if (!name || !value) {
		return;
	}

	object = getThis();
	if (!object) {
		return;
	}

	extra = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_extra"), 1 TSRMLS_CC);
	if (extra && Z_TYPE_P(extra) == IS_ARRAY) {
		Z_ADDREF_P(value);
		add_assoc_zval_ex(extra, name, name_len + 1, value);
	}
}
/* }}} */

/** {{{ proto public Yod_Client::__get($name)
*/
PHP_METHOD(yod_client, __get) {
	yod_client_t *object;
	zval *extra, **value;
	char *name = NULL;
	uint name_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &name_len) == FAILURE) {
		return;
	}

	object = getThis();
	if (!object || !name) {
		RETURN_NULL();
	}

	extra = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_extra"), 1 TSRMLS_CC);
	if (extra && Z_TYPE_P(extra) == IS_ARRAY) {
		if (zend_hash_find(Z_ARRVAL_P(extra), name, name_len, (void **)&value) == SUCCESS) {
			RETURN_ZVAL(*value, 1, 0);
		}
	}

	RETURN_NULL();
}
/* }}} */

/** {{{ proto public Yod_Client::__isset($name)
*/
PHP_METHOD(yod_client, __isset) {
	yod_client_t *object;
	zval *extra;
	char *name = NULL;
	uint name_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &name_len) == FAILURE) {
		return;
	}

	object = getThis();
	if (!object || !name) {
		RETURN_FALSE;
	}

	extra = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_extra"), 1 TSRMLS_CC);
	if (extra && Z_TYPE_P(extra) == IS_ARRAY) {
		if (zend_hash_exists(Z_ARRVAL_P(extra), name, name_len)) {
			RETURN_TRUE;
		}
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yod_Client::__unset($name)
*/
PHP_METHOD(yod_client, __unset) {
	yod_client_t *object;
	zval *extra;
	char *name = NULL;
	uint name_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &name_len) == FAILURE) {
		return;
	}

	object = getThis();
	if (!object || !name) {
		return;
	}

	extra = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_extra"), 1 TSRMLS_CC);
	if (extra && Z_TYPE_P(extra) == IS_ARRAY) {
		if (zend_hash_exists(Z_ARRVAL_P(extra), name, name_len)) {
			zend_hash_del_key_or_index(Z_ARRVAL_P(extra), name, name_len, 0, HASH_DEL_KEY);
		}
	}
}
/* }}} */

/** {{{ proto public Yod_Client::__call($method, $params)
*/
PHP_METHOD(yod_client, __call) {
	zval *params = NULL;
	char *method = NULL;
	uint method_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &method, &method_len, &params) == FAILURE) {
		return;
	}

#if PHP_YOD_DEBUG
	yod_debugf("yod_client_call(%s)", method ? method : "");
#endif

	yod_client_call(getThis(), method, method_len, params, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_Client::debug()
*/
PHP_METHOD(yod_client, debug) {
	yod_client_t *object;
	zend_bool debug;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "b", &debug) == FAILURE) {
		return;
	}

	object = getThis();
	if (!object) {
		return;
	}

	zend_update_property_bool(Z_OBJCE_P(object), object, ZEND_STRL("_debug"), debug TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_Client::timeout($timeout)
*/
PHP_METHOD(yod_client, timeout) {
	yod_client_t *object;
	long timeout = 5;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &timeout) == FAILURE) {
		return;
	}

	object = getThis();
	if (!object) {
		return;
	}

	zend_update_property_long(Z_OBJCE_P(object), object, ZEND_STRL("_timeout"), timeout TSRMLS_CC);
}
/* }}} */

/** {{{ proto protected Yod_Client::encrypt($data)
*/
PHP_METHOD(yod_client, encrypt) {
	char *data = NULL;
	uint data_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &data, &data_len) == FAILURE) {
		return;
	}

#if PHP_YOD_DEBUG
	yod_debugl(1 TSRMLS_CC);
	yod_debugf("yod_client_encrypt()");
#endif

	if (!data) {
		RETURN_NULL();
	}

	RETURN_STRINGL(data, data_len, 1);
}
/* }}} */

/** {{{ proto protected Yod_Client::decrypt($data)
*/
PHP_METHOD(yod_client, decrypt) {
	char *data = NULL;
	uint data_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &data, &data_len) == FAILURE) {
		return;
	}

#if PHP_YOD_DEBUG
	yod_debugl(1 TSRMLS_CC);
	yod_debugf("yod_client_decrypt()");
#endif

	if (!data) {
		RETURN_NULL();
	}
	
	RETURN_STRINGL(data, data_len, 1);
}
/* }}} */

/** {{{ yod_action_methods[]
*/
zend_function_entry yod_client_methods[] = {
	PHP_ME(yod_client, __construct,		yod_client_construct_arginfo,	ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(yod_client, __set,			yod_client_set_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_client, __get,			yod_client_get_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_client, __isset,			yod_client_isset_arginfo,		ZEND_ACC_PUBLIC)
	PHP_ME(yod_client, __unset,			yod_client_unset_arginfo,		ZEND_ACC_PUBLIC)
	PHP_ME(yod_client, __call,			yod_client_call_arginfo,		ZEND_ACC_PUBLIC)
	PHP_ME(yod_client, debug,			yod_client_debug_arginfo,		ZEND_ACC_PUBLIC)
	PHP_ME(yod_client, timeout,			yod_client_timeout_arginfo,		ZEND_ACC_PUBLIC)
	PHP_ME(yod_client, encrypt,			yod_client_encrypt_arginfo,		ZEND_ACC_PROTECTED)
	PHP_ME(yod_client, decrypt,			yod_client_decrypt_arginfo,		ZEND_ACC_PROTECTED)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ PHP_MINIT_FUNCTION
*/
PHP_MINIT_FUNCTION(yod_client) {
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "Yod_Client", yod_client_methods);
	yod_client_ce = zend_register_internal_class(&ce TSRMLS_CC);

	zend_declare_property_null(yod_client_ce, ZEND_STRL("_url"), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yod_client_ce, ZEND_STRL("_extra"), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yod_client_ce, ZEND_STRL("_handle"), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_long(yod_client_ce, ZEND_STRL("_timeout"), 5, ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_bool(yod_client_ce, ZEND_STRL("_debug"), 0, ZEND_ACC_PROTECTED TSRMLS_CC);

	return SUCCESS;
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
