/*
 * php_screw
 * (C) 2007, Kunimasa Noda/PM9.com, Inc. <http://www.pm9.com,  kuni@pm9.com>
 * see file LICENSE for license details
 */

#ifndef PHP_YOD_CRYPT_H
#define PHP_YOD_CRYPT_H

#define YOD_CRYPT_SCREW        "<?php exit('Forbidden'); ?>\nYOD_CRYPT\n"
#define YOD_CRYPT_SCREW_LEN    38

#define YOD_CRYPT_ZBUF_SIZE    100000

#include <zlib.h>

char yod_crypt_zbuf[YOD_CRYPT_ZBUF_SIZE];
z_stream yod_crypt_z;

#if PHP_YOD_CRYPT

zend_op_array *yod_crypt_compile_file(zend_file_handle *file_handle, int type TSRMLS_DC);
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
