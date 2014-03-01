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

#ifndef PHP_YOD_REQUEST_H
#define PHP_YOD_REQUEST_H

#define YOD_ERR404_HTML	"<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n\
<html><head><title>404 Not Found</title>\n\
</head><body>\n\
<h1>Not Found</h1>\n\
<p>The requested URL %s was not found on this server.</p>\n\
</body></html>"

extern zend_class_entry *yod_request_ce;

zval *yod_request_construct(zval *object, char *route, uint route_len TSRMLS_DC);
int yod_request_dispatch(yod_request_t *object TSRMLS_DC);
void yod_request_error404(yod_request_t *object, zval *html TSRMLS_DC);
void yod_request_erroraction(yod_request_t *object TSRMLS_DC);

PHP_MINIT_FUNCTION(yod_request);

#endif
/*
* Local variables:
* tab-width: 4
* c-basic-offset: 4
* End:
* vim600: noet sw=4 ts=4 fdm=marker
* vim<600: noet sw=4 ts=4
*/
