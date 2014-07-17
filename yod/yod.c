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
#include "php_ini.h"
#include "main/SAPI.h"
#include "Zend/zend_interfaces.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"

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
#include "yod_plugin.h"

#if PHP_YOD_DEBUG
#include "yod_debug.h"
#endif

#define MICRO_IN_SEC 1000000.00

zend_class_entry *yod_ce;

ZEND_DECLARE_MODULE_GLOBALS(yod);

/** {{{ ARG_INFO
*/
ZEND_BEGIN_ARG_INFO_EX(yod_app_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, config)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_config_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_import_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, alias)
	ZEND_ARG_INFO(0, classext)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_plugin_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, alias)
	ZEND_ARG_INFO(0, classext)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_model_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, config)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dmodel_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, config)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_db_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, config)
ZEND_END_ARG_INFO()
/* }}} */

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

/** {{{ zval* yod_call_method(zval **object_pp, zend_class_entry *obj_ce, zend_function **fn_proxy, char *function_name, int function_name_len, zval **retval_ptr_ptr, int param_count, zval* arg1, zval* arg2, zval* arg3, zval* arg4, zval* arg5 TSRMLS_DC)
*/
zval* yod_call_method(zval **object_pp, zend_class_entry *obj_ce, zend_function **fn_proxy, char *function_name, int function_name_len, zval **retval_ptr_ptr, int param_count, zval* arg1, zval* arg2, zval* arg3, zval* arg4, zval* arg5 TSRMLS_DC)
{
	int result;
	zend_fcall_info fci;
	zval z_fname;
	zval *retval;
	HashTable *function_table;

	zval **params[5];

	params[0] = &arg1;
	params[1] = &arg2;
	params[2] = &arg3;
	params[3] = &arg4;
	params[4] = &arg5;

#if PHP_YOD_DEBUG
	if (obj_ce) {
		yod_debugf("yod_call_method(%s, %s)", obj_ce->name, function_name ? function_name : "");
	} else {
		yod_debugf("yod_call_method(%s)", function_name ? function_name : "");
	}
#endif
	
	fci.size = sizeof(fci);
	/*fci.function_table = NULL; will be read form zend_class_entry of object if needed */
#if PHP_API_VERSION > 20041225
	fci.object_ptr = object_pp ? *object_pp : NULL;
#else
	fci.object_pp = object_pp ? object_pp : NULL;
#endif
	fci.function_name = &z_fname;
	fci.retval_ptr_ptr = retval_ptr_ptr ? retval_ptr_ptr : &retval;
	fci.param_count = param_count;
	fci.params = params;
	fci.no_separation = 1;
	fci.symbol_table = NULL;

	if (!fn_proxy && !obj_ce) {
		/* no interest in caching and no information already present that is
		 * needed later inside zend_call_function. */
		ZVAL_STRINGL(&z_fname, function_name, function_name_len, 0);
		fci.function_table = !object_pp ? EG(function_table) : NULL;
		result = zend_call_function(&fci, NULL TSRMLS_CC);
	} else {
		zend_fcall_info_cache fcic;

		fcic.initialized = 1;
		if (!obj_ce) {
			obj_ce = object_pp ? Z_OBJCE_PP(object_pp) : NULL;
		}
		if (obj_ce) {
			function_table = &obj_ce->function_table;
		} else {
			function_table = EG(function_table);
		}
		if (!fn_proxy || !*fn_proxy) {
			if (zend_hash_find(function_table, function_name, function_name_len+1, (void **) &fcic.function_handler) == FAILURE) {
				/* error at c-level */
				zend_error(E_CORE_ERROR, "Couldn't find implementation for method %s%s%s", obj_ce ? obj_ce->name : "", obj_ce ? "::" : "", function_name);
			}
			if (fn_proxy) {
				*fn_proxy = fcic.function_handler;
			}
		} else {
			fcic.function_handler = *fn_proxy;
		}
		fcic.calling_scope = obj_ce;
#if PHP_API_VERSION > 20041225
		if (object_pp) {
			fcic.called_scope = Z_OBJCE_PP(object_pp);
		} else if (obj_ce &&
		           !(EG(called_scope) &&
		             instanceof_function(EG(called_scope), obj_ce TSRMLS_CC))) {
			fcic.called_scope = obj_ce;
		} else {
			fcic.called_scope = EG(called_scope);
		}
		fcic.object_ptr = object_pp ? *object_pp : NULL;
#else
		fcic.object_pp = object_pp ? object_pp : NULL;
#endif
		result = zend_call_function(&fci, &fcic TSRMLS_CC);
	}
	if (result == FAILURE) {
		/* error at c-level */
		if (!obj_ce) {
			obj_ce = object_pp ? Z_OBJCE_PP(object_pp) : NULL;
		}
		if (!EG(exception)) {
			zend_error(E_CORE_ERROR, "Couldn't execute method %s%s%s", obj_ce ? obj_ce->name : "", obj_ce ? "::" : "", function_name);
		}
	}
	if (!retval_ptr_ptr) {
		if (retval) {
			zval_ptr_dtor(&retval);
		}
		return NULL;
	}
	return *retval_ptr_ptr;
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
		zval_dtor(&runtime);
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
		zval_dtor(&forward);
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
			zval_dtor(&runmode);
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
			YOD_G(charset) = estrndup(Z_STRVAL(charset), Z_STRLEN(charset));
			zval_dtor(&charset);
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
			YOD_G(viewext) = estrndup(Z_STRVAL(viewext), Z_STRLEN(viewext));
			zval_dtor(&viewext);
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
			YOD_G(pathvar) = estrndup(Z_STRVAL(pathvar), Z_STRLEN(pathvar));
			zval_dtor(&pathvar);
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

/** {{{ char *yod_runfile(TSRMLS_D)
*/
char *yod_runfile(TSRMLS_D) {
	zval **ppval;
	uint runfile_len;
	HashTable *_SERVER;

	if (!YOD_G(runfile)) {
		if (!PG(http_globals)[TRACK_VARS_SERVER]) {
			zend_is_auto_global(ZEND_STRL("_SERVER") TSRMLS_CC);
		}
		_SERVER = HASH_OF(PG(http_globals)[TRACK_VARS_SERVER]);
		if (zend_hash_find(_SERVER, ZEND_STRS("SCRIPT_FILENAME"), (void **) &ppval) != FAILURE &&
			Z_TYPE_PP(ppval) == IS_STRING
		) {
			runfile_len = Z_STRLEN_PP(ppval);
			YOD_G(runfile) = estrndup(Z_STRVAL_PP(ppval), runfile_len);
		} else {
			runfile_len = strlen(SG(request_info).path_translated);
			YOD_G(runfile) = estrndup(SG(request_info).path_translated, runfile_len);
		}
	}

	return YOD_G(runfile);
}
/* }}} */

/** {{{ char *yod_runpath(TSRMLS_D)
*/
char *yod_runpath(TSRMLS_D) {
	zval runpath;
	uint runpath_len;
	char *runfile;

	if (!YOD_G(runpath)) {
		if (zend_get_constant(ZEND_STRL("YOD_RUNPATH"), &runpath TSRMLS_CC)) {
			convert_to_string(&runpath);
			YOD_G(runpath) = estrndup(Z_STRVAL(runpath), Z_STRLEN(runpath));
			zval_dtor(&runpath);
		} else {
			INIT_ZVAL(runpath);
			runfile = yod_runfile(TSRMLS_C);
			runpath_len = strlen(runfile);
			YOD_G(runpath) = estrndup(runfile, runpath_len);
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
	zval extpath;
	uint extpath_len;
	char *runfile;

	if (!YOD_G(extpath)) {
		if (zend_get_constant(ZEND_STRL("YOD_EXTPATH"), &extpath TSRMLS_CC)) {
			convert_to_string(&extpath);
			YOD_G(extpath) = estrndup(Z_STRVAL(extpath), Z_STRLEN(extpath));
			zval_dtor(&extpath);
		} else {
			INIT_ZVAL(extpath);
			runfile = yod_runfile(TSRMLS_C);
			extpath_len = strlen(runfile);
			YOD_G(extpath) = estrndup(runfile, extpath_len);
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
			YOD_G(logpath) = estrndup(Z_STRVAL(logpath), Z_STRLEN(logpath));
			zval_dtor(&logpath);
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
	yod_logpath(TSRMLS_C);
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

zend_op_array *(*yod_orig_compile_file)(zend_file_handle *file_handle, int type TSRMLS_DC);

/** {{{ zend_op_array *yod_init_compile_file(zend_file_handle *file_handle, int type TSRMLS_DC)
*/
static zend_op_array *yod_init_compile_file(zend_file_handle *file_handle, int type TSRMLS_DC) /* {{{ */
{
	zval *retval = NULL;
	zend_op_array *op_array;
	char *runfile;
	
	if (!file_handle || !file_handle->filename) {
		return yod_orig_compile_file(file_handle, type TSRMLS_CC);
	}

	runfile = yod_runfile(TSRMLS_C);
	if (strcmp(file_handle->filename, runfile)) {
		return yod_orig_compile_file(file_handle, type TSRMLS_CC);
	}

	zend_compile_file = yod_orig_compile_file;

	op_array = zend_compile_file(file_handle, type TSRMLS_CC);

	if (op_array) {
		EG(return_value_ptr_ptr) = &retval;
		EG(active_op_array) 	 = op_array;

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2)) || (PHP_MAJOR_VERSION > 5)
		if (!EG(active_symbol_table)) {
			zend_rebuild_symbol_table(TSRMLS_C);
		}
#endif
		zend_execute(op_array TSRMLS_CC);

		destroy_op_array(op_array TSRMLS_CC);
		efree(op_array);
		if (!EG(exception)) {
			if (EG(return_value_ptr_ptr)) {
				zval_ptr_dtor(EG(return_value_ptr_ptr));
			}
		}
	}

	yod_application_autorun(TSRMLS_C);

	return NULL;
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
	yod_globals->running	= 0;
	yod_globals->imports	= NULL;
	yod_globals->plugins	= NULL;

	yod_globals->exited		= 0;
	yod_globals->loading	= 0;
	yod_globals->runfile	= NULL;

#if PHP_YOD_DEBUG
	yod_globals->debugs		= NULL;
#endif
}
/* }}} */

/** {{{ proto public Yod::app($config = null)
*/
PHP_METHOD(yod, app) {
	zval *config = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z!", &config) == FAILURE) {
		return;
	}

	yod_application_app(config TSRMLS_CC);

	RETURN_ZVAL(YOD_G(yodapp), 1, 0);
}
/* }}} */

/** {{{ proto public Yod::config($name = null)
*/
PHP_METHOD(yod, config) {
	char *name = NULL;
	uint name_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &name, &name_len) == FAILURE) {
		return;
	}

	yod_application_config(name, name_len, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod::import($alias, $classext = '.class.php')
*/
PHP_METHOD(yod, import) {
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

/** {{{ proto public Yod::plugin($alias, $classext = '.class.php')
*/
PHP_METHOD(yod, plugin) {
	char *alias = NULL, *classext = NULL;
	uint alias_len = 0, classext_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s", &alias, &alias_len, &classext, &classext_len) == FAILURE) {
		return;
	}

	yod_application_plugin(alias, alias_len, classext, classext_len, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ proto protected Yod::model($name = '', $config = '')
*/
PHP_METHOD(yod, model) {
	zval *config = NULL;
	char *name = NULL;
	uint name_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sz!", &name, &name_len, &config) == FAILURE) {
		return;
	}

	yod_model_getinstance(name, name_len, config, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ proto protected Yod::dmodel($name = '', $config = '')
*/
PHP_METHOD(yod, dmodel) {
	zval *config = NULL;
	char *name = NULL;
	uint name_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sz!", &name, &name_len, &config) == FAILURE) {
		return;
	}

	yod_dbmodel_getinstance(name, name_len, config, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod::db($config = 'db_dsn')
*/
PHP_METHOD(yod, db) {
	zval *config = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z!", &config) == FAILURE) {
		return;
	}

	yod_database_getinstance(config, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ yod_methods[]
*/
zend_function_entry yod_methods[] = {
	PHP_ME(yod, app,        yod_app_arginfo,      ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod, config,     yod_config_arginfo,   ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod, import,     yod_import_arginfo,   ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod, plugin,     yod_plugin_arginfo,   ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod, model,      yod_model_arginfo,    ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod, dmodel,     yod_dmodel_arginfo,   ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod, db,         yod_db_arginfo,       ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ PHP_MINIT_FUNCTION
*/
PHP_MINIT_FUNCTION(yod)
{
	zend_class_entry ce;

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
	PHP_MINIT(yod_plugin)(INIT_FUNC_ARGS_PASSTHRU);

	INIT_CLASS_ENTRY(ce, "Yod", yod_methods);
	yod_ce = zend_register_internal_class(&ce TSRMLS_CC);
	yod_ce->ce_flags |= ZEND_ACC_IMPLICIT_ABSTRACT_CLASS;

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

	yod_orig_compile_file = zend_compile_file;
	zend_compile_file = yod_init_compile_file;

	/* runtime */
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
	YOD_G(running)			= 0;
	
	MAKE_STD_ZVAL(YOD_G(imports));
	array_init(YOD_G(imports));

	MAKE_STD_ZVAL(YOD_G(plugins));
	array_init(YOD_G(plugins));

	YOD_G(exited)			= 0;
	YOD_G(loading)			= 0;
	YOD_G(runfile)			= NULL;

#if PHP_YOD_DEBUG
	MAKE_STD_ZVAL(YOD_G(debugs));
	array_init(YOD_G(debugs));
#endif

	return SUCCESS;
}
/* }}} */

/** {{{ PHP_RSHUTDOWN_FUNCTION
*/
PHP_RSHUTDOWN_FUNCTION(yod)
{
	if (zend_compile_file == yod_init_compile_file) {
		zend_compile_file = yod_orig_compile_file;
	}

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

	if (YOD_G(plugins)) {
		zval_ptr_dtor(&YOD_G(plugins));
		YOD_G(plugins) = NULL;
	}

	if (YOD_G(runfile)) {
		efree(YOD_G(runfile));
		YOD_G(runfile) = NULL;
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
