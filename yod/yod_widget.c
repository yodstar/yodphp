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
#include "Zend/zend_interfaces.h"

#include "php_yod.h"
#include "yod_application.h"
#include "yod_controller.h"
#include "yod_widget.h"

#if PHP_YOD_DEBUG
#include "yod_debug.h"
#endif

zend_class_entry *yod_widget_ce;

/** {{{ ARG_INFO
*/
ZEND_BEGIN_ARG_INFO_EX(yod_widget_construct_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, route)
	ZEND_ARG_INFO(0, action)
	ZEND_ARG_INFO(0, params)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_widget_run_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()
/* }}} */

/** {{{ static void yod_widget_construct(yod_widget_t *object, yod_request_t *request, char *action, uint action_len, zval *params TSRMLS_DC)
*/
static void yod_widget_construct(yod_widget_t *object, yod_request_t *request, char *action, uint action_len, zval *params TSRMLS_DC) {
	zval *p_action, *tpl_view, *tpl_data;
	char *cname, *name, *method, *tpl_path;
	uint cname_len, name_len, method_len;
	int dupl;

	if (!object) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Cannot instantiate abstract class Yod_Widget");
		return;
	}

#if PHP_YOD_DEBUG
	yod_debugf("yod_widget_construct()");
#endif

#if PHP_API_VERSION < 20100412
	dupl = zend_get_object_classname(object, &cname, &cname_len TSRMLS_CC);
#else
	dupl = zend_get_object_classname(object, (const char **)&cname, &cname_len TSRMLS_CC);
#endif

	if (cname_len > 6) {
		name = estrndup(cname, cname_len);
		*(name + cname_len - 6) = '\0';
		name_len = cname_len - 6;
		zend_str_tolower(name, name_len);
		zend_update_property_string(Z_OBJCE_P(object), object, ZEND_STRL("_name"), name TSRMLS_CC);
		efree(name);
	}
	
	if (action_len) {
		zend_str_tolower(action, action_len);
	} else {
		action = "index";
	}
	zend_update_property_string(Z_OBJCE_P(object), object, ZEND_STRL("_action"), action TSRMLS_CC);

	zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_request"), request TSRMLS_CC);

	MAKE_STD_ZVAL(tpl_data);
	yod_application_config(ZEND_STRL("tpl_data"), tpl_data TSRMLS_CC);
	if (!tpl_data || Z_TYPE_P(tpl_data) != IS_ARRAY) {
		array_init(tpl_data);
	}
	spprintf(&tpl_path, 0, "%s/widgets", yod_runpath(TSRMLS_C));
	MAKE_STD_ZVAL(tpl_view);
	array_init(tpl_view);
	add_assoc_zval(tpl_view, "tpl_data", tpl_data);
	add_assoc_string(tpl_view, "tpl_path", tpl_path, 1);
	add_assoc_string(tpl_view, "tpl_file", "", 1);
	zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_view"), tpl_view TSRMLS_CC);
	zval_ptr_dtor(&tpl_view);
	efree(tpl_path);

#if PHP_YOD_DEBUG
	yod_debugf("yod_widget_init()");
#endif

	if (zend_hash_exists(&(Z_OBJCE_P(object)->function_table), ZEND_STRS("init"))) {
		zend_call_method_with_0_params(&object, Z_OBJCE_P(object), NULL, "init", NULL);
	}

	p_action = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_action"), 1 TSRMLS_CC);
	if (p_action && Z_TYPE_P(p_action) == IS_STRING && Z_STRLEN_P(p_action)) {
		zend_str_tolower(Z_STRVAL_P(p_action), Z_STRLEN_P(p_action));
		action_len = spprintf(&action, 0, "%s", Z_STRVAL_P(p_action));
		method_len = spprintf(&method, 0, "%saction", Z_STRVAL_P(p_action));
	} else {
		action_len = spprintf(&action, 0, "index");
		method_len = spprintf(&method, 0, "%saction", action);
	}
	zend_update_property_string(Z_OBJCE_P(object), object, ZEND_STRL("_action"), action TSRMLS_CC);

	if (zend_hash_exists(&(Z_OBJCE_P(object)->function_table), method, method_len + 1)) {
		zend_call_method(&object, Z_OBJCE_P(object), NULL, method, method_len, NULL, 1, params, NULL TSRMLS_CC);
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unavailable action %s::%sAction()", cname, action);
	}
	efree(action);
	efree(method);

	if (!dupl) {
		efree(cname);
	}
}
/* }}} */

/** {{{ proto public Yod_Widget::__construct($request, $action = null, $params = null)
*/
PHP_METHOD(yod_widget, __construct) {
	yod_request_t *request = NULL;
	zval *params = NULL;
	char *action = NULL;
	uint action_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|sz!", &request, &action, &action_len, &params) == FAILURE) {
		return;
	}

	yod_widget_construct(getThis(), request, action, action_len, params TSRMLS_CC);

}
/* }}} */

/** {{{ proto protected Yod_Widget::run()
*/
PHP_METHOD(yod_widget, run) {
	
}
/* }}} */

/** {{{ yod_widget_methods[]
*/
zend_function_entry yod_widget_methods[] = {
	PHP_ME(yod_widget, __construct,		yod_widget_construct_arginfo,	ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(yod_widget, run,				yod_widget_run_arginfo,		ZEND_ACC_PROTECTED)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ PHP_MINIT_FUNCTION
*/
PHP_MINIT_FUNCTION(yod_widget) {
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "Yod_Widget", yod_widget_methods);
	yod_widget_ce = zend_register_internal_class_ex(&ce, yod_controller_ce, NULL TSRMLS_CC);
	yod_widget_ce->ce_flags |= ZEND_ACC_IMPLICIT_ABSTRACT_CLASS;
	
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
