/*
  +----------------------------------------------------------------------+
  | Yod Framework as PHP extension										 |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,		 |
  | that is bundled with this package in the file LICENSE, and is		 |
  | available through the world-wide-web at the following url:			 |
  | http://www.php.net/license/3_01.txt									 |
  | If you did not receive a copy of the PHP license and are unable to	 |
  | obtain it through the world-wide-web, please send a note to			 |
  | license@php.net so we can mail you a copy immediately.				 |
  +----------------------------------------------------------------------+
  | Author: Baoqiang Su  <zmrnet@qq.com>								 |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "main/SAPI.h"
#include "ext/standard/file.h"
#include "ext/standard/flock_compat.h"
#include "ext/standard/php_filestat.h"
#include "ext/standard/php_array.h"
#include "ext/standard/php_string.h"

#ifdef HAVE_SYS_FILE_H
# include <sys/file.h>
#endif

#ifdef PHP_WIN32
#include "win32/time.h"
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#include "php_yod.h"
#include "yod_application.h"
#include "yod_request.h"

#if PHP_YOD_DEBUG
#include "yod_debug.h"
#endif

zend_class_entry *yod_application_ce;

/** {{{ ARG_INFO
 */
ZEND_BEGIN_ARG_INFO_EX(yod_application_construct_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, config)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_application_run_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_application_config_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_application_import_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, alias)
	ZEND_ARG_INFO(0, classext)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_application_app_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, config)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_application_autorun_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_application_autoload_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, classname)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_application_errorlog_arginfo, 0, 0, 4)
	ZEND_ARG_INFO(0, errno)
	ZEND_ARG_INFO(0, errstr)
	ZEND_ARG_INFO(0, errfile)
	ZEND_ARG_INFO(0, errline)
	ZEND_ARG_INFO(0, errcontext)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_application_destruct_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()
/* }}} */

/** {{{ static void yod_application_init_config(zval *config TSRMLS_DC)
*/
static void yod_application_init_config(zval *config TSRMLS_DC) {
	zval *config1, *value1, *pzval, **ppconf, **data, **ppval;
	char *filepath, *filename, *str_key;
	uint filepath_len, entry_len, key_len;
	ulong num_key;
	HashPosition pos;

	php_stream *stream = NULL;
	php_stream_dirent dirent;
	php_stream_context *context = NULL;

#if PHP_YOD_DEBUG
	yod_debugf("yod_application_init_config()");
#endif

	MAKE_STD_ZVAL(config1);
	if (config && Z_TYPE_P(config) == IS_ARRAY) {
		ZVAL_ZVAL(config1, config, 1, 0);
	} else {
		if(config && Z_TYPE_P(config) == IS_STRING) {
			spprintf(&filepath, 0, "%s", Z_STRVAL_P(config));
		} else {
			spprintf(&filepath, 0, "%s/configs/config.php", yod_runpath(TSRMLS_C));
		}

		if (VCWD_ACCESS(filepath, F_OK) == 0) {
			yod_include(filepath, &config1, 0 TSRMLS_CC);
		} else {
			array_init(config1);
			filepath_len = php_dirname(filepath, strlen(filepath));
			if (VCWD_ACCESS(filepath, F_OK) == 0) {
				stream = php_stream_opendir(filepath, ENFORCE_SAFE_MODE | REPORT_ERRORS, context);	
			}

			if (stream) {
				while (php_stream_readdir(stream, &dirent)) {
					entry_len = strlen(dirent.d_name);
					if (entry_len < 12 || strncmp(dirent.d_name + entry_len - 11, ".config.php", 11)) {
						continue;
					}

					spprintf(&filename, 0, "%s/%s", filepath, dirent.d_name);
					if (VCWD_ACCESS(filename, F_OK) == 0) {
						yod_include(filename, &value1, 0 TSRMLS_CC);
						if (Z_TYPE_P(value1) == IS_ARRAY) {
							if (entry_len == 15 && strncmp(dirent.d_name, "base", 4) == 0) {
								php_array_merge(Z_ARRVAL_P(config1), Z_ARRVAL_P(value1), 0 TSRMLS_CC);
							} else {
								key_len = entry_len - 11;
								str_key = estrndup(dirent.d_name, key_len);

								if (zend_hash_find(Z_ARRVAL_P(config1), str_key, key_len + 1, (void **)&ppval) == SUCCESS &&
									Z_TYPE_PP(ppval) == IS_ARRAY
								) {
									php_array_merge(Z_ARRVAL_P(value1), Z_ARRVAL_PP(ppval), 0 TSRMLS_CC);
								}
								add_assoc_zval_ex(config1, str_key, key_len + 1, value1);
								efree(str_key);
							}
						}
					}
					efree(filename);
				}
				php_stream_closedir(stream);
			}
		}
		efree(filepath);
	}

	if (zend_hash_find(&EG(symbol_table), "config", sizeof("config"), (void **) &ppconf) == SUCCESS &&
		Z_TYPE_PP(ppconf) == IS_ARRAY
	) {
		if (!config1 || Z_TYPE_P(config1) != IS_ARRAY) {
			ZVAL_ZVAL(config1, *ppconf, 1, 0);
		} else {
			zend_hash_internal_pointer_reset_ex(Z_ARRVAL_PP(ppconf), &pos);
			while (zend_hash_get_current_data_ex(Z_ARRVAL_PP(ppconf), (void **)&data, &pos) == SUCCESS) {
				if (zend_hash_get_current_key_ex(Z_ARRVAL_PP(ppconf), &str_key, &key_len, &num_key, 0, &pos) == HASH_KEY_IS_STRING) {
					if (zend_hash_find(Z_ARRVAL_P(config1), str_key, key_len, (void **)&ppval) == SUCCESS &&
						Z_TYPE_PP(ppval) == IS_ARRAY && Z_TYPE_PP(data) == IS_ARRAY
					) {
						php_array_merge(Z_ARRVAL_PP(ppval), Z_ARRVAL_PP(data), 0 TSRMLS_CC);
					} else {
						MAKE_STD_ZVAL(pzval);
						ZVAL_ZVAL(pzval, *data, 1, 0);
						add_assoc_zval_ex(config1, str_key, key_len, pzval);
					}
				}
				zend_hash_move_forward_ex(Z_ARRVAL_PP(ppconf), &pos);
			}
		}
	}

	YOD_G(config) = config1;
}
/* }}} */

/** {{{ static void yod_application_construct(yod_application_t *object, zval *config TSRMLS_DC)
*/
static void yod_application_construct(yod_application_t *object, zval *config TSRMLS_DC) {
	yod_request_t *request;

#if PHP_YOD_DEBUG
	yod_debugf("yod_application_construct()");
#endif

	if (YOD_G(yodapp)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Only one application can be initialized");
		return;
	}

	
	if (object) {
		YOD_G(yodapp) = object;
		zval_add_ref(&object);
	} else {
		MAKE_STD_ZVAL(YOD_G(yodapp));
		object_init_ex(YOD_G(yodapp), yod_application_ce);
	}

	// loading
	if (!YOD_G(loading)) {
		yod_loading(TSRMLS_C);
	}

	// errorlog
	if ((yod_runmode(TSRMLS_C) & 2) && YOD_G(logpath)) {
		yod_register("set_error_handler", "errorlog" TSRMLS_CC);
	}

	// autoload
	yod_register("spl_autoload_register", "autoload" TSRMLS_CC);

	// config
	yod_application_init_config(config TSRMLS_CC);
	zend_update_static_property(yod_application_ce, ZEND_STRL("_config"), YOD_G(config) TSRMLS_CC);

	// imports
	zend_update_static_property(yod_application_ce, ZEND_STRL("_imports"), YOD_G(imports) TSRMLS_CC);

	// request
	request = yod_request_construct(NULL, NULL, 0 TSRMLS_CC);
	zend_update_property(yod_application_ce, YOD_G(yodapp), ZEND_STRL("_request"), request TSRMLS_CC);
	zval_ptr_dtor(&request);

	// app
	zend_update_static_property(yod_application_ce, ZEND_STRL("_app"), YOD_G(yodapp) TSRMLS_CC);
}
/* }}} */

/** {{{ void yod_application_run(TSRMLS_D)
*/
void yod_application_run(TSRMLS_D) {
	yod_request_t *request;

#if PHP_YOD_DEBUG
	yod_debugl(1 TSRMLS_CC);
	yod_debugf("yod_application_run()");
#endif

	if (YOD_G(running)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "An application instance already running");
		return;
	}

	YOD_G(running) = 1;

	zend_update_static_property_bool(yod_application_ce, ZEND_STRL("_running"), 1 TSRMLS_CC);

	request = zend_read_property(yod_application_ce, YOD_G(yodapp), ZEND_STRL("_request"), 1 TSRMLS_CC);
	yod_request_dispatch(request TSRMLS_CC);
}
/* }}} */

/** {{{ int yod_application_config(char *name, uint name_len, zval *result TSRMLS_DC)
*/
int yod_application_config(char *name, uint name_len, zval *result TSRMLS_DC) {
	zval *pzval, **ppval;
	char *name1, *skey, *token;

#if PHP_YOD_DEBUG
	yod_debugf("yod_application_config(%s)", name ? name : "");
#endif
	
	if (!YOD_G(loading)) {
		yod_loading(TSRMLS_C);
	}

	if (!YOD_G(config) || Z_TYPE_P(YOD_G(config)) != IS_ARRAY) {
		ZVAL_NULL(result);
		return 0;
	}

	if (name_len == 0) {
		ZVAL_ZVAL(result, YOD_G(config), 1, 0);
		return 1;
	} else {
		name1 = estrndup(name, name_len);
		if (zend_hash_find(Z_ARRVAL_P(YOD_G(config)), name1, name_len + 1, (void **)&ppval) == SUCCESS) {
			ZVAL_ZVAL(result, *ppval, 1, 0);
			efree(name1);
			return 1;
		} else {
			pzval = YOD_G(config);
			skey = php_strtok_r(name1, ".", &token);
			while (skey) {
				if (zend_hash_find(Z_ARRVAL_P(pzval), skey, strlen(skey) + 1, (void **)&ppval) == SUCCESS) {
					pzval = *ppval;
				} else {
					ZVAL_NULL(result);
					efree(name1);
					return 0;
				}
				skey = php_strtok_r(NULL, ".", &token);
			}
			ZVAL_ZVAL(result, pzval, 1, 0);
		}
		efree(name1);
		return 1;
	}
}
/* }}} */

/** {{{ int yod_application_import(char *alias, uint alias_len, char *classext, uint classext_len TSRMLS_DC)
*/
int yod_application_import(char *alias, uint alias_len, char *classext, uint classext_len TSRMLS_DC) {
	zval **ppval;
	char *classfile, *classname, *classpath;
	size_t classfile_len, classname_len;
	zend_class_entry **pce = NULL;

#if PHP_YOD_DEBUG
	yod_debugf("yod_application_import(%s)", alias ? alias : "");
#endif

	if (!YOD_G(loading)) {
		yod_loading(TSRMLS_C);
	}

	if (alias_len == 0) {
		return 0;
	}

	classfile = estrndup(alias, alias_len);
	classfile_len = 0;
	
	while (*classfile != '\0') {
		if (*classfile == '.' || *classfile == '\\') {
			*classfile = '/';
		}
		classfile++;
		classfile_len++;
	}

	while (*classfile == '/') {
		classfile--;
		classfile_len--;
	}

	classfile = classfile - classfile_len;

	while (*classfile == '/') {
		classfile++;
		classfile_len--;
	}

	php_basename(classfile, classfile_len, NULL, 0, &classname, &classname_len TSRMLS_CC);

	if (zend_hash_find(Z_ARRVAL_P(YOD_G(imports)), alias, alias_len + 1, (void **)&ppval) == FAILURE) {
		if (classfile_len > 4 && strncasecmp(classfile, "yod/", 4) == 0) {
			if (classext_len) {
				spprintf(&classpath, 0, "%s/extends/%s%s", yod_extpath(TSRMLS_C), classfile + 4, classext);
			} else {
				spprintf(&classpath, 0, "%s/extends/%s.class.php", yod_extpath(TSRMLS_C), classfile + 4);
			}
		} else {
			if (classext_len) {
				spprintf(&classpath, 0, "%s/extends/%s%s", yod_runpath(TSRMLS_C), classfile, classext);
			} else {
				spprintf(&classpath, 0, "%s/extends/%s.class.php", yod_runpath(TSRMLS_C), classfile);
			}
		}

		if (VCWD_ACCESS(classpath, F_OK) == 0) {
			yod_include(classpath, NULL, 1 TSRMLS_CC);
		}
		efree(classpath);

		add_assoc_string_ex(YOD_G(imports), alias, alias_len + 1, classpath, 1);
		zend_update_static_property(yod_application_ce, ZEND_STRL("_imports"), YOD_G(imports) TSRMLS_CC);
	}
	efree(classfile);

#if PHP_API_VERSION < 20100412
	if (zend_lookup_class_ex(classname, classname_len, 0, &pce TSRMLS_CC) == SUCCESS) {
#else
	if (zend_lookup_class_ex(classname, classname_len, NULL, 0, &pce TSRMLS_CC) == SUCCESS) {
#endif
		efree(classname);
		return 1;
	}
	efree(classname);
	return 0;
}
/* }}} */

/** {{{ void yod_application_app(zval *config TSRMLS_DC)
*/
void yod_application_app(zval *config TSRMLS_DC) {

#if PHP_YOD_DEBUG
	yod_debugf("yod_application_app()");
#endif

	if (!YOD_G(yodapp)) {
		yod_application_construct(NULL, config TSRMLS_CC);
	}
}
/* }}} */

/** {{{ void yod_application_autorun(TSRMLS_D)
*/
void yod_application_autorun(TSRMLS_D) {
	zval runpath;

	if (YOD_G(running) || YOD_G(exited) || (yod_runmode(TSRMLS_C) & 1) == 0) {
		return;
	}

#if PHP_YOD_DEBUG
	yod_debugf("yod_application_autorun()");
#endif

	if (zend_get_constant(ZEND_STRL("YOD_RUNPATH"), &runpath TSRMLS_CC)) {
		SG(headers_sent) = 0;

		yod_application_app(NULL TSRMLS_CC);
		yod_application_run(TSRMLS_C);

		if (!SG(headers_sent)) {
			sapi_send_headers(TSRMLS_C);
		}

		zval_dtor(&runpath);
	}
}
/* }}} */

/** {{{ static int yod_application_autoload(char *classname, uint classname_len TSRMLS_DC)
*/
static int yod_application_autoload(char *classname, uint classname_len TSRMLS_DC) {
	zend_class_entry **pce = NULL;
	char *classfile, *classpath;

	classfile = estrndup(classname, classname_len);
	// class name with namespace in PHP 5.3
	if (strstr(classname, "\\")) {
		while (*classfile != '\0') {
			if (*classfile == '\\') {
				*classfile = '_';
			}
			classfile++;
		}
		classfile = classfile - classname_len + 1;
	}

	if (strncmp(classfile, "Yod_", 4) == 0) { // yodphp extends class
		if (strncmp(classfile, "Yod_Db", 6) == 0) {
			spprintf(&classpath, 0, "%s/drivers/%s.class.php", yod_extpath(TSRMLS_C), classfile + 4);
		} else {
			spprintf(&classpath, 0, "%s/extends/%s.class.php", yod_extpath(TSRMLS_C), classfile + 4);
		}
	} else {
		if (classname_len > 5 && strncmp(classfile + classname_len - 5, "Model", 5) == 0) {
			spprintf(&classpath, 0, "%s/models/%s.php", yod_runpath(TSRMLS_C), classfile);
		} else {
			spprintf(&classpath, 0, "%s/extends/%s.class.php", yod_runpath(TSRMLS_C), classfile);
		}
	}
	efree(classfile);

#if PHP_YOD_DEBUG
	yod_debugf("yod_application_autoload(%s):%s", classname, classpath);
#endif

	if (VCWD_ACCESS(classpath, F_OK) == 0) {
		yod_include(classpath, NULL, 1 TSRMLS_CC);
	}

	efree(classpath);

#if PHP_API_VERSION < 20100412
	if (zend_lookup_class_ex(classname, classname_len, 0, &pce TSRMLS_CC) == SUCCESS) {
#else
	if (zend_lookup_class_ex(classname, classname_len, NULL, 0, &pce TSRMLS_CC) == SUCCESS) {
#endif
		return 1;
	}
	return 0;
}
/* }}} */

/** {{{ static int yod_application_errorlog(long errnum, char *errstr, uint errstr_len, char *errfile, uint errfile_len, long errline, zval *errcontext TSRMLS_DC)
*/
static int yod_application_errorlog(long errnum, char *errstr, uint errstr_len, char *errfile, uint errfile_len, long errline, zval *errcontext TSRMLS_DC) {
	struct timeval tp = {0};
	struct tm *ta, tmbuf;
	time_t curtime;
	char *datetime, asctimebuf[52];
	uint datetime_len;

	zval *zcontext = NULL;
	php_stream_statbuf ssb;
	php_stream_context *context;
	php_stream *stream;

	char *logdata, *logpath, *logfile, *errtype;
	uint logdata_len;

	switch (errnum) {
	 	case E_ERROR:
	 	case E_CORE_ERROR:
	 	case E_COMPILE_ERROR:
	 	case E_USER_ERROR:
	 	case E_RECOVERABLE_ERROR:
	 		errtype = "Error";
	 		break;
	 	case E_WARNING:
	 	case E_CORE_WARNING:
	 	case E_COMPILE_WARNING:
	 	case E_USER_WARNING:
	 		errtype = "Warning";
	 		break;
	 	case E_PARSE:
	 		errtype = "Parse";
	 		break;
	 	case E_NOTICE:
	 	case E_USER_NOTICE:
	 		errtype = "Notice";
	 		break;
	 	case E_STRICT:
	 		errtype = "Strict";
	 		break;
#ifdef E_DEPRECATED
	 	case E_DEPRECATED:
	 	case E_USER_DEPRECATED:
	 		errtype = "Deprecated";
	 		break;
#endif
	 	default:
	 		errtype = "Unknown";
	 		break;
	}

	logpath = yod_logpath(TSRMLS_C);
	if (php_stream_stat_path(logpath, &ssb) == FAILURE) {
		if (!php_stream_mkdir(logpath, 0750, REPORT_ERRORS, NULL)) {
			return 0;
		}
	}

	spprintf(&logfile, 0, "%s/errors.log", logpath);
	context = php_stream_context_from_zval(zcontext, 0);

#if PHP_API_VERSION < 20100412
	stream = php_stream_open_wrapper_ex(logfile, "ab", ENFORCE_SAFE_MODE | REPORT_ERRORS, NULL, context);
#else
	stream = php_stream_open_wrapper_ex(logfile, "ab", REPORT_ERRORS, NULL, context);
#endif

	if (stream) {
		if (php_stream_supports_lock(stream)) {
			php_stream_lock(stream, LOCK_EX);
		}

		time(&curtime);
		ta = php_localtime_r(&curtime, &tmbuf);
		datetime = php_asctime_r(ta, asctimebuf);
		datetime_len = strlen(datetime);
		datetime[datetime_len - 1] = 0;

		if (!gettimeofday(&tp, NULL)) {
			logdata_len = spprintf(&logdata, 0, "[%s %06d] %s: %s in %s(%d)\n", datetime, tp.tv_usec, errtype, (errstr_len ? errstr : ""), (errfile_len ? errfile : "Unknown"), errline);
			php_stream_write(stream, logdata, logdata_len);
			efree(logdata);
		}
		php_stream_close(stream);
	}
	efree(logfile);

	return 1;
}
/* }}} */

/** {{{ proto public Yod_Application::__construct($config = null)
*/
PHP_METHOD(yod_application, __construct) {
	zval *config = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z!", &config) == FAILURE) {
		return;
	}

	yod_application_construct(getThis(), config TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_Application::run()
*/
PHP_METHOD(yod_application, run) {

	yod_application_run(TSRMLS_C);
}
/* }}} */

/** {{{ proto public Yod_Application::config($name = null)
*/
PHP_METHOD(yod_application, config) {
	char *name = NULL;
	uint name_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &name, &name_len) == FAILURE) {
		return;
	}

	yod_application_config(name, name_len, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_Application::import($alias, $classext = '.class.php')
*/
PHP_METHOD(yod_application, import) {
	char *alias = NULL, *classext = NULL;
	uint alias_len = 0, classext_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s", &alias, &alias_len, &classext, &classext_len) == FAILURE) {
		return;
	}

	if (yod_application_import(alias, alias_len, classext, classext_len TSRMLS_CC)) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public static Yod_Application::app($config = null)
*/
PHP_METHOD(yod_application, app) {
	zval *config = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z!", &config) == FAILURE) {
		return;
	}

	yod_application_app(config TSRMLS_CC);

	RETURN_ZVAL(YOD_G(yodapp), 1, 0);
}
/* }}} */

/** {{{ proto public static Yod_Application::autorun()
*/
PHP_METHOD(yod_application, autorun) {

	yod_application_autorun(TSRMLS_C);
}
/* }}} */

/** {{{ proto public static Yod_Application::autoload($classname)
*/
PHP_METHOD(yod_application, autoload) {
	char *classname;
	uint classname_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &classname, &classname_len) == FAILURE) {
		return;
	}

	if (yod_application_autoload(classname, classname_len TSRMLS_CC)) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public static Yod_Application::errorlog($errno, $errstr, $errfile = '', $errline = 0, $errcontext = array())
*/
PHP_METHOD(yod_application, errorlog) {
	zval *errcontext = NULL;
	char *errstr, *errfile = NULL;
	uint errstr_len, errfile_len = 0;
	long errnum, errline = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls|slz!", &errnum, &errstr, &errstr_len, &errfile, &errfile_len, &errline, &errcontext) == FAILURE) {
		return;
	}

	yod_application_errorlog(errnum, errstr, errstr_len, errfile, errfile_len, errline, errcontext TSRMLS_CC);

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yod_Application::__destruct()
*/
PHP_METHOD(yod_application, __destruct) {

	if (!YOD_G(running) && !YOD_G(exited)) {
		yod_application_run(TSRMLS_C);
	}

#if PHP_YOD_DEBUG
	yod_debugs(TSRMLS_C);
#endif
}
/* }}} */

/* {{{ yod_application_methods[]
*/
zend_function_entry yod_application_methods[] = {
	PHP_ME(yod_application, __construct,	yod_application_construct_arginfo,	ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(yod_application, run,			yod_application_run_arginfo,		ZEND_ACC_PUBLIC)
	PHP_ME(yod_application, app,			yod_application_app_arginfo,		ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod_application, config,			yod_application_config_arginfo,		ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod_application, import,			yod_application_import_arginfo,		ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod_application, autorun,		yod_application_autorun_arginfo,	ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod_application, autoload,		yod_application_autoload_arginfo,	ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod_application, errorlog,		yod_application_errorlog_arginfo,	ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod_application, __destruct,		yod_application_destruct_arginfo,	ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ PHP_MINIT_FUNCTION
*/
PHP_MINIT_FUNCTION(yod_application) {
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "Yod_Application", yod_application_methods);
	yod_application_ce = zend_register_internal_class(&ce TSRMLS_CC);
	yod_application_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;

	zend_declare_property_null(yod_application_ce, ZEND_STRL("_app"), ZEND_ACC_PROTECTED|ZEND_ACC_STATIC TSRMLS_CC);
	zend_declare_property_null(yod_application_ce, ZEND_STRL("_config"), ZEND_ACC_PROTECTED|ZEND_ACC_STATIC TSRMLS_CC);
	zend_declare_property_null(yod_application_ce, ZEND_STRL("_imports"), ZEND_ACC_PROTECTED|ZEND_ACC_STATIC TSRMLS_CC);
	zend_declare_property_bool(yod_application_ce, ZEND_STRL("_running"), 0, ZEND_ACC_PROTECTED|ZEND_ACC_STATIC TSRMLS_CC);
	zend_declare_property_null(yod_application_ce, ZEND_STRL("_request"), ZEND_ACC_PROTECTED TSRMLS_CC);

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
