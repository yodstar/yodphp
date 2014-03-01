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

#if PHP_YOD_DEBUG

#include "main/SAPI.h"
#include "ext/standard/file.h"
#include "ext/standard/flock_compat.h"
#include "ext/standard/php_filestat.h"
#include "ext/standard/php_var.h"

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
#include "yod_debug.h"

#define MICRO_IN_SEC 1000000.00

/** {{{ void yod_debugf(const char *format,...)
*/
void yod_debugf(const char *format,...) {
	struct timeval tp = {0};
	va_list args;
	char *buffer, *buffer1;
	size_t mem_usage;

	struct tm *ta, tmbuf;
	time_t curtime;
	char *datetime, asctimebuf[52];
	size_t datetime_len;
	
	TSRMLS_FETCH();
	
	if ((yod_runmode(TSRMLS_C) & 12) == 0) {
		return;
	}

	time(&curtime);
	ta = php_localtime_r(&curtime, &tmbuf);
	datetime = php_asctime_r(ta, asctimebuf);
	datetime_len = strlen(datetime);
	datetime[datetime_len - 1] = 0;

	if (!gettimeofday(&tp, NULL)) {
		va_start(args, format);
		vspprintf(&buffer1, 0, format, args);
		va_end(args);

		mem_usage = zend_memory_usage(1 TSRMLS_CC) / 1024;
		spprintf(&buffer, 0, "[%s %06d] (%dk) %s\n", datetime, tp.tv_usec, mem_usage, buffer1);

		add_next_index_string(YOD_G(debugs), buffer, 1);

		efree(buffer1);
		efree(buffer);
	}
}
/* }}} */

/** {{{ void yod_debugl(int ltype TSRMLS_DC)
*/
void yod_debugl(int ltype TSRMLS_DC) {
	char *buffer;
	uint buffer_len;

	if ((yod_runmode(TSRMLS_C) & 12) == 0) {
		return;
	}

	switch (ltype) {
		case 1 :
			buffer_len = spprintf(&buffer, 0, "%s\n", YOD_DOTLINE);
			break;
		case 2 :
			buffer_len = spprintf(&buffer, 0, "%s\n", YOD_DIVLINE);
			break;
		default :
			buffer_len = spprintf(&buffer, 0, "%s\n", YOD_DOTLINE);
	}

	add_next_index_string(YOD_G(debugs), buffer, 1);
	efree(buffer);
}
/* }}} */

/** {{{ void yod_debugz(zval *pzval, int dump TSRMLS_DC) {
*/
void yod_debugz(zval *pzval, int dump TSRMLS_DC) {
	zval *ob_buffer;
	int ob_start;

	if ((yod_runmode(TSRMLS_C) & 12) == 0) {
		return;
	}

#ifdef PHP_OUTPUT_NEWAPI
	ob_start = php_output_start_user(NULL, 0, PHP_OUTPUT_HANDLER_STDFLAGS TSRMLS_CC);
#else
	ob_start = php_start_ob_buffer(NULL, 0, 1 TSRMLS_CC);
#endif

	php_printf("%s\n", YOD_DOTLINE);
	if (dump) {
		php_var_dump(&pzval, 0 TSRMLS_CC);
	} else {
		zend_print_zval_r(pzval, 0 TSRMLS_CC);
	}
	php_printf("\n%s\n", YOD_DOTLINE);

	if (ob_start == SUCCESS) {
		MAKE_STD_ZVAL(ob_buffer);

#ifdef PHP_OUTPUT_NEWAPI
		if (php_output_get_contents(ob_buffer TSRMLS_CC) == SUCCESS) {
#else
		if (php_ob_get_buffer(ob_buffer TSRMLS_CC) == SUCCESS) {
#endif
			if (ob_buffer && Z_TYPE_P(ob_buffer) == IS_STRING) {
				add_next_index_stringl(YOD_G(debugs), Z_STRVAL_P(ob_buffer), Z_STRLEN_P(ob_buffer), 1);
			}
		}
		zval_ptr_dtor(&ob_buffer);

#ifdef PHP_OUTPUT_NEWAPI
		php_output_discard(TSRMLS_C);
#else
		if (OG(ob_nesting_level)) {
			php_end_ob_buffer(0, 0 TSRMLS_CC);
		}
#endif
	}
}
/* }}} */

/** {{{ int yod_debugw(char *data, uint data_len TSRMLS_DC)
*/
int yod_debugw(char *data, uint data_len TSRMLS_DC) {
	zval *zcontext = NULL;
	php_stream_statbuf ssb;
	php_stream_context *context;
	php_stream *stream;
	
	char *logpath, *logfile;

	logpath = yod_logpath(TSRMLS_C);

	if (logpath == NULL || data_len == 0) {
		return 0;
	}

	if (php_stream_stat_path(logpath, &ssb) == FAILURE) {
		if (!php_stream_mkdir(logpath, 0750, REPORT_ERRORS, NULL)) {
			return 0;
		}
	}
	
	spprintf(&logfile, 0, "%s/debugs.log", logpath);
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
		php_stream_write(stream, data, data_len);
		php_stream_close(stream);
	}
	efree(logfile);

	return 1;
}
/* }}} */

/** {{{ void yod_debugs(TSRMLS_D)
*/
void yod_debugs(TSRMLS_D) {
	zval **data;
	long runmode;
	double runtime;
	struct timeval tp = {0};
	char *buffer;
	uint buffer_len;

	zval *ob_buffer;
	int ob_start;

	if (YOD_G(exited)) {
		return;
	}

#if PHP_YOD_DEBUG
	yod_debugf("yod_debugs()");
#endif

	runmode = yod_runmode(TSRMLS_C);

	if ((runmode & 12) == 0) {
		return;
	}

	if (gettimeofday(&tp, NULL)) {
		runtime	= 0;
	} else {
		runtime	= (double)(tp.tv_sec + tp.tv_usec / MICRO_IN_SEC);
	}
	runtime = (runtime - YOD_G(runtime)) * 1000;

#ifdef PHP_OUTPUT_NEWAPI
	ob_start = php_output_start_user(NULL, 0, PHP_OUTPUT_HANDLER_STDFLAGS TSRMLS_CC);
#else
	ob_start = php_start_ob_buffer(NULL, 0, 1 TSRMLS_CC);
#endif

	if (SG(request_info).request_method) {
		php_printf("\n<pre><hr><font color=\"red\">Yod is running in debug mode (%d)</font>\n%s\n", runmode, YOD_DOTLINE);
	} else {
		php_printf("\n%s\nYod is running in debug mode (%d)\n%s\n", YOD_DOTLINE, runmode, YOD_DOTLINE);
	}

	zend_hash_internal_pointer_reset(Z_ARRVAL_P(YOD_G(debugs)));
	while (zend_hash_get_current_data(Z_ARRVAL_P(YOD_G(debugs)), (void **) &data) == SUCCESS) {
		if (Z_TYPE_PP(data) == IS_STRING) {
			PHPWRITE(Z_STRVAL_PP(data), Z_STRLEN_PP(data));
		}
		zend_hash_move_forward(Z_ARRVAL_P(YOD_G(debugs)));
	}

	buffer_len = spprintf(&buffer, 0, "%s\n[%fms]\n", YOD_DOTLINE, runtime);
	PHPWRITE(buffer, buffer_len);
	efree(buffer);

	if (ob_start == SUCCESS) {
		if (runmode & 8) {
			MAKE_STD_ZVAL(ob_buffer);

#ifdef PHP_OUTPUT_NEWAPI
			if (php_output_get_contents(ob_buffer TSRMLS_CC) == SUCCESS) {
#else
			if (php_ob_get_buffer(ob_buffer TSRMLS_CC) == SUCCESS) {
#endif
				if (ob_buffer && Z_TYPE_P(ob_buffer) == IS_STRING) {
					yod_debugw(Z_STRVAL_P(ob_buffer), Z_STRLEN_P(ob_buffer) TSRMLS_CC);
				}
			}
			zval_ptr_dtor(&ob_buffer);
		}

		// runmode
		if (runmode & 4) {
#ifdef PHP_OUTPUT_NEWAPI
			php_output_end(TSRMLS_C);
#else
			if (OG(ob_nesting_level)) {
				php_end_ob_buffer(1, 0 TSRMLS_CC);
			}
#endif		
			return;
		}

#ifdef PHP_OUTPUT_NEWAPI
		php_output_discard(TSRMLS_C);
#else
		if (OG(ob_nesting_level)) {
			php_end_ob_buffer(0, 0 TSRMLS_CC);
		}
#endif
	}
}
/* }}} */

#endif

/*
* Local variables:
* tab-width: 4
* c-basic-offset: 4
* End:
* vim600: noet sw=4 ts=4 fdm=marker
* vim<600: noet sw=4 ts=4
*/
