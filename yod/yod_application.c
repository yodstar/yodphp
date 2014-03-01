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
#include "ext/standard/php_array.h"
#include "ext/standard/php_string.h"

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

ZEND_BEGIN_ARG_INFO_EX(yod_application_destruct_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()
/* }}} */

#ifdef PHP_WIN32
/** {{{ DIR *opendir(const char *dir)
 * */
static DIR *opendir(const char *dir) {
	DIR *dp;
	char *filespec;
	HANDLE handle;
	int index;
	char resolved_path_buff[MAXPATHLEN];
	TSRMLS_FETCH();

	if (!VCWD_REALPATH(dir, resolved_path_buff)) {
		return NULL;
	}

	filespec = (char *)malloc(strlen(resolved_path_buff) + 2 + 1);
	if (filespec == NULL) {
		return NULL;
	}
	strcpy(filespec, resolved_path_buff);
	index = strlen(filespec) - 1;
	if (index >= 0 && (filespec[index] == '/' || 
	   (filespec[index] == '\\' && (index == 0 || !IsDBCSLeadByte(filespec[index-1])))))
		filespec[index] = '\0';
	strcat(filespec, "\\*");

	dp = (DIR *) malloc(sizeof(DIR));
	if (dp == NULL) {
		return NULL;
	}
	dp->offset = 0;
	dp->finished = 0;

	if ((handle = FindFirstFile(filespec, &(dp->fileinfo))) == INVALID_HANDLE_VALUE) {
		DWORD err = GetLastError();
		if (err == ERROR_NO_MORE_FILES || err == ERROR_FILE_NOT_FOUND) {
			dp->finished = 1;
		} else {
			free(dp);
			free(filespec);
			return NULL;
		}
	}
	dp->dir = strdup(resolved_path_buff);
	dp->handle = handle;
	free(filespec);

	return dp;
}
/* }}} */

/** {{{ int readdir_r(DIR *dp, struct dirent *entry, struct dirent **result)
 * */
static int readdir_r(DIR *dp, struct dirent *entry, struct dirent **result) {
	if (!dp || dp->finished) {
		*result = NULL;
		return 0;
	}

	if (dp->offset != 0) {
		if (FindNextFile(dp->handle, &(dp->fileinfo)) == 0) {
			dp->finished = 1;
			*result = NULL;
			return 0;
		}
	}
	dp->offset++;

	strlcpy(dp->dent.d_name, dp->fileinfo.cFileName, _MAX_FNAME+1);
	dp->dent.d_ino = 1;
	dp->dent.d_reclen = strlen(dp->dent.d_name);
	dp->dent.d_off = dp->offset;

	memcpy(entry, &dp->dent, sizeof(*entry));

	*result = &dp->dent;

	return 0;
}
/* }}} */

/** {{{ int closedir(DIR *dp)
 * */
static int closedir(DIR *dp) {
	if (!dp)
		return 0;
	/* It is valid to scan an empty directory but we have an invalid
	   handle in this case (no first file found). */
	if (dp->handle != INVALID_HANDLE_VALUE) {
		FindClose(dp->handle);
	}
	if (dp->dir)
		free(dp->dir);
	if (dp)
		free(dp);

	return 0;
}
/* }}} */
#endif

/** {{{ static int yod_application_init_autoload(TSRMLS_D)
 * */
static int yod_application_init_autoload(TSRMLS_D) {
	zval *param1, *function, *retval = NULL;
	zval **params[1] = {&param1};
	zend_fcall_info fci;

#if PHP_YOD_DEBUG
	yod_debugf("yod_application_init_autoload()");
#endif

	MAKE_STD_ZVAL(param1);
	array_init(param1);
	add_next_index_string(param1, YOD_APP_CNAME, 1);
	add_next_index_string(param1, "autoload", 1);

	MAKE_STD_ZVAL(function);
	ZVAL_STRING(function, "spl_autoload_register", 1);

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

/** {{{ static void yod_application_init_configs(yod_application_t *object, zval *config TSRMLS_DC)
*/
static void yod_application_init_configs(yod_application_t *object, zval *config TSRMLS_DC) {
	zval *config1, *value1, *pzval, **ppconf, **data, **ppval;
	char *filepath, *filename, *str_key;
	uint filepath_len, entry_len, key_len;
	ulong num_key;
	HashPosition pos;

	DIR *dir = NULL;
	char dentry[sizeof(struct dirent) + MAXPATHLEN];
	struct dirent *entry = (struct dirent *) &dentry;

#if PHP_YOD_DEBUG
	yod_debugf("yod_application_init_configs()");
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
			dir = VCWD_OPENDIR(filepath);
			if (dir) {
				while (php_readdir_r(dir, (struct dirent *) dentry, &entry) == 0 && entry) {
					entry_len = strlen(entry->d_name);

					if (entry_len > 11 && strncmp(entry->d_name + entry_len - 11, ".config.php", 11) == 0) {
						spprintf(&filename, 0, "%s/%s", filepath, entry->d_name);
						if (VCWD_ACCESS(filename, F_OK) == 0) {
							yod_include(filename, &value1, 0 TSRMLS_CC);
							if (Z_TYPE_P(value1) == IS_ARRAY) {
								if (entry_len == 15 && strncmp(entry->d_name, "base", 4) == 0) {
									php_array_merge(Z_ARRVAL_P(config1), Z_ARRVAL_P(value1), 0 TSRMLS_CC);
								} else {
									key_len = entry_len - 11;
									str_key = estrndup(entry->d_name, key_len);

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
					
				}
				closedir(dir);
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

	zend_update_property(yod_application_ce, object, ZEND_STRL("_config"), config1 TSRMLS_CC);
	zval_ptr_dtor(&config1);
}
/* }}} */

/** {{{ static void yod_application_construct(yod_application_t *object, zval *config TSRMLS_DC)
*/
static void yod_application_construct(yod_application_t *object, zval *config TSRMLS_DC) {
	yod_request_t *request;
	zval *imports;

#if PHP_YOD_DEBUG
	yod_debugf("yod_application_construct()");
#endif

	if (YOD_G(yodapp)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Only one application can be initialized");
		return;
	}

	if (object) {
		YOD_G(yodapp) = object;
	} else {
		MAKE_STD_ZVAL(YOD_G(yodapp));
		object_init_ex(YOD_G(yodapp), yod_application_ce);
	}

	// autoload
	yod_application_init_autoload(TSRMLS_C);

	// runmode
	if ((yod_runmode(TSRMLS_C) & 1) == 0) {
		EG(error_reporting) = 0;
	}

	// config
	yod_application_init_configs(YOD_G(yodapp), config TSRMLS_CC);

	// request
	request = yod_request_construct(NULL, NULL, 0 TSRMLS_CC);
	zend_update_property(yod_application_ce, YOD_G(yodapp), ZEND_STRL("_request"), request TSRMLS_CC);
	zval_ptr_dtor(&request);

	// imports
	MAKE_STD_ZVAL(imports);
	array_init(imports);
	zend_update_property(yod_application_ce, YOD_G(yodapp), ZEND_STRL("_imports"), imports TSRMLS_CC);
	zval_ptr_dtor(&imports);

	// app
	zend_update_static_property(yod_application_ce, ZEND_STRL("_app"), YOD_G(yodapp) TSRMLS_CC);
}
/* }}} */

/** {{{ void yod_application_run(TSRMLS_D)
*/
void yod_application_run(TSRMLS_D) {
	yod_request_t *request;

#if PHP_YOD_DEBUG
	yod_debugf("yod_application_run()");
#endif

	if (YOD_G(running)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "An application instance already running");
		return;
	}

	YOD_G(running) = 1;

	zend_update_property_bool(yod_application_ce, YOD_G(yodapp), ZEND_STRL("_running"), 1 TSRMLS_CC);

	request = zend_read_property(yod_application_ce, YOD_G(yodapp), ZEND_STRL("_request"), 1 TSRMLS_CC);
	yod_request_dispatch(request TSRMLS_CC);
}
/* }}} */

/** {{{ int yod_application_config(char *name, uint name_len, zval *result TSRMLS_DC)
*/
int yod_application_config(char *name, uint name_len, zval *result TSRMLS_DC) {
	zval *config, *pzval, **ppval;
	char *name1, *skey, *token;

#if PHP_YOD_DEBUG
	yod_debugf("yod_application_config(%s)", name ? name : "");
#endif

	if (!YOD_G(yodapp)) {
		ZVAL_NULL(result);
		return 0;
	}

	config = zend_read_property(yod_application_ce, YOD_G(yodapp), ZEND_STRL("_config"), 1 TSRMLS_CC);
	if (Z_TYPE_P(config) != IS_ARRAY) {
		ZVAL_NULL(result);
		return 0;
	}

	if (name_len == 0) {
		ZVAL_ZVAL(result, config, 1, 0);
		return 1;
	} else {
		name1 = estrndup(name, name_len);
		if (zend_hash_find(Z_ARRVAL_P(config), name1, name_len + 1, (void **)&ppval) == SUCCESS) {
			ZVAL_ZVAL(result, *ppval, 1, 0);
			efree(name1);
			return 1;
		} else {
			pzval = config;
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
		zval_ptr_dtor(ppval);
		efree(name1);
	}
	return 1;
}
/* }}} */

/** {{{ int yod_application_import(char *alias, uint alias_len, char *classext, uint classext_len TSRMLS_DC)
*/
int yod_application_import(char *alias, uint alias_len, char *classext, uint classext_len TSRMLS_DC) {
	zval *p_imports, **ppval;
	char *classfile, *classname, *classpath;
	size_t classfile_len, classname_len;
	zend_class_entry **pce = NULL;

#if PHP_YOD_DEBUG
	yod_debugf("yod_application_import(%s)", alias ? alias : "");
#endif

	if (!YOD_G(yodapp) || alias_len == 0) {
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

	p_imports = zend_read_property(yod_application_ce, YOD_G(yodapp), ZEND_STRL("_imports"), 1 TSRMLS_CC);
	if (zend_hash_find(Z_ARRVAL_P(p_imports), alias, alias_len + 1, (void **)&ppval) == FAILURE) {
		if (classext_len) {
			spprintf(&classpath, 0, "%s/extends/%s%s", yod_runpath(TSRMLS_C), classfile, classext);
		} else {
			spprintf(&classpath, 0, "%s/extends/%s.class.php", yod_runpath(TSRMLS_C), classfile);
		}

		if (VCWD_ACCESS(classpath, F_OK) == 0) {
			yod_include(classpath, NULL, 1 TSRMLS_CC);
		}
		efree(classpath);

		add_assoc_string_ex(p_imports, alias, alias_len + 1, classpath, 1);
		zend_update_property(yod_application_ce, YOD_G(yodapp), ZEND_STRL("_imports"), p_imports TSRMLS_CC);
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

/** {{{ static void yod_application_autorun(TSRMLS_D)
*/
static void yod_application_autorun(TSRMLS_D) {
	zval runpath;

#if PHP_YOD_DEBUG
	yod_debugf("yod_application_autorun()");
#endif

	if (zend_get_constant(ZEND_STRL("YOD_RUNPATH"), &runpath TSRMLS_CC)) {
		if (!YOD_G(yodapp) && !YOD_G(exited)) {
			SG(headers_sent) = 0;

			yod_application_app(NULL TSRMLS_CC);
			yod_application_run(TSRMLS_C);

			if (!SG(headers_sent)) {
				sapi_send_headers(TSRMLS_C);
			}
		}
		zval_dtor(&runpath);
	}

#if PHP_YOD_DEBUG
	if (YOD_G(yodapp)) {
		yod_debugs(TSRMLS_C);
	}
#endif
}
/* }}} */

/** {{{ static int yod_application_autoload(char *classname, uint classname_len TSRMLS_DC)
*/
static int yod_application_autoload(char *classname, uint classname_len TSRMLS_DC) {
	zend_class_entry **pce = NULL;
	char *classfile, *classpath;
	zval *retval;

	if (!YOD_G(yodapp)) return 0;

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

	if (VCWD_ACCESS(classpath, F_OK) == 0) {
		yod_include(classpath, &retval, 1 TSRMLS_CC);
	}
	
#if PHP_YOD_DEBUG
	yod_debugf("yod_application_autoload(%s):%s", classname, classpath);
#endif

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

/** {{{ proto public Yod_Application::__construct($config = null)
*/
PHP_METHOD(yod_application, __construct) {
	zval *config = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z", &config) == FAILURE) {
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

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z", &config) == FAILURE) {
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

/** {{{ proto public Yod_Application::__destruct()
*/
PHP_METHOD(yod_application, __destruct) {

	if (!YOD_G(running) && !YOD_G(exited)) {
		yod_application_run(TSRMLS_C);
	}
}
/* }}} */

/* {{{ yod_application_methods[]
*/
zend_function_entry yod_application_methods[] = {
	PHP_ME(yod_application, __construct,	yod_application_construct_arginfo,	ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(yod_application, run,			yod_application_run_arginfo,		ZEND_ACC_PUBLIC)
	PHP_ME(yod_application, config,			yod_application_config_arginfo,		ZEND_ACC_PUBLIC)
	PHP_ME(yod_application, import,			yod_application_import_arginfo,		ZEND_ACC_PUBLIC)
	PHP_ME(yod_application, app,			yod_application_app_arginfo,		ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod_application, autorun,		yod_application_autorun_arginfo,	ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod_application, autoload,		yod_application_autoload_arginfo,	ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
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
	zend_declare_property_null(yod_application_ce, ZEND_STRL("_config"), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yod_application_ce, ZEND_STRL("_request"), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yod_application_ce, ZEND_STRL("_imports"), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_bool(yod_application_ce, ZEND_STRL("_running"), 0, ZEND_ACC_PROTECTED TSRMLS_CC);

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
