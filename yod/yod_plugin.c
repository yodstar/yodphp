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

#include "php_yod.h"
#include "yod_plugin.h"

zend_class_entry *yod_plugin_ce;

/** {{{ yod_plugin_methods[]
*/
zend_function_entry yod_plugin_methods[] = {
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ PHP_MINIT_FUNCTION
*/
PHP_MINIT_FUNCTION(yod_plugin) {
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "Yod_Plugin", yod_plugin_methods);
	yod_plugin_ce = zend_register_internal_class(&ce TSRMLS_CC);
	yod_plugin_ce->ce_flags |= ZEND_ACC_IMPLICIT_ABSTRACT_CLASS;

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
