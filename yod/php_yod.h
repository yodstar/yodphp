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

#ifndef PHP_YOD_H
#define PHP_YOD_H

extern zend_module_entry yod_module_entry;
#define phpext_yod_ptr &yod_module_entry

#ifdef PHP_WIN32
#define PHP_YOD_API __declspec(dllexport)
#ifndef _MSC_VER
#define _MSC_VER 1600
#endif
#else
#define PHP_YOD_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#ifdef ZTS
#define YOD_G(v) TSRMG(yod_globals_id, zend_yod_globals *, v)
#else
#define YOD_G(v) (yod_globals.v)
#endif

#ifdef PHP_WIN32
#undef strcasecmp
#undef strncasecmp
#undef strdup
#define strcasecmp(s1, s2) _stricmp(s1, s2)
#define strncasecmp(s1, s2, n) _strnicmp(s1, s2, n)
#define strdup _strdup
#endif

#if PHP_YOD_DEBUG
#define YOD_VERSION					"1.3.2-dev"
#define YOD_RUNMODE					7
#else
#define YOD_VERSION					"1.3.2"
#define YOD_RUNMODE					3
#endif

#define YOD_FORWARD					5
#define YOD_CHARSET					"utf-8"
#define YOD_VIEWEXT					".php"
#define YOD_PATHVAR					""

#define YOD_APP_CNAME				"Yod_Application"

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION == 2)) || (PHP_MAJOR_VERSION > 5)
#define Z_SET_REFCOUNT_P(pz, rc)	  (pz)->refcount = rc
#define Z_SET_REFCOUNT_PP(ppz, rc)	  Z_SET_REFCOUNT_P(*(ppz), rc)
#define Z_ADDREF_P	 ZVAL_ADDREF
#define Z_REFCOUNT_P ZVAL_REFCOUNT
#define Z_DELREF_P	 ZVAL_DELREF
#endif

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2)) || (PHP_MAJOR_VERSION > 5)
#define YOD_STORE_EG_ENVIRON() \
	{ \
		zval ** orig_return_value_pp	= EG(return_value_ptr_ptr); \
		zend_op ** orig_opline_ptr		= EG(opline_ptr); \
		zend_op_array * orig_op_array	= EG(active_op_array);
#define YOD_RESTORE_EG_ENVIRON() \
		EG(return_value_ptr_ptr)	= orig_return_value_pp; \
		EG(opline_ptr)				= orig_opline_ptr; \
		EG(active_op_array)			= orig_op_array; \
	}
#else
#define YOD_STORE_EG_ENVIRON() \
	{ \
		zval ** orig_return_value_pp			= EG(return_value_ptr_ptr); \
		zend_op ** orig_opline_ptr				= EG(opline_ptr); \
		zend_op_array * orig_op_array			= EG(active_op_array); \
		zend_function_state * orig_func_state	= EG(function_state_ptr);
#define YOD_RESTORE_EG_ENVIRON() \
		EG(return_value_ptr_ptr)	= orig_return_value_pp; \
		EG(opline_ptr)				= orig_opline_ptr; \
		EG(active_op_array)			= orig_op_array; \
		EG(function_state_ptr)		= orig_func_state; \
	}
#endif

#define yod_application_t	zval
#define yod_request_t		zval
#define yod_controller_t	zval
#define yod_action_t		zval
#define yod_widget_t		zval
#define yod_model_t			zval
#define yod_dbmodel_t		zval
#define yod_database_t		zval
#define yod_dbpdo_t			zval

long yod_runmode(TSRMLS_D);
long yod_forward(TSRMLS_D);
char *yod_charset(TSRMLS_D);
char *yod_viewext(TSRMLS_D);
char *yod_pathvar(TSRMLS_D);
char *yod_runfile(TSRMLS_D);
char *yod_runpath(TSRMLS_D);
char *yod_extpath(TSRMLS_D);
char *yod_logpath(TSRMLS_D);
void yod_loading(TSRMLS_D);

int yod_do_exit(long status TSRMLS_DC);
int yod_register(char *moduel, char *method TSRMLS_DC);
int yod_include(char *filepath, zval **retval, int dtor TSRMLS_DC);

zval* yod_call_method(zval **object_pp, zend_class_entry *obj_ce, zend_function **fn_proxy, char *function_name, int function_name_len, zval **retval_ptr_ptr, int param_count, zval* arg1, zval* arg2, zval* arg3, zval* arg4, zval* arg5 TSRMLS_DC);

#define yod_call_method_with_3_params(obj, obj_ce, fn_proxy, function_name, retval, arg1, arg2, arg3) \
	yod_call_method(obj, obj_ce, fn_proxy, function_name, sizeof(function_name)-1, retval, 3, arg1, arg2, arg3, NULL, NULL TSRMLS_CC)

#define yod_call_method_with_4_params(obj, obj_ce, fn_proxy, function_name, retval, arg1, arg2, arg3, arg4) \
	yod_call_method(obj, obj_ce, fn_proxy, function_name, sizeof(function_name)-1, retval, 4, arg1, arg2, arg3, arg4, NULL TSRMLS_CC)

#define yod_call_method_with_5_params(obj, obj_ce, fn_proxy, function_name, retval, arg1, arg2, arg3, arg4, arg5) \
	yod_call_method(obj, obj_ce, fn_proxy, function_name, sizeof(function_name)-1, retval, 5, arg1, arg2, arg3, arg4, arg5 TSRMLS_CC)

ZEND_BEGIN_MODULE_GLOBALS(yod)
	double		runtime;
	long		forward;
	long		runmode;
	char		*charset;
	char		*viewext;
	char		*pathvar;
	char		*runpath;
	char		*extpath;
	char		*logpath;

	zval		*yodapp;
	zval		*config;
	int			running;
	zval		*imports;
	zval		*plugins;

	int			exited;
	int			loading;
	char		*runfile;

#if PHP_YOD_DEBUG
	zval		*debugs;
#endif
ZEND_END_MODULE_GLOBALS(yod)

PHP_MINIT_FUNCTION(yod);
PHP_MSHUTDOWN_FUNCTION(yod);
PHP_RINIT_FUNCTION(yod);
PHP_RSHUTDOWN_FUNCTION(yod);
PHP_MINFO_FUNCTION(yod);

extern ZEND_DECLARE_MODULE_GLOBALS(yod);

extern zend_class_entry *yod_ce;

#endif
/*
* Local variables:
* tab-width: 4
* c-basic-offset: 4
* End:
* vim600: noet sw=4 ts=4 fdm=marker
* vim<600: noet sw=4 ts=4
*/
