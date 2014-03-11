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

#ifndef PHP_YOD_DEBUG_H
#define PHP_YOD_DEBUG_H

#if PHP_YOD_DEBUG
#define YOD_DOTLINE					"----------------------------------------------------------------------"
#define YOD_DIVLINE					"======================================================================"
#endif

#if PHP_YOD_DEBUG
void yod_debugf(const char *format,...);
void yod_debugl(int ltype TSRMLS_DC);
void yod_debugz(zval *pzval, int dump TSRMLS_DC);
void yod_debugs(TSRMLS_D);
#endif

#endif

/*
* Local variables:
* tab-width: 4
* c-basic-offset: 4
* End:
* vim600: noet sw=4 ts=4 fdm=marker
* vim<600: noet sw=4 ts=4
*/
