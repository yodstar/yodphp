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
#include "main/SAPI.h"
#include "Zend/zend_interfaces.h"
#include "ext/standard/php_array.h"
#include "ext/standard/php_string.h"

#include "php_yod.h"
#include "yod_application.h"
#include "yod_request.h"
#include "yod_controller.h"

#if PHP_YOD_DEBUG
#include "yod_debug.h"
#endif

zend_class_entry *yod_request_ce;

/** {{{ ARG_INFO
 */
ZEND_BEGIN_ARG_INFO_EX(yod_request_construct_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, route)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_request_route_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, route)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_request_dispatch_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_request_isget_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_request_ispost_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_request_isput_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_request_ishead_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_request_isoptions_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_request_iscli_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_request_isajax_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()
/* }}} */

/** {{{ static void yod_request_err404_html(TSRMLS_D)
*/
static void yod_request_err404_html(TSRMLS_D) {
	HashTable *_SERVER;
	zval **ppval;
	char *errpath = "";

#if PHP_YOD_DEBUG
	yod_debugf("yod_request_err404_html()");
#endif

	if (!PG(http_globals)[TRACK_VARS_SERVER]) {
		zend_is_auto_global(ZEND_STRL("_SERVER") TSRMLS_CC);
	}
	_SERVER = HASH_OF(PG(http_globals)[TRACK_VARS_SERVER]);

	if (zend_hash_find(_SERVER, ZEND_STRS("PHP_SELF"), (void **) &ppval) != FAILURE &&
		Z_TYPE_PP(ppval) == IS_STRING
	) {
		errpath = Z_STRVAL_PP(ppval);
	}

	php_printf(YOD_ERR404_HTML, errpath);
}
/* }}} */

/** {{{ void yod_request_error404(yod_request_t *object, zval *html TSRMLS_DC)
*/
void yod_request_error404(yod_request_t *object, zval *html TSRMLS_DC) {
	zval *method;

	sapi_header_line ctr = {0};
	ctr.response_code = 404;
	ctr.line = "HTTP/1.0 404 Not Found";
	ctr.line_len = sizeof("HTTP/1.0 404 Not Found")-1;
	sapi_header_op(SAPI_HEADER_REPLACE, &ctr TSRMLS_CC);
	sapi_send_headers(TSRMLS_C);

#if PHP_YOD_DEBUG
	yod_debugf("yod_request_error404()");
#endif

	if (html && Z_TYPE_P(html) == IS_STRING) {
		PHPWRITE(Z_STRVAL_P(html), Z_STRLEN_P(html));
	} else {
		if (object && Z_TYPE_P(object) == IS_OBJECT) {
			method = zend_read_property(yod_request_ce, object, ZEND_STRL("method"), 1 TSRMLS_CC);
			if (method && Z_TYPE_P(method) == IS_STRING) {
				if (strncasecmp(Z_STRVAL_P(method), "cli", 3) == 0 || strncasecmp(Z_STRVAL_P(method), "unknown", 7) == 0) {
					PHPWRITE(ctr.line, ctr.line_len);
				} else {
					yod_request_err404_html(TSRMLS_C);
				}
			} else {
				yod_request_err404_html(TSRMLS_C);
			}
		} else {
			PHPWRITE(ctr.line, ctr.line_len);
		}
	}

	yod_do_exit(0 TSRMLS_CC);
}
/* }}} */

/** {{{ void yod_request_erroraction(yod_request_t *object TSRMLS_DC)
*/
void yod_request_erroraction(yod_request_t *object TSRMLS_DC) {
	zval *controller, *target;
	char *controller1, *classpath, *runpath;
	uint controller1_len;
	zend_class_entry **pce = NULL;

#if PHP_YOD_DEBUG
	yod_debugf("yod_request_erroraction()");
#endif

	controller = zend_read_property(yod_request_ce, object, ZEND_STRL("controller"), 1 TSRMLS_CC);
	if (controller && Z_TYPE_P(controller) == IS_STRING) {
		controller1_len = spprintf(&controller1, 0, "%s", Z_STRVAL_P(controller));
		zend_str_tolower(controller1, controller1_len);
	} else {
		controller1_len = spprintf(&controller1, 0, "index");
	}
	runpath = yod_runpath(TSRMLS_C);
	spprintf(&classpath, 0, "%s/actions/%s/ErrorAction.php", runpath, controller1);
	efree(controller1);

	MAKE_STD_ZVAL(target);
	if (VCWD_ACCESS(classpath, F_OK) == 0) {
		yod_include(classpath, NULL, 1 TSRMLS_CC);
#if PHP_API_VERSION < 20100412
		if (zend_lookup_class_ex(ZEND_STRL("ErrorAction"), 0, &pce TSRMLS_CC) == SUCCESS) {
#else
		if (zend_lookup_class_ex(ZEND_STRL("ErrorAction"), NULL, 0, &pce TSRMLS_CC) == SUCCESS) {
#endif
			object_init_ex(target, *pce);
			if (zend_hash_exists(&(*pce)->function_table, ZEND_STRS(ZEND_CONSTRUCTOR_FUNC_NAME))) {
				zend_call_method_with_1_params(&target, *pce, &(*pce)->constructor, ZEND_CONSTRUCTOR_FUNC_NAME, NULL, object);
			}
		} else {
			php_error_docref(NULL TSRMLS_CC, E_ERROR, "Class 'ErrorAction' not found");
		}
	} else {
		efree(classpath);
		zend_update_property_string(yod_request_ce, object, ZEND_STRL("controller"), "Error" TSRMLS_CC);
		spprintf(&classpath, 0, "%s/actions/ErrorAction.php", runpath);
		if (VCWD_ACCESS(classpath, F_OK) == 0) {
			yod_include(classpath, NULL, 1 TSRMLS_CC);
#if PHP_API_VERSION < 20100412
			if (zend_lookup_class_ex(ZEND_STRL("ErrorAction"), 0, &pce TSRMLS_CC) == SUCCESS) {
#else
			if (zend_lookup_class_ex(ZEND_STRL("ErrorAction"), NULL, 0, &pce TSRMLS_CC) == SUCCESS) {
#endif
				object_init_ex(target, *pce);
				if (zend_hash_exists(&(*pce)->function_table, ZEND_STRS(ZEND_CONSTRUCTOR_FUNC_NAME))) {
					zend_call_method_with_1_params(&target, *pce, &(*pce)->constructor, ZEND_CONSTRUCTOR_FUNC_NAME, NULL, object);
				}
			} else {
				php_error_docref(NULL TSRMLS_CC, E_ERROR, "Class 'ErrorAction' not found");
			}
		} else {
			yod_request_error404(object, NULL TSRMLS_CC);
		}
	}
	zval_ptr_dtor(&target);
	efree(classpath);
}
/* }}} */

/** {{{ int yod_request_route(yod_request_t *object, char *route, uint route_len TSRMLS_DC)
*/
int yod_request_route(yod_request_t *object, char *route, uint route_len TSRMLS_DC) {
	HashTable *_SERVER;
	zval      **_REQUEST;
	zend_bool jit_initialization;

	zval *method, *params, *rules, *value1, *pzval, **argv, **pinfo, **data, **data1, **ppval;
	char *classname, *classname1, *controller, *action, *token, *runfile, *pathvar;
	size_t classname_len, classname1_len;
	zend_class_entry **pce = NULL;

	char *str_key, *temp, *name, *value, *route1, *route2, *route3;
	uint key_len, temp_len, name_len, tail_len, pos1, pos2;
	int flag, count1, count2, route3_len;
	ulong num_key;
	HashPosition pos;

#if PHP_YOD_DEBUG
	yod_debugf("yod_request_route(%s)", route ? route : "");
#endif

	if (!object || Z_TYPE_P(object) != IS_OBJECT) {
		return 0;
	}

	zend_update_property_bool(yod_request_ce, object, ZEND_STRL("_routed"), 1 TSRMLS_CC);

#if PHP_API_VERSION < 20100412
	jit_initialization = (PG(auto_globals_jit) && !PG(register_globals) && !PG(register_long_arrays));
#else
	jit_initialization = PG(auto_globals_jit);
#endif

	if (jit_initialization) {
		zend_is_auto_global(ZEND_STRL("_SERVER") TSRMLS_CC);
	}
	_SERVER = HASH_OF(PG(http_globals)[TRACK_VARS_SERVER]);

	if (jit_initialization) {
		zend_is_auto_global(ZEND_STRL("_REQUEST") TSRMLS_CC);
	}
	(void)zend_hash_find(&EG(symbol_table), ZEND_STRS("_REQUEST"), (void **)&_REQUEST);

	if (route_len > 0) {
		route1 = estrndup(route, route_len);
	} else {
		method = zend_read_property(yod_request_ce, object, ZEND_STRL("method"), 1 TSRMLS_CC);
		if (method && Z_TYPE_P(method) == IS_STRING) {
			if (strncasecmp(Z_STRVAL_P(method), "cli", 3) == 0) {
				if((zend_hash_find(_SERVER, ZEND_STRS("argv"), (void **) &argv) != FAILURE ||
					zend_hash_find(&EG(symbol_table), ZEND_STRS("argv"), (void **) &argv) != FAILURE) &&
					Z_TYPE_PP(argv) == IS_ARRAY &&
					zend_hash_index_find(Z_ARRVAL_PP(argv), 1, (void**)&data) == SUCCESS &&
					Z_TYPE_PP(data) == IS_STRING
				){
					route_len = Z_STRLEN_PP(data);
					route1 = estrndup(Z_STRVAL_PP(data), route_len);
				}
			} else if (strncasecmp(Z_STRVAL_P(method), "unknown", 7) == 0) {
				if((zend_hash_find(_SERVER, ZEND_STRS("argv"), (void **) &argv) != FAILURE ||
					zend_hash_find(&EG(symbol_table), ZEND_STRS("argv"), (void **) &argv) != FAILURE) &&
					Z_TYPE_PP(argv) == IS_ARRAY &&
					zend_hash_index_find(Z_ARRVAL_PP(argv), 1, (void**)&data) == SUCCESS &&
					Z_TYPE_PP(data) == IS_STRING
				){
					route_len = Z_STRLEN_PP(data);
					route1 = estrndup(Z_STRVAL_PP(data), route_len);
				} else if (zend_hash_find(_SERVER, ZEND_STRS("PATH_INFO"), (void **) &pinfo) != FAILURE &&
					Z_TYPE_PP(pinfo) == IS_STRING
				) {
					route_len = Z_STRLEN_PP(pinfo);
					route1 = estrndup(Z_STRVAL_PP(pinfo), route_len);
				}
			} else {
				pathvar = yod_pathvar(TSRMLS_C);
				if (HASH_OF(PG(http_globals)[TRACK_VARS_GET]) &&
					zend_hash_find(HASH_OF(PG(http_globals)[TRACK_VARS_GET]), pathvar, strlen(pathvar) + 1, (void **) &data) != FAILURE &&
					Z_TYPE_PP(data) == IS_STRING
				) {
					route_len = Z_STRLEN_PP(data);
					route1 = estrndup(Z_STRVAL_PP(data), route_len);
				} else if (zend_hash_find(_SERVER, ZEND_STRS("PATH_INFO"), (void **) &pinfo) != FAILURE &&
					Z_TYPE_PP(pinfo) == IS_STRING
				) {
					route_len = Z_STRLEN_PP(pinfo);
					route1 = estrndup(Z_STRVAL_PP(pinfo), route_len);
				}
			}
		}
	}

	/* route */
	if (route_len == 0) {
		route1 = estrndup("", 0);
	} else {
		route2 = php_str_to_str(route1, route_len, "\\", 1, "/", 1, &route_len);
		efree(route1);
		route1 = route2;
		while (strstr(route1, "//")) {
			route2 = php_str_to_str(route1, route_len, "//", 2, "/", 1, &route_len);
			efree(route1);
			route1 = route2;
		}
		route3 = route1;
		if (*(route2 + route_len - 1) == '/') {
			*(route2 + route_len - 1) = '\0';
			route_len--;
		}
		while (*route2 == '/') {
			route_len--;
			route2++;
		}
		route1 = estrndup(route2, route_len);
		efree(route3);
	}

	/* uri */
	zend_update_property_string(yod_request_ce, object, ZEND_STRL("uri"), route1 TSRMLS_CC);

	/* params */
	MAKE_STD_ZVAL(params);
	array_init(params);

	/* rules */
	MAKE_STD_ZVAL(rules);
	yod_application_config(ZEND_STRL("urlrules"), rules TSRMLS_CC);
	if (!rules || Z_TYPE_P(rules) != IS_ARRAY) {
		yod_application_config(ZEND_STRL("url_rules"), rules TSRMLS_CC);
	}
	if (rules && Z_TYPE_P(rules) == IS_ARRAY) {
		/*
			'url_rules' => array( 
				'en/*' => '*', 
				'zh/*' => array('*', array('lang' => 'zh')),
			),
		*/
		zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(rules), &pos);
		while (zend_hash_get_current_data_ex(Z_ARRVAL_P(rules), (void **)&data1, &pos) == SUCCESS) {
			if (zend_hash_get_current_key_ex(Z_ARRVAL_P(rules), &str_key, &key_len, &num_key, 0, &pos) == HASH_KEY_IS_STRING) {
				pos1 = key_len - 2;
				if (pos1 > 1 && strncmp(str_key + pos1 - 1, "/*", 2) == 0) {
					if (strncasecmp(route1, str_key, pos1 - 1) || (route_len > pos1 && strncasecmp(route1, str_key, pos1))) {
						zend_hash_move_forward_ex(Z_ARRVAL_P(rules), &pos);
						continue;
					}

					route3_len = -1;
					if (data1) {
						if (Z_TYPE_PP(data1) == IS_ARRAY) {
							if (zend_hash_index_find(Z_ARRVAL_PP(data1), 1, (void**)&ppval) == SUCCESS
								&& Z_TYPE_PP(ppval) == IS_ARRAY
							) {
								php_array_merge(Z_ARRVAL_P(params), Z_ARRVAL_PP(ppval), 0 TSRMLS_CC);
								if (HASH_OF(PG(http_globals)[TRACK_VARS_GET])) {
									php_array_merge(HASH_OF(PG(http_globals)[TRACK_VARS_GET]), Z_ARRVAL_PP(ppval), 0 TSRMLS_CC);
								}
								if (_REQUEST && Z_TYPE_PP(_REQUEST) == IS_ARRAY) {
									php_array_merge(Z_ARRVAL_PP(_REQUEST), Z_ARRVAL_PP(ppval), 0 TSRMLS_CC);
								}
							}

							if (zend_hash_index_find(Z_ARRVAL_PP(data1), 0, (void**)&ppval) == SUCCESS
								&& Z_TYPE_PP(ppval) == IS_STRING
							) {
								route3_len = Z_STRLEN_PP(ppval);
								route3 = estrndup(Z_STRVAL_PP(ppval), route3_len);
							}
						} else if (Z_TYPE_PP(data1) == IS_STRING) {
							route3_len = Z_STRLEN_PP(data1);
							route3 = estrndup(Z_STRVAL_PP(data1), route3_len);
						}

						if (route3_len == 1 && *route3 == '*') {
							efree(route3);
							route3_len = route_len - pos1;
							if (route3_len >= 0) {
								route3 = estrndup(route1 + pos1, route3_len);
							}
						}
					}

					efree(route1);
					if (route3_len >= 0) {
						route1 = estrndup(route3, route3_len);
						route_len = route3_len;
						efree(route3);
						break;
					}

					route1 = estrndup("", 0);
					route_len = 0;
					break;
				}
			}
			zend_hash_move_forward_ex(Z_ARRVAL_P(rules), &pos);
		}

		/*
			'url_rules' => array(
				'rule1' => 'index/rule1/id/1', 
				'rule2/:id' => 'index/rule2', 
				'rule3/:id/edit' => 'index/rule3/action/edit', 
				'rule4/:id/remove' => array('index/rule4', array('action' => 'remove'), 
			),
		*/
		count1 = 0;
		temp = route1;
		temp_len = route_len;
		while (temp_len > 0){
			if (*temp == '/') {
				count1++;
			}
			temp_len--;
			temp++;
		}
		zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(rules), &pos);
		while (zend_hash_get_current_data_ex(Z_ARRVAL_P(rules), (void **)&data1, &pos) == SUCCESS) {
			if (zend_hash_get_current_key_ex(Z_ARRVAL_P(rules), &str_key, &key_len, &num_key, 0, &pos) == HASH_KEY_IS_STRING) {
				pos1 = key_len - 2;
				if (pos1 > 1 && strncmp(str_key + pos1 - 1, "/*", 2) == 0) {
					zend_hash_move_forward_ex(Z_ARRVAL_P(rules), &pos);
					continue;
				}

				count2 = 0;
				temp = str_key;
				temp_len = key_len;
				pos1 = pos2 = flag = 0;
				while (temp_len > 0){
					switch (*temp ) {
						case '/' :
							count2++;
							if (flag == 1) {
								pos2 = key_len - temp_len;
							}
							flag = 0;
							break;
						case ':' :
							if (pos1) {
								pos2 = key_len - temp_len;
							} else {
								pos1 = key_len - temp_len;
							}
							flag = 1;
							break;
					}
					temp_len--;
					temp++;
				}
				if (flag == 1) {
					pos2 = 0;
				}

				if (count1 == count2) {
					tail_len = key_len - pos2 - 1;
					if ((pos1 == 0 && strncasecmp(route1, str_key, key_len)) ||
						(pos1 && strncasecmp(route1, str_key, pos1)) ||
						(pos2 && (route_len < tail_len || strncasecmp(route1 + route_len - tail_len, str_key + pos2, tail_len)))
					) {
 						zend_hash_move_forward_ex(Z_ARRVAL_P(rules), &pos);
						continue;
					}

					if (pos1) {
						name = php_strtok_r(str_key + pos1, "/", &token);
						value = php_strtok_r(route1 + pos1, "/", &token);
						while (name) {
							name_len = strlen(name);
							if (name_len && *name == ':') {
								MAKE_STD_ZVAL(value1);
								if (value && strlen(value)) {
									ZVAL_STRING(value1, value, 1);
								} else {
									ZVAL_NULL(value1);
								}
								name++;
								zend_hash_update(Z_ARRVAL_P(params), name, name_len, (void **)&value1, sizeof(zval *), NULL);
								if (HASH_OF(PG(http_globals)[TRACK_VARS_GET])) {
									zend_hash_update(HASH_OF(PG(http_globals)[TRACK_VARS_GET]), name, name_len, (void **)&value1, sizeof(zval *), NULL);
								}
								if (_REQUEST && Z_TYPE_PP(_REQUEST) == IS_ARRAY) {
									zend_hash_update(Z_ARRVAL_PP(_REQUEST), name, name_len, (void **)&value1, sizeof(zval *), NULL);
								}
							}
							name = php_strtok_r(NULL, "/", &token);
							value = php_strtok_r(NULL, "/", &token);
						}
					}

					efree(route1);
					if (data1) {
						if (Z_TYPE_PP(data1) == IS_ARRAY) {
							if (zend_hash_index_find(Z_ARRVAL_PP(data1), 1, (void**)&ppval) == SUCCESS
								&& Z_TYPE_PP(ppval) == IS_ARRAY
							) {
								php_array_merge(Z_ARRVAL_P(params), Z_ARRVAL_PP(ppval), 0 TSRMLS_CC);
								if (HASH_OF(PG(http_globals)[TRACK_VARS_GET])) {
									php_array_merge(HASH_OF(PG(http_globals)[TRACK_VARS_GET]), Z_ARRVAL_PP(ppval), 0 TSRMLS_CC);
								}
								if (_REQUEST && Z_TYPE_PP(_REQUEST) == IS_ARRAY) {
									php_array_merge(Z_ARRVAL_PP(_REQUEST), Z_ARRVAL_PP(ppval), 0 TSRMLS_CC);
								}
							}

							if (zend_hash_index_find(Z_ARRVAL_PP(data1), 0, (void**)&ppval) == SUCCESS
								&& Z_TYPE_PP(ppval) == IS_STRING
							) {
								route_len = Z_STRLEN_PP(ppval);
								route1 = estrndup(Z_STRVAL_PP(ppval), route_len);
								break;
							}
						} else if (Z_TYPE_PP(data1) == IS_STRING) {
							route_len = Z_STRLEN_PP(data1);
							route1 = estrndup(Z_STRVAL_PP(data1), route_len);
							break;
						}
					}

					route1 = estrndup("", 0);
					route_len = 0;
					break;
				}
			}

			zend_hash_move_forward_ex(Z_ARRVAL_P(rules), &pos);
		}
	}
	zval_ptr_dtor(&rules);

	/* classname */
	runfile = yod_runfile(TSRMLS_C);
	classname_len = strlen(runfile);
	php_basename(runfile, classname_len, ".php", 4, &classname1, &classname1_len TSRMLS_CC);
	zend_str_tolower(classname1, classname1_len);
	classname_len = spprintf(&classname, 0, "%sController", classname1);
	efree(classname1);

#if PHP_API_VERSION < 20100412
	if (zend_lookup_class_ex(classname, classname_len, 0, &pce TSRMLS_CC) == SUCCESS) {
#else
	if (zend_lookup_class_ex(classname, classname_len, NULL, 0, &pce TSRMLS_CC) == SUCCESS) {
#endif
		classname1 = estrndup(classname, classname_len - 10);
		route_len = spprintf(&route2, 0, "%s/%s", classname1, route1);
		efree(classname1);
		efree(route1);
		route1 = route2;
	}

	/* controller */
	controller = php_strtok_r(route1, "/", &token);
	if (!controller) {
		zend_update_property_string(yod_request_ce, object, ZEND_STRL("controller"), "Index" TSRMLS_CC);
		zend_update_property_string(yod_request_ce, object, ZEND_STRL("action"), "index" TSRMLS_CC);
	} else {
		zend_str_tolower(controller, strlen(controller));
		*controller = toupper(*controller);
		zend_update_property_string(yod_request_ce, object, ZEND_STRL("controller"), controller TSRMLS_CC);

		/* action */
		action = php_strtok_r(NULL, "/", &token);
		if (!action) {
			zend_update_property_string(yod_request_ce, object, ZEND_STRL("action"), "index" TSRMLS_CC);
		} else {
			zend_str_tolower(action, strlen(action));
			zend_update_property_string(yod_request_ce, object, ZEND_STRL("action"), action TSRMLS_CC);

			/* params */
			while (name = php_strtok_r(NULL, "/", &token)) {
				name_len = strlen(name);
				if (name_len) {
					MAKE_STD_ZVAL(pzval);
					value = php_strtok_r(NULL, "/", &token);
					if (value && strlen(value)) {
						ZVAL_STRING(pzval, value, 1);
					} else {
						ZVAL_NULL(pzval);
					}
					zend_hash_update(Z_ARRVAL_P(params), name, name_len + 1, (void **)&pzval, sizeof(zval *), NULL);
					if (HASH_OF(PG(http_globals)[TRACK_VARS_GET])) {
						zend_hash_update(HASH_OF(PG(http_globals)[TRACK_VARS_GET]), name, name_len + 1, (void **)&pzval, sizeof(zval *), NULL);
					}
					if (_REQUEST && Z_TYPE_PP(_REQUEST) == IS_ARRAY) {
						zend_hash_update(Z_ARRVAL_PP(_REQUEST), name, name_len + 1, (void **)&pzval, sizeof(zval *), NULL);
					}
				}
			}
		}
	}
	zend_update_property(yod_request_ce, object, ZEND_STRL("params"), params TSRMLS_CC);

	zval_ptr_dtor(&params);
	efree(classname);
	efree(route1);

	return 1;
}
/* }}} */

/** {{{ yod_request_t *yod_request_construct(yod_request_t *object, char *route, uint route_len TSRMLS_DC)
*/
yod_request_t *yod_request_construct(yod_request_t *object, char *route, uint route_len TSRMLS_DC) {
	yod_request_t *retval;
	zval *method;

#if PHP_YOD_DEBUG
	yod_debugf("yod_request_construct()");
#endif

	if (object) {
		retval = object;
	} else {
		MAKE_STD_ZVAL(retval);
		object_init_ex(retval, yod_request_ce);
	}

	MAKE_STD_ZVAL(method);
	if (SG(request_info).request_method) {
		ZVAL_STRING(method, (char *)SG(request_info).request_method, 1);
	} else if (strncasecmp(sapi_module.name, "cli", 3)) {
		ZVAL_STRING(method, "Unknown", 1);
	} else {
		ZVAL_STRING(method, "Cli", 1);
	}
	zend_update_property(yod_request_ce, retval, ZEND_STRL("method"), method TSRMLS_CC);
	zval_ptr_dtor(&method);

	if (route) {
		yod_request_route(retval, route, route_len TSRMLS_CC);
	}

	return retval;
}
/* }}} */

/** {{{ int yod_request_dispatch(yod_request_t *object TSRMLS_DC)
*/
int yod_request_dispatch(yod_request_t *object TSRMLS_DC) {
	zval *controller, *action, *dispatched, *routed, *target;
	char *controller1, *action1, *classname, *classpath, *runpath;
	uint controller1_len, action1_len, classname_len;
	zend_class_entry **pce = NULL;

#if PHP_YOD_DEBUG
	yod_debugf("yod_request_dispatch()");
#endif

	if (!object || Z_TYPE_P(object) != IS_OBJECT) {
		return 0;
	}

	dispatched = zend_read_property(yod_request_ce, object, ZEND_STRL("_dispatched"), 1 TSRMLS_CC);
	if (dispatched && Z_TYPE_P(dispatched) == IS_BOOL && Z_BVAL_P(dispatched)) {
		return 0;
	}
	zend_update_property_bool(yod_request_ce, object, ZEND_STRL("_dispatched"), 1 TSRMLS_CC);

	routed = zend_read_property(yod_request_ce, object, ZEND_STRL("_routed"), 1 TSRMLS_CC);
	if (!routed || Z_TYPE_P(routed) != IS_BOOL || !Z_BVAL_P(routed)) {
		yod_request_route(object, NULL, 0 TSRMLS_CC);
	}

	controller = zend_read_property(yod_request_ce, object, ZEND_STRL("controller"), 1 TSRMLS_CC);
	if (controller && Z_TYPE_P(controller) == IS_STRING) {
		controller1_len = spprintf(&controller1, 0, "%s", Z_STRVAL_P(controller));
		zend_str_tolower(controller1, controller1_len);
		*controller1 = toupper(*controller1);
	} else {
		controller1_len = spprintf(&controller1, 0, "Index");
	}
	classname_len = spprintf(&classname, 0, "%sController", controller1);
	
	MAKE_STD_ZVAL(target);
#if PHP_API_VERSION < 20100412
	if (zend_lookup_class_ex(classname, classname_len, 0, &pce TSRMLS_CC) == SUCCESS) {
#else
	if (zend_lookup_class_ex(classname, classname_len, NULL, 0, &pce TSRMLS_CC) == SUCCESS) {
#endif
		object_init_ex(target, *pce);
		yod_controller_construct(target, object, NULL, 0 TSRMLS_CC);
	} else {
		runpath = yod_runpath(TSRMLS_C);
		spprintf(&classpath, 0, "%s/controllers/%sController.php", runpath, controller1);
		if (VCWD_ACCESS(classpath, F_OK) == 0) {
			yod_include(classpath, NULL, 1 TSRMLS_CC);
#if PHP_API_VERSION < 20100412
			if (zend_lookup_class_ex(classname, classname_len, 0, &pce TSRMLS_CC) == SUCCESS) {
#else
			if (zend_lookup_class_ex(classname, classname_len, NULL, 0, &pce TSRMLS_CC) == SUCCESS) {
#endif
				object_init_ex(target, *pce);
				yod_controller_construct(target, object, NULL, 0 TSRMLS_CC);
			} else {
				php_error_docref(NULL TSRMLS_CC, E_ERROR, "Class '%s' not found", classname);
				zval_ptr_dtor(&target);
				efree(controller1);
				efree(classpath);
				efree(classname);
				return 0;
			}
		} else {
			efree(classpath);
			action = zend_read_property(yod_request_ce, object, ZEND_STRL("action"), 1 TSRMLS_CC);
			if (action && Z_TYPE_P(action) == IS_STRING) {
				action1_len = spprintf(&action1, 0, Z_STRVAL_P(action));
				zend_str_tolower(action1, action1_len);
			} else {
				action1_len = spprintf(&action1, 0, "index");
			}
			zend_str_tolower(controller1, controller1_len);
			spprintf(&classpath, 0, "%s/actions/%s/%sAction.php", runpath, controller1, action1);
			if (VCWD_ACCESS(classpath, F_OK) == 0) {
				efree(classname);
				yod_include(classpath, NULL, 1 TSRMLS_CC);
				classname_len = spprintf(&classname, 0, "%sAction", action1);
#if PHP_API_VERSION < 20100412
				if (zend_lookup_class_ex(classname, classname_len, 0, &pce TSRMLS_CC) == SUCCESS) {
#else
				if (zend_lookup_class_ex(classname, classname_len, NULL, 0, &pce TSRMLS_CC) == SUCCESS) {
#endif
					object_init_ex(target, *pce);
					yod_controller_construct(target, object, NULL, 0 TSRMLS_CC);
				} else {
					php_error_docref(NULL TSRMLS_CC, E_ERROR, "Class '%s' not found", classname);
					zval_ptr_dtor(&target);
					efree(controller1);
					efree(classpath);
					efree(classname);
					efree(action1);
					return 0;
				}
			} else {
				efree(classpath);
				spprintf(&classpath, 0, "%s/controllers/ErrorController.php", runpath);
				if (VCWD_ACCESS(classpath, F_OK) == 0) {
					yod_include(classpath, NULL, 1 TSRMLS_CC);
#if PHP_API_VERSION < 20100412
					if (zend_lookup_class_ex(ZEND_STRL("ErrorController"), 0, &pce TSRMLS_CC) == SUCCESS) {
#else
					if (zend_lookup_class_ex(ZEND_STRL("ErrorController"), NULL, 0, &pce TSRMLS_CC) == SUCCESS) {
#endif
						object_init_ex(target, *pce);
						yod_controller_construct(target, object, ZEND_STRL("error") TSRMLS_CC);
					} else {
						php_error_docref(NULL TSRMLS_CC, E_ERROR, "Class 'ErrorController' not found");
						zval_ptr_dtor(&target);
						efree(controller1);
						efree(classpath);
						efree(action1);
						return 0;
					}
				} else {
					yod_request_erroraction(object TSRMLS_CC);
				}
			}
			efree(action1);
		}
		efree(classpath);
	}
	zval_ptr_dtor(&target);
	efree(controller1);
	efree(classname);

	return 1;
}
/* }}} */

/** {{{ proto public Yod_Request::__construct($route = null)
*/
PHP_METHOD(yod_request, __construct) {
	char *route = NULL;
	size_t route_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &route, &route_len) == FAILURE) {
		return;
	}

	(void)yod_request_construct(getThis(), route, route_len TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_Request::route($route = null)
*/
PHP_METHOD(yod_request, route) {
	yod_request_t *object;
	char *route = NULL;
	uint route_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &route, &route_len) == FAILURE) {
		return;
	}

	object = getThis();

	yod_request_route(object, route, route_len TSRMLS_CC);

	RETURN_ZVAL(object, 1, 0);
}
/* }}} */

/** {{{ proto public Yod_Request::dispatch()
*/
PHP_METHOD(yod_request, dispatch) {

	yod_request_dispatch(getThis() TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_Request::isGet()
*/
PHP_METHOD(yod_request, isGet) {
	zval *method;

	method  = zend_read_property(yod_request_ce, getThis(), ZEND_STRL("method"), 0 TSRMLS_CC);
	if (strncasecmp("GET", Z_STRVAL_P(method), 3) == 0) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yod_Request::isPost()
*/
PHP_METHOD(yod_request, isPost) {
	zval *method;

	method  = zend_read_property(yod_request_ce, getThis(), ZEND_STRL("method"), 0 TSRMLS_CC);
	if (strncasecmp("POST", Z_STRVAL_P(method), 4) == 0) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yod_Request::isPut()
*/
PHP_METHOD(yod_request, isPut) {
	zval *method;

	method  = zend_read_property(yod_request_ce, getThis(), ZEND_STRL("method"), 0 TSRMLS_CC);
	if (strncasecmp("PUT", Z_STRVAL_P(method), 4) == 0) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yod_Request::isHead()
*/
PHP_METHOD(yod_request, isHead) {
	zval *method;

	method  = zend_read_property(yod_request_ce, getThis(), ZEND_STRL("method"), 0 TSRMLS_CC);
	if (strncasecmp("HEAD", Z_STRVAL_P(method), 4) == 0) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yod_Request::isOptions()
*/
PHP_METHOD(yod_request, isOptions) {
	zval *method;

	method  = zend_read_property(yod_request_ce, getThis(), ZEND_STRL("method"), 0 TSRMLS_CC);
	if (strncasecmp("OPTIONS", Z_STRVAL_P(method), 7) == 0) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yod_Request::isCli()
*/
PHP_METHOD(yod_request, isCli) {
	zval *method;

	method  = zend_read_property(yod_request_ce, getThis(), ZEND_STRL("method"), 0 TSRMLS_CC);
	if (strncasecmp("CLI", Z_STRVAL_P(method), 3) == 0) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yod_Request::isAjax()
*/
PHP_METHOD(yod_request, isAjax) {
	zval **ppval;
	HashTable *_SERVER;

	if (!PG(http_globals)[TRACK_VARS_SERVER]) {
		zend_is_auto_global(ZEND_STRL("_SERVER") TSRMLS_CC);
	}
	_SERVER = HASH_OF(PG(http_globals)[TRACK_VARS_SERVER]);
	if (zend_hash_find(_SERVER, ZEND_STRS("HTTP_X_REQUESTED_WITH"), (void **) &ppval) != FAILURE &&
		Z_TYPE_PP(ppval) == IS_STRING &&
		strncasecmp("XmlHttpRequest", Z_STRVAL_PP(ppval), 14) == 0
	) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yod_Request::__destruct()
*/
PHP_METHOD(yod_request, __destruct) {

}
/* }}} */

/** {{{ yod_request_methods[]
*/
zend_function_entry yod_request_methods[] = {
	PHP_ME(yod_request, __construct,	yod_request_construct_arginfo,		ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(yod_request, route,			yod_request_route_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_request, dispatch,		yod_request_dispatch_arginfo,		ZEND_ACC_PUBLIC)
	PHP_ME(yod_request, isGet,			yod_request_isget_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_request, isPost,			yod_request_ispost_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_request, isPut,			yod_request_isput_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_request, isHead,			yod_request_ishead_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_request, isOptions,		yod_request_isoptions_arginfo,		ZEND_ACC_PUBLIC)
	PHP_ME(yod_request, isCli,			yod_request_iscli_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_request, isAjax,			yod_request_isajax_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_request, __destruct,		NULL,	ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ PHP_MINIT_FUNCTION
*/
PHP_MINIT_FUNCTION(yod_request) {
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "Yod_Request", yod_request_methods);
	yod_request_ce = zend_register_internal_class(&ce TSRMLS_CC);
	yod_request_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;

	zend_declare_property_bool(yod_request_ce, ZEND_STRL("_routed"), 0, ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_bool(yod_request_ce, ZEND_STRL("_dispatched"), 0, ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yod_request_ce, ZEND_STRL("uri"), ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(yod_request_ce, ZEND_STRL("controller"), ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(yod_request_ce, ZEND_STRL("action"), ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(yod_request_ce, ZEND_STRL("params"), ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(yod_request_ce, ZEND_STRL("method"), ZEND_ACC_PUBLIC TSRMLS_CC);

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
