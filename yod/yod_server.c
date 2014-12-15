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
#include "ext/standard/file.h"
#include "ext/standard/flock_compat.h"
#include "ext/standard/php_filestat.h"
#include "ext/json/php_json.h"

#ifdef HAVE_SYS_FILE_H
# include <sys/file.h>
#endif

#include "php_yod.h"
#include "yod_server.h"

#if PHP_YOD_DEBUG
#include "yod_debug.h"
#endif

zend_class_entry *yod_server_ce;

/** {{{ ARG_INFO
*/
ZEND_BEGIN_ARG_INFO_EX(yod_server_construct_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, object)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_server_handle_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_server_encrypt_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_server_decrypt_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()
/* }}} */

/** {{{ void yod_server_construct(yod_server_t *object, zval *handle TSRMLS_DC)
*/
void yod_server_construct(yod_server_t *object, zval *handle TSRMLS_DC) {

#if PHP_YOD_DEBUG
	yod_debugl(1 TSRMLS_CC);
	yod_debugf("yod_server_construct()");
#endif

	if (!object || !handle) {
		return;
	}

	zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_handle"), handle TSRMLS_CC);

}
/* }}} */

/** {{{ void yod_server_handle(yod_server_t *object TSRMLS_DC)
*/
void yod_server_handle(yod_server_t *object TSRMLS_DC) {

	zval *result, *handle, *params, *func1, *data, *extra;
	zval *input1, *input2, *output, *output1, *pzval, **ppval, **data1;

	char *input, *errmsg, *method;
	int input_len, errmsg_len, method_len;
	int debug = 0;

	long maxlen = PHP_STREAM_COPY_ALL;

	smart_str buf = {0};

	zend_object *zobj;
	HashTable *props;
	HashPosition pos;

	php_stream *stream1, *stream2;
	php_stream_context *context1 = NULL, *context2 = NULL;

#if PHP_YOD_DEBUG
	yod_debugf("yod_server_handle()");
#endif

	if (!object) {
		return;
	}

	stream1  = php_stream_open_wrapper_ex("php://input",  "rb", 0, NULL, context1);
	if (!stream1) {
		return;
	}

	input_len = php_stream_copy_to_mem(stream1, &input, maxlen, 0);
	php_stream_close(stream1);
	if (input_len < 1) {
		if (input) {
			efree(input);
		}
		return;
	}

	// result
	MAKE_STD_ZVAL(result);
	array_init(result);

	add_assoc_string(result, "server", "Yod_Server", 1);
	add_assoc_long(result, "status", 0);
	add_assoc_null(result, "data");
	add_assoc_null(result, "extra");

	// handle
	handle = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_handle"), 1 TSRMLS_CC);
	if (!handle || Z_TYPE_P(handle) != IS_OBJECT) {
		add_assoc_string(result, "data", "Call to undefined handle", 1);
	} else {
		// decrypt
		MAKE_STD_ZVAL(input1);
		ZVAL_STRINGL(input1, input, input_len, 1);
		zend_call_method_with_1_params(&object, Z_OBJCE_P(object), NULL, "decrypt", &pzval, input1);
		zval_ptr_dtor(&input1);

		if (!pzval) {
			zval_ptr_dtor(&result);
			efree(input);
			goto out;
		}

		if (strncmp(Z_STRVAL_P(pzval), "{\"client\":\"Yod_Client\",", 23) != 0) {
			zval_ptr_dtor(&pzval);
			goto out;
		}

		// json_decode
		MAKE_STD_ZVAL(input2);
#if PHP_API_VERSION > 20041225
#ifndef PHP_JSON_OBJECT_AS_ARRAY
		php_json_decode(input2, Z_STRVAL_P(pzval), Z_STRLEN_P(pzval), 1, 512 TSRMLS_CC);
#else
		php_json_decode(input2, Z_STRVAL_P(pzval), Z_STRLEN_P(pzval), PHP_JSON_OBJECT_AS_ARRAY, 512 TSRMLS_CC);
#endif
#else
		php_json_decode(input2, Z_STRVAL_P(pzval), Z_STRLEN_P(pzval), 1 TSRMLS_CC);
#endif
		zval_ptr_dtor(&pzval);
		if (input2 && Z_TYPE_P(input2) == IS_ARRAY) {
			// method
			if (zend_hash_find(Z_ARRVAL_P(input2), ZEND_STRS("method"), (void **)&ppval) != SUCCESS) {
				add_assoc_string(result, "data", "Call to undefined method", 1);
			} else {
				convert_to_string(*ppval);
				method_len = spprintf(&method, 0, "%s", Z_STRVAL_PP(ppval));

				if (zend_hash_exists(&(Z_OBJCE_P(handle))->function_table, method, method_len + 1)) {
					// extra
					if (zend_hash_find(Z_ARRVAL_P(input2), ZEND_STRS("extra"), (void **)&ppval) == SUCCESS) {
						if (ppval && Z_TYPE_PP(ppval) == IS_ARRAY) {
							zend_hash_internal_pointer_reset_ex(Z_ARRVAL_PP(ppval), &pos);
							while (zend_hash_get_current_data_ex(Z_ARRVAL_PP(ppval), (void **)&data1, &pos) == SUCCESS) {
								char *str_key = NULL;
								uint key_len;
								ulong num_key;

								if (zend_hash_get_current_key_ex(Z_ARRVAL_PP(ppval), &str_key, &key_len, &num_key, 0, &pos) == HASH_KEY_IS_STRING) {
									zend_update_property(Z_OBJCE_P(handle), handle, str_key, key_len - 1, *data1 TSRMLS_CC);
								}

								zend_hash_move_forward_ex(Z_ARRVAL_PP(ppval), &pos);
							}
						}
					}

					// params
					MAKE_STD_ZVAL(params);
					if (zend_hash_find(Z_ARRVAL_P(input2), ZEND_STRS("params"), (void **)&ppval) == SUCCESS) {
						ZVAL_ZVAL(params, *ppval, 1, 0);
					} else {
						ZVAL_NULL(params);
					}

					MAKE_STD_ZVAL(func1);
					array_init(func1);
					Z_ADDREF_P(handle);
					add_next_index_zval(func1, handle);
					add_next_index_stringl(func1, method, method_len, 1);
					zend_call_method_with_2_params(NULL, NULL, NULL, "call_user_func_array", &data, func1, params);
					zval_ptr_dtor(&params);

					if (!EG(exception)) {
						add_assoc_long(result, "status", 1);
						add_assoc_zval(result, "data", data);

						if (Z_OBJ_HT_P(handle)->get_properties) {

							props = Z_OBJ_HT_P(handle)->get_properties(handle TSRMLS_CC);
							if (props) {
								MAKE_STD_ZVAL(extra);
								array_init(extra);

								zobj = zend_objects_get_address(handle TSRMLS_CC);
								zend_hash_internal_pointer_reset_ex(props, &pos);
								while (zend_hash_get_current_data_ex(props, (void **) &ppval, &pos) == SUCCESS) {
#if PHP_API_VERSION > 20090626
									const char *pname, *cname;
#else
									char *pname, *cname;
#endif
									char *str_key = NULL;
									uint key_len;
									ulong num_key;

									if (zend_hash_get_current_key_ex(props, &str_key, &key_len, &num_key, 0, &pos) == HASH_KEY_IS_STRING) {
										if (zend_check_property_access(zobj, str_key, key_len-1 TSRMLS_CC) == SUCCESS) {
											zend_unmangle_property_name(str_key, key_len-1, &cname, &pname);
											/* Not separating references */
											Z_ADDREF_PP(ppval);
											add_assoc_zval_ex(extra, pname, strlen(pname)+1, *ppval);
										}
									}
									zend_hash_move_forward_ex(props, &pos);
								}

								add_assoc_zval(result, "extra", extra);
							}
						}
					}

					zval_ptr_dtor(&func1);

				} else {
					errmsg_len = spprintf(&errmsg, 0, "Call to undefined method %s::%s()", Z_OBJCE_P(handle)->name, method);
					add_assoc_stringl(result, "data", errmsg, errmsg_len, 1);
					efree(errmsg);
				}
				efree(method);
			}
		}
		if (input2) {
			zval_ptr_dtor(&input2);
		}
	}
	efree(input);

out:

	MAKE_STD_ZVAL(output1);
#if PHP_API_VERSION > 20041225
	php_json_encode(&buf, result, 0 TSRMLS_CC);
#else
	php_json_encode(&buf, result TSRMLS_CC);
#endif
	ZVAL_STRINGL(output1, buf.c, buf.len, 1);
	zend_call_method_with_1_params(&object, Z_OBJCE_P(object), NULL, "encrypt", &output, output1);
	zval_ptr_dtor(&output1);
	zval_ptr_dtor(&result);
	smart_str_free(&buf);

	// output
	if (!output || Z_TYPE_P(output) != IS_STRING) {
		if (output) {
			zval_ptr_dtor(&output);
		}
		return;
	}

	stream2  = php_stream_open_wrapper_ex("php://output",  "wb", 0, NULL, context2);
	if (stream2) {
		if (php_stream_supports_lock(stream2)) {
			php_stream_lock(stream2, LOCK_EX);
		}
		php_stream_write(stream2, Z_STRVAL_P(output), Z_STRLEN_P(output));
		php_stream_close(stream2);
	}
	zval_ptr_dtor(&output);

}
/* }}} */

/** {{{ proto public Yod_Server::__construct($handle)
*/
PHP_METHOD(yod_server, __construct) {
	zval *handle = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &handle) == FAILURE) {
		return;
	}

	yod_server_construct(getThis(), handle TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_Server::handle()
*/
PHP_METHOD(yod_server, handle) {

	yod_server_handle(getThis() TSRMLS_CC);
}
/* }}} */

/** {{{ proto protected Yod_Server::encrypt($data)
*/
PHP_METHOD(yod_server, encrypt) {
	char *data = NULL;
	uint data_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &data, &data_len) == FAILURE) {
		return;
	}

#if PHP_YOD_DEBUG
	yod_debugf("yod_server_encrypt()");
#endif

	if (!data) {
		RETURN_NULL();
	}

	RETURN_STRINGL(data, data_len, 1);
}
/* }}} */

/** {{{ proto protected Yod_Server::decrypt($data)
*/
PHP_METHOD(yod_server, decrypt) {
	char *data = NULL;
	uint data_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &data, &data_len) == FAILURE) {
		return;
	}

#if PHP_YOD_DEBUG
	yod_debugf("yod_server_decrypt()");
#endif

	if (!data) {
		RETURN_NULL();
	}
	
	RETURN_STRINGL(data, data_len, 1);
}
/* }}} */

/** {{{ yod_action_methods[]
*/
zend_function_entry yod_server_methods[] = {
	PHP_ME(yod_server, __construct,		yod_server_construct_arginfo,	ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(yod_server, handle,			yod_server_handle_arginfo,		ZEND_ACC_PUBLIC)
	PHP_ME(yod_server, encrypt,			yod_server_encrypt_arginfo,		ZEND_ACC_PROTECTED)
	PHP_ME(yod_server, decrypt,			yod_server_decrypt_arginfo,		ZEND_ACC_PROTECTED)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ PHP_MINIT_FUNCTION
*/
PHP_MINIT_FUNCTION(yod_server) {
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "Yod_Server", yod_server_methods);
	yod_server_ce = zend_register_internal_class(&ce TSRMLS_CC);

	zend_declare_property_null(yod_server_ce, ZEND_STRL("_handle"), ZEND_ACC_PROTECTED TSRMLS_CC);

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
