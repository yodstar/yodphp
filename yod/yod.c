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
#include "php_ini.h"
#include "main/SAPI.h"
#include "Zend/zend_interfaces.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"

/*
#include "Zend/zend_alloc.h"
#include "ext/standard/php_var.h"
#include "ext/standard/php_math.h"
#include "ext/standard/php_smart_str.h"
*/

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
#include "yod_controller.h"
#include "yod_action.h"
#include "yod_widget.h"
#include "yod_model.h"
#include "yod_dbmodel.h"
#include "yod_database.h"
#include "yod_dbpdo.h"

#if PHP_YOD_DEBUG
#include "yod_debug.h"
#endif

#define MICRO_IN_SEC 1000000.00

ZEND_DECLARE_MODULE_GLOBALS(yod);

/** {{{ int yod_do_exit(long status TSRMLS_DC)
*/
int yod_do_exit(long status TSRMLS_DC) {

#if PHP_YOD_DEBUG
	yod_debugf("yod_do_exit()");
	yod_debugs(TSRMLS_C);
#endif

	YOD_G(exited) = 1;
	EG(exit_status) = status;
	zend_set_memory_limit(PG(memory_limit));
	zend_objects_store_mark_destructed(&EG(objects_store) TSRMLS_CC);
	zend_bailout();

	return 1;
}
/* }}} */

/** {{{ int yod_call_method(zval *object, char *func, int func_len, zval **result, int pcount, zval *arg1, zval *arg2, zval *arg3, zval *arg4 TSRMLS_DC)
*/
int yod_call_method(zval *object, char *func, int func_len, zval **result, int pcount, zval *arg1, zval *arg2, zval *arg3, zval *arg4 TSRMLS_DC)
{
	zval *method, *argv[4], *pzval, retval;

#if PHP_YOD_DEBUG
	if (object) {
		yod_debugf("yod_call_method(%s, %s)", Z_OBJCE_P(object)->name, func ? func : "");
	} else {
		yod_debugf("yod_call_method(%s)", func ? func : "");
	}
#endif

	MAKE_STD_ZVAL(argv[0]);
	MAKE_STD_ZVAL(argv[1]);
	MAKE_STD_ZVAL(argv[2]);
	MAKE_STD_ZVAL(argv[3]);

	if (arg1) {
		ZVAL_ZVAL(argv[0], arg1, 1, 0);
	} else {
		ZVAL_NULL(argv[0]);
	}
	if (arg2) {
		ZVAL_ZVAL(argv[1], arg2, 1, 0);
	} else {
		ZVAL_NULL(argv[1]);
	}
	if (arg3) {
		ZVAL_ZVAL(argv[2], arg3, 1, 0);
	} else {
		ZVAL_NULL(argv[2]);
	}
	if (arg4) {
		ZVAL_ZVAL(argv[3], arg4, 1, 0);
	} else {
		ZVAL_NULL(argv[3]);
	}

	MAKE_STD_ZVAL(method);
	ZVAL_STRINGL(method, func, func_len, 1);

	if (call_user_function(NULL, &object, method, &retval, pcount, argv TSRMLS_CC) == FAILURE) {
		if (result) {
			ZVAL_BOOL(*result, 0);
		}
		zval_ptr_dtor(&argv[0]);
		zval_ptr_dtor(&argv[1]);
		zval_ptr_dtor(&argv[2]);
		zval_ptr_dtor(&argv[3]);
		zval_ptr_dtor(&method);
		zval_dtor(&retval);

		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Error calling %s::%s()", Z_OBJCE_P(object)->name, func);
		return 0;
	}
	
	if (result) {
		pzval = &retval;
		ZVAL_ZVAL(*result, pzval, 1, 0);
	}
	zval_ptr_dtor(&argv[0]);
	zval_ptr_dtor(&argv[1]);
	zval_ptr_dtor(&argv[2]);
	zval_ptr_dtor(&argv[3]);
	zval_ptr_dtor(&method);
	zval_dtor(&retval);

	return 1;
}
/* }}} */

/** {{{ int yod_register(char *moduel, char *method TSRMLS_DC)
 * */
int yod_register(char *moduel, char *method TSRMLS_DC) {
	zval *param1, *function, *retval = NULL;
	zval **params[1] = {&param1};
	zend_fcall_info fci;

	MAKE_STD_ZVAL(param1);
	array_init(param1);
	add_next_index_string(param1, YOD_APP_CNAME, 1);
	add_next_index_string(param1, method, 1);

	MAKE_STD_ZVAL(function);
	ZVAL_STRING(function, moduel, 1);

	fci.size = sizeof(fci);
	fci.function_table = EG(function_table);
	fci.function_name = function;
	fci.symbol_table = NULL;
	fci.retval_ptr_ptr = &retval;
	fci.param_count = 1;
	fci.params = (zval ***)params;
#if PHP_API_VERSION > 20041225
	fci.object_ptr = NULL;
#else
	fci.object_pp = NULL;
#endif
	fci.no_separation = 1;

	zend_call_function(&fci, NULL TSRMLS_CC);

	zval_ptr_dtor(&function);
	zval_ptr_dtor(&param1);

	if (retval) {
		zval_ptr_dtor(&retval);
		return 1;
	}
	return 0;
}
/* }}} */

/** {{{ double yod_runtime(TSRMLS_D)
*/
double yod_runtime(TSRMLS_D) {
	zval runtime;

	if (zend_get_constant(ZEND_STRL("YOD_RUNTIME"), &runtime TSRMLS_CC)) {
		convert_to_double(&runtime);
		YOD_G(runtime) = Z_DVAL(runtime);
	} else {
		zend_register_double_constant(ZEND_STRS("YOD_RUNTIME"), YOD_G(runtime), CONST_CS, 0 TSRMLS_CC);
	}

	return YOD_G(runtime);
}
/* }}} */

/** {{{ long yod_forward(TSRMLS_D)
*/
long yod_forward(TSRMLS_D) {
	zval forward;
	long retval;

	if (zend_get_constant(ZEND_STRL("YOD_FORWARD"), &forward TSRMLS_CC)) {
		convert_to_long(&forward);
		retval = Z_LVAL(forward);
	} else {
		retval = YOD_FORWARD;
		zend_register_long_constant(ZEND_STRS("YOD_FORWARD"), retval, CONST_CS, 0 TSRMLS_CC);
	}

	return retval;
}
/* }}} */

/** {{{ long yod_runmode(TSRMLS_D)
*/
long yod_runmode(TSRMLS_D) {
	zval runmode;

	if (!YOD_G(runmode)) {
		if (zend_get_constant(ZEND_STRL("YOD_RUNMODE"), &runmode TSRMLS_CC)) {
			convert_to_long(&runmode);
			YOD_G(runmode) = Z_LVAL(runmode);
		} else {
			YOD_G(runmode) = YOD_RUNMODE;
			zend_register_long_constant(ZEND_STRS("YOD_RUNMODE"), YOD_G(runmode), CONST_CS, 0 TSRMLS_CC);
		}
	}

	return YOD_G(runmode);
}
/* }}} */

/** {{{ char *yod_charset(TSRMLS_D)
*/
char *yod_charset(TSRMLS_D) {
	zval charset;

	if (!YOD_G(charset)) {
		if (zend_get_constant(ZEND_STRL("YOD_CHARSET"), &charset TSRMLS_CC)) {
			convert_to_string(&charset);
			YOD_G(charset) = Z_STRVAL(charset);
		} else {
			INIT_ZVAL(charset);
			ZVAL_STRINGL(&charset, YOD_CHARSET, sizeof(YOD_CHARSET)-1, 1);
			YOD_G(charset) = estrndup(YOD_CHARSET, sizeof(YOD_CHARSET)-1);
			zend_register_string_constant(ZEND_STRS("YOD_CHARSET"), Z_STRVAL(charset), CONST_CS, 0 TSRMLS_CC);
		}
	}

	return YOD_G(charset);
}
/* }}} */

/** {{{ char *yod_viewext(TSRMLS_D)
*/
char *yod_viewext(TSRMLS_D) {
	zval viewext;

	if (!YOD_G(viewext)) {
		if (zend_get_constant(ZEND_STRL("YOD_VIEWEXT"), &viewext TSRMLS_CC)) {
			convert_to_string(&viewext);
			YOD_G(viewext) = Z_STRVAL(viewext);
		} else {
			INIT_ZVAL(viewext);
			ZVAL_STRINGL(&viewext, YOD_VIEWEXT, sizeof(YOD_VIEWEXT)-1, 1);
			YOD_G(viewext) = estrndup(YOD_VIEWEXT, sizeof(YOD_VIEWEXT)-1);
			zend_register_string_constant(ZEND_STRS("YOD_VIEWEXT"), Z_STRVAL(viewext), CONST_CS, 0 TSRMLS_CC);
		}
	}

	return YOD_G(viewext);
}
/* }}} */

/** {{{ char *yod_pathvar(TSRMLS_D)
*/
char *yod_pathvar(TSRMLS_D) {
	zval pathvar;

	if (!YOD_G(pathvar)) {
		if (zend_get_constant(ZEND_STRL("YOD_PATHVAR"), &pathvar TSRMLS_CC)) {
			convert_to_string(&pathvar);
			YOD_G(pathvar) = Z_STRVAL(pathvar);
		} else {
			INIT_ZVAL(pathvar);
			ZVAL_STRINGL(&pathvar, YOD_PATHVAR, sizeof(YOD_PATHVAR)-1, 1);
			YOD_G(pathvar) = estrndup(YOD_PATHVAR, sizeof(YOD_PATHVAR)-1);
			zend_register_string_constant(ZEND_STRS("YOD_PATHVAR"), Z_STRVAL(pathvar), CONST_CS, 0 TSRMLS_CC);
		}

#if PHP_YOD_DEBUG
		yod_debugf("yod_pathvar():%s", YOD_G(pathvar));
#endif
	}

	return YOD_G(pathvar);
}
/* }}} */

/** {{{ char *yod_runpath(TSRMLS_D)
*/
char *yod_runpath(TSRMLS_D) {
	zval runpath, **ppval;
	uint runpath_len;
	HashTable *_SERVER;

	if (!YOD_G(runpath)) {
		if (zend_get_constant(ZEND_STRL("YOD_RUNPATH"), &runpath TSRMLS_CC)) {
			convert_to_string(&runpath);
			YOD_G(runpath) = Z_STRVAL(runpath);
		} else {
			INIT_ZVAL(runpath);
			if (!PG(http_globals)[TRACK_VARS_SERVER]) {
				zend_is_auto_global(ZEND_STRL("_SERVER") TSRMLS_CC);
			}
			_SERVER = HASH_OF(PG(http_globals)[TRACK_VARS_SERVER]);
			if (zend_hash_find(_SERVER, ZEND_STRS("SCRIPT_FILENAME"), (void **) &ppval) != FAILURE &&
				Z_TYPE_PP(ppval) == IS_STRING
			) {
				runpath_len = Z_STRLEN_PP(ppval);
				YOD_G(runpath) = estrndup(Z_STRVAL_PP(ppval), runpath_len);
			} else {
				runpath_len = strlen(SG(request_info).path_translated);
				YOD_G(runpath) = estrndup(SG(request_info).path_translated, runpath_len);
			}
			runpath_len = php_dirname(YOD_G(runpath), runpath_len);
			ZVAL_STRINGL(&runpath, YOD_G(runpath), runpath_len, 1);
			zend_register_stringl_constant(ZEND_STRS("YOD_RUNPATH"), Z_STRVAL(runpath), runpath_len, CONST_CS, 0 TSRMLS_CC);
		}

#if PHP_YOD_DEBUG
		yod_debugf("yod_runpath():%s", YOD_G(runpath));
#endif
	}

	return YOD_G(runpath);
}
/* }}} */

/** {{{ char *yod_extpath(TSRMLS_D)
*/
char *yod_extpath(TSRMLS_D) {
	zval extpath, **ppval;
	uint extpath_len;
	HashTable *_SERVER;

	if (!YOD_G(extpath)) {
		if (zend_get_constant(ZEND_STRL("YOD_EXTPATH"), &extpath TSRMLS_CC)) {
			convert_to_string(&extpath);
			YOD_G(extpath) = Z_STRVAL(extpath);
		} else {
			INIT_ZVAL(extpath);
			if (!PG(http_globals)[TRACK_VARS_SERVER]) {
				zend_is_auto_global(ZEND_STRL("_SERVER") TSRMLS_CC);
			}
			_SERVER = HASH_OF(PG(http_globals)[TRACK_VARS_SERVER]);
			if (zend_hash_find(_SERVER, ZEND_STRS("SCRIPT_FILENAME"), (void **) &ppval) != FAILURE &&
				Z_TYPE_PP(ppval) == IS_STRING
			) {
				extpath_len = Z_STRLEN_PP(ppval);
				YOD_G(extpath) = estrndup(Z_STRVAL_PP(ppval), extpath_len);
			} else {
				extpath_len = strlen(SG(request_info).path_translated);
				YOD_G(extpath) = estrndup(SG(request_info).path_translated, extpath_len);
			}
			extpath_len = php_dirname(YOD_G(extpath), extpath_len);
			ZVAL_STRINGL(&extpath, YOD_G(extpath), extpath_len, 1);
			zend_register_stringl_constant(ZEND_STRS("YOD_EXTPATH"), Z_STRVAL(extpath), extpath_len, CONST_CS, 0 TSRMLS_CC);
		}
	
#if PHP_YOD_DEBUG
		yod_debugf("yod_extpath():%s", YOD_G(extpath));
#endif
	}

	return YOD_G(extpath);
}
/* }}} */

/** {{{ char *yod_logpath(TSRMLS_D)
*/
char *yod_logpath(TSRMLS_D) {
	zval logpath;

	if (!YOD_G(logpath)) {
		if (zend_get_constant(ZEND_STRL("YOD_LOGPATH"), &logpath TSRMLS_CC)) {
			convert_to_string(&logpath);
			YOD_G(logpath) = Z_STRVAL(logpath);
		}
	
#if PHP_YOD_DEBUG
		yod_debugf("yod_logpath():%s", YOD_G(logpath) ? YOD_G(logpath) : "");
#endif
	}

	return YOD_G(logpath);
}
/* }}} */

/** {{{ void yod_loading(TSRMLS_D)
*/
void yod_loading(TSRMLS_D) {
	if (YOD_G(loading)) {
		return;
	}
	YOD_G(loading) = 1;

	yod_runtime(TSRMLS_C);
	yod_forward(TSRMLS_C);
	yod_runmode(TSRMLS_C);
	yod_charset(TSRMLS_C);
	yod_viewext(TSRMLS_C);
	yod_pathvar(TSRMLS_C);
	yod_runpath(TSRMLS_C);
	yod_extpath(TSRMLS_C);
}
/* }}} */

/** {{{ int yod_include(char *filepath, zval **retval, int dtor TSRMLS_DC)
*/
int yod_include(char *filepath, zval **retval, int dtor TSRMLS_DC) {
	zend_file_handle file_handle;
	zend_op_array 	*op_array;
	char realpath[MAXPATHLEN];
	zval *pzval = NULL;

	if (!VCWD_REALPATH(filepath, realpath)) {
		return 0;
	}

#if PHP_YOD_DEBUG
	yod_debugf("yod_include(%s)", filepath);
#endif

	file_handle.filename = filepath;
	file_handle.free_filename = 0;
	file_handle.type = ZEND_HANDLE_FILENAME;
	file_handle.opened_path = NULL;
	file_handle.handle.fp = NULL;

	op_array = zend_compile_file(&file_handle, ZEND_INCLUDE TSRMLS_CC);

	if (op_array && file_handle.handle.stream.handle) {
		int dummy = 1;

		if (!file_handle.opened_path) {
			file_handle.opened_path = filepath;
		}

		zend_hash_add(&EG(included_files), file_handle.opened_path, strlen(file_handle.opened_path) + 1, (void *)&dummy, sizeof(int), NULL);
	}
	zend_destroy_file_handle(&file_handle TSRMLS_CC);

	if (op_array) {
		YOD_STORE_EG_ENVIRON();

		EG(return_value_ptr_ptr) = retval ? retval : &pzval;
		EG(active_op_array) 	 = op_array;

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2)) || (PHP_MAJOR_VERSION > 5)
		if (!EG(active_symbol_table)) {
			zend_rebuild_symbol_table(TSRMLS_C);
		}
#endif
		zend_try {
			zend_execute(op_array TSRMLS_CC);

			destroy_op_array(op_array TSRMLS_CC);
			efree(op_array);
			
			if (!EG(exception) && dtor) {
				if (EG(return_value_ptr_ptr) && *EG(return_value_ptr_ptr)) {
					zval_ptr_dtor(EG(return_value_ptr_ptr));
				}
			}
		} zend_end_try();

		YOD_RESTORE_EG_ENVIRON();

	    return 1;
	}
	return 0;
}
/* }}} */

/* {{{ yod_functions[]
*/
zend_function_entry yod_functions[] = {
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ PHP_GINIT_FUNCTION
*/
PHP_GINIT_FUNCTION(yod)
{
	yod_globals->runtime	= 0;
	yod_globals->forward	= 0;
	yod_globals->runmode	= 0;
	yod_globals->charset	= NULL;
	yod_globals->viewext	= NULL;
	yod_globals->pathvar	= NULL;
	yod_globals->runpath	= NULL;
	yod_globals->extpath	= NULL;
	yod_globals->logpath	= NULL;
	yod_globals->yodapp		= NULL;
	yod_globals->config		= NULL;
	yod_globals->exited		= 0;
	yod_globals->running	= 0;
	yod_globals->loading	= 0;
	yod_globals->imports	= NULL;

#if PHP_YOD_DEBUG
	yod_globals->debugs		= NULL;
#endif
}
/* }}} */

/** {{{ PHP_MINIT_FUNCTION
*/
PHP_MINIT_FUNCTION(yod)
{
	REGISTER_STRINGL_CONSTANT("YOD_VERSION", YOD_VERSION, sizeof(YOD_VERSION) - 1, CONST_PERSISTENT | CONST_CS);

	/* startup class */
	PHP_MINIT(yod_application)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(yod_request)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(yod_controller)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(yod_action)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(yod_widget)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(yod_model)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(yod_dbmodel)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(yod_database)(INIT_FUNC_ARGS_PASSTHRU);
	PHP_MINIT(yod_dbpdo)(INIT_FUNC_ARGS_PASSTHRU);

	return SUCCESS;
}
/* }}} */

/** {{{ PHP_MSHUTDOWN_FUNCTION
*/
PHP_MSHUTDOWN_FUNCTION(yod)
{
	return SUCCESS;
}
/* }}} */

/** {{{ PHP_RINIT_FUNCTION
*/
PHP_RINIT_FUNCTION(yod)
{
	struct timeval tp = {0};

	// runtime
	if (gettimeofday(&tp, NULL)) {
		YOD_G(runtime)		= 0;	
	} else {
		YOD_G(runtime)		= (double)(tp.tv_sec + tp.tv_usec / MICRO_IN_SEC);
	}
	REGISTER_DOUBLE_CONSTANT("YOD_RUNTIME", YOD_G(runtime), CONST_CS);

	YOD_G(forward)			= 0;
	YOD_G(runmode)			= 0;
	YOD_G(charset)			= NULL;
	YOD_G(viewext)			= NULL;
	YOD_G(pathvar)			= NULL;
	YOD_G(runpath)			= NULL;
	YOD_G(extpath)			= NULL;
	YOD_G(logpath)			= NULL;
	YOD_G(yodapp)			= NULL;
	YOD_G(config)			= NULL;
	YOD_G(exited)			= 0;
	YOD_G(running)			= 0;
	YOD_G(loading)			= 0;
	
	MAKE_STD_ZVAL(YOD_G(imports));
	array_init(YOD_G(imports));

#if PHP_YOD_DEBUG
	MAKE_STD_ZVAL(YOD_G(debugs));
	array_init(YOD_G(debugs));
#endif

/*
	zval *params1;

	MAKE_STD_ZVAL(params1);
	array_init(params1);
	add_next_index_string(params1, YOD_APP_CNAME, 1);
	add_next_index_string(params1, "autorun", 1);
	zend_call_method_with_1_params(NULL, NULL, NULL, "register_shutdown_function", NULL, params1);
	zval_ptr_dtor(&params1);
*/

	// register
	yod_register("register_shutdown_function", "autorun" TSRMLS_CC);

	return SUCCESS;
}
/* }}} */

/** {{{ PHP_RSHUTDOWN_FUNCTION
*/
PHP_RSHUTDOWN_FUNCTION(yod)
{
	if (YOD_G(charset)) {
		efree(YOD_G(charset));
		YOD_G(charset) = NULL;
	}

	if (YOD_G(viewext)) {
		efree(YOD_G(viewext));
		YOD_G(viewext) = NULL;
	}
	
	if (YOD_G(pathvar)) {
		efree(YOD_G(pathvar));
		YOD_G(pathvar) = NULL;
	}

	if (YOD_G(runpath)) {
		efree(YOD_G(runpath));
		YOD_G(runpath) = NULL;
	}

	if (YOD_G(extpath)) {
		efree(YOD_G(extpath));
		YOD_G(extpath) = NULL;
	}

	if (YOD_G(logpath)) {
		efree(YOD_G(logpath));
		YOD_G(logpath) = NULL;
	}

	if (YOD_G(yodapp)) {
		zval_ptr_dtor(&YOD_G(yodapp));
		YOD_G(yodapp) = NULL;
	}

	if (YOD_G(config)) {
		zval_ptr_dtor(&YOD_G(config));
		YOD_G(config) = NULL;
	}

	if (YOD_G(imports)) {
		zval_ptr_dtor(&YOD_G(imports));
		YOD_G(imports) = NULL;
	}

#if PHP_YOD_DEBUG
	if (YOD_G(debugs)) {
		zval_ptr_dtor(&YOD_G(debugs));
		YOD_G(debugs) = NULL;
	}
#endif

	return SUCCESS;
}
/* }}} */

/** {{{ PHP_MINFO_FUNCTION
*/
PHP_MINFO_FUNCTION(yod)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "yod support", "enabled");
	php_info_print_table_row(2, "Version", YOD_VERSION);
	php_info_print_table_end();
}
/* }}} */

/** {{{ DL support
 */
#ifdef COMPILE_DL_YOD
ZEND_GET_MODULE(yod)
#endif
/* }}} */

/** {{{ module depends
 */
#if ZEND_MODULE_API_NO >= 20050922
zend_module_dep yod_deps[] = {
	ZEND_MOD_REQUIRED("spl")
	{NULL, NULL, NULL}
};
#endif
/* }}} */

/** {{{ yod_module_entry
*/
zend_module_entry yod_module_entry = {
#if ZEND_MODULE_API_NO >= 20050922
	STANDARD_MODULE_HEADER_EX, NULL,
	yod_deps,
#else
	STANDARD_MODULE_HEADER,
#endif
	"yod",
	yod_functions,
	PHP_MINIT(yod),
	PHP_MSHUTDOWN(yod),
	PHP_RINIT(yod),
	PHP_RSHUTDOWN(yod),
	PHP_MINFO(yod),
	YOD_VERSION,
	PHP_MODULE_GLOBALS(yod),
	PHP_GINIT(yod),
	NULL,
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
