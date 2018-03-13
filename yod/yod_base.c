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
#include "main/SAPI.h"
#include "Zend/zend_interfaces.h"
#include "ext/standard/file.h"
#include "ext/standard/flock_compat.h"
#include "ext/standard/php_filestat.h"
#include "ext/standard/php_string.h"
#include "ext/session/php_session.h"

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
#include "yod_application.h"
#include "yod_model.h"
#include "yod_database.h"
#include "yod_base.h"

#if PHP_YOD_DEBUG
#include "yod_debug.h"
#endif

zend_class_entry *yod_base_ce;

/** {{{ ARG_INFO
*/
ZEND_BEGIN_ARG_INFO_EX(yod_base_app_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, config)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_base_config_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_base_session_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_base_import_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, alias)
	ZEND_ARG_INFO(0, classext)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_base_plugin_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, alias)
	ZEND_ARG_INFO(0, classext)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_base_model_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, config)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_base_autoload_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, classname)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_base_errorlog_arginfo, 0, 0, 4)
	ZEND_ARG_INFO(0, errno)
	ZEND_ARG_INFO(0, errstr)
	ZEND_ARG_INFO(0, errfile)
	ZEND_ARG_INFO(0, errline)
	ZEND_ARG_INFO(0, errcontext)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_base_db_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, config)
ZEND_END_ARG_INFO()
/* }}} */

/** {{{ int yod_base_config(char *name, uint name_len, zval *result TSRMLS_DC)
*/
int yod_base_config(char *name, uint name_len, zval *result TSRMLS_DC) {
	zval *pzval, **ppval;
	char *skey, *name1, *name2, *token;
	uint skey_len;

	if (!YOD_G(startup)) {
		yod_init_startup(TSRMLS_C);
	}

	if (!YOD_G(config) || Z_TYPE_P(YOD_G(config)) != IS_ARRAY) {
		ZVAL_NULL(result);
		return 0;
	}

	if (name_len == 0) {
		ZVAL_ZVAL(result, YOD_G(config), 1, 0);
		return 1;
	} else {
		name1 = estrndup(name, name_len);

		name2 = name1;
		while (*name2 != '\0') {
			if (*name2 == '.') {
				break;
			}
			*name2 = tolower(*name2);
			name2++;
		}

		if (zend_hash_find(Z_ARRVAL_P(YOD_G(config)), name1, name_len + 1, (void **)&ppval) == SUCCESS) {
			ZVAL_ZVAL(result, *ppval, 1, 0);
			efree(name1);
			return 1;
		} else {
			pzval = YOD_G(config);
			skey = php_strtok_r(name1, ".", &token);
			while (skey) {
				skey_len = strlen(skey);
				if (zend_hash_find(Z_ARRVAL_P(pzval), skey, skey_len + 1, (void **)&ppval) == SUCCESS ||
					(is_numeric_string(skey, skey_len, NULL, NULL, 0) == IS_LONG &&
						zend_hash_index_find(Z_ARRVAL_P(pzval), atoi(skey), (void**)&ppval) == SUCCESS)
				) {
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
		efree(name1);
		return 1;
	}
}
/* }}} */

/** {{{ int yod_base_session(zval *name, zval *value, zval *result, int num_args TSRMLS_DC)
*/
int yod_base_session(zval *name, zval *value, zval *result, int num_args TSRMLS_DC) {
	char *name1, *name2;
	uint name1_len, name2_len;
	zval **value1, **value2;

	/* session_start */
	if (PS(session_status) != php_session_active) {
		php_session_start(TSRMLS_C);
	}

	/* When the number of arguments is 0 */
	if (num_args == 0) {
		/* return $_SESSION */
		if (PS(http_session_vars) && PS(http_session_vars)->type == IS_ARRAY) {
			ZVAL_ZVAL(result, PS(http_session_vars), 1, 0);
		}
		return 1;
	}

	/* When the number of arguments is greater than 1 */
	if (num_args > 1) {
		if (!value || Z_TYPE_P(value) == IS_NULL) {
			if (!name || Z_TYPE_P(name) == IS_NULL) {
				/* When name and value are null, return $_SESSION */
				if (PS(http_session_vars) && PS(http_session_vars)->type == IS_ARRAY) {
					ZVAL_ZVAL(result, PS(http_session_vars), 1, 0);
				}
			} else {
				/* When name isn't null and value is null, unset $_SESSION[name] */
				if (PS(http_session_vars) && PS(http_session_vars)->type == IS_ARRAY) {
					SEPARATE_ZVAL_IF_NOT_REF(&PS(http_session_vars));
					PS_DEL_VARL(Z_STRVAL_P(name), Z_STRLEN_P(name));
				}
			}
		} else {
			if (Z_TYPE_P(name) != IS_STRING) {
				convert_to_string(name);
			}

			/* When name and value aren't null, $_SESSION[name] = value */
			php_set_session_var(Z_STRVAL_P(name), Z_STRLEN_P(name), value, NULL TSRMLS_CC);
			PS_ADD_VARL(Z_STRVAL_P(name), Z_STRLEN_P(name));
		}
		return 1;
	}

	/* When the number of arguments is 1 */

	if (!name || Z_TYPE_P(name) == IS_NULL) {
		/* When name is null, session_unset */
		if (PS(http_session_vars) && PS(http_session_vars)->type == IS_ARRAY) {
			HashTable *ht_sess_var;

			SEPARATE_ZVAL_IF_NOT_REF(&PS(http_session_vars));
			ht_sess_var = Z_ARRVAL_P(PS(http_session_vars));

#if PHP_API_VERSION < 20100412
			if (PG(register_globals)) {
				char *str_key;
				uint key_len;
				ulong num_key;
				HashPosition pos;

				zend_hash_internal_pointer_reset_ex(ht_sess_var, &pos);

				while (zend_hash_get_current_key_ex(ht_sess_var, &str_key, &key_len, &num_key, 0, &pos) == HASH_KEY_IS_STRING) {
					zend_delete_global_variable(str_key, key_len - 1 TSRMLS_CC);
					zend_hash_move_forward_ex(ht_sess_var, &pos);
				}
			}
#endif
			/* Clean $_SESSION. */
			zend_hash_clean(ht_sess_var);
		}
		return 1;
	}

	if (Z_TYPE_P(name) != IS_STRING) {
		convert_to_string(name);
	}
	
	if (Z_STRLEN_P(name) > 0) {
		/* When name isn't null, return $_SESSION[name] */
		name1_len = 0;
		name2_len = Z_STRLEN_P(name);
		name1 = name2 = Z_STRVAL_P(name);
		while (name2_len > 0) {
			if (*name2 == '.') {
				name2++;
				name2_len--;
				break;
			}
			name2++;
			name2_len--;
			name1_len++;
		}

		if (name1_len > 0 && name2_len > 0) { /* $_SESSION[name1][name2] */
			name1 = estrndup(name1, name1_len);
			if (php_get_session_var(name1, name1_len, &value1 TSRMLS_CC) == SUCCESS
				&& Z_TYPE_PP(value1) == IS_ARRAY
				&& (zend_hash_find(Z_ARRVAL_PP(value1), name2, name2_len + 1, (void **)&value2) == SUCCESS)
			) {
				ZVAL_ZVAL(result, *value2, 1, 0);
			}
			efree(name1);
		} else if (name1_len > 0) { /* $_SESSION[name1] */
			if (php_get_session_var(name1, name1_len, &value1 TSRMLS_CC) == SUCCESS) {
				ZVAL_ZVAL(result, *value1, 1, 0);
			}
		} else if (name2_len > 0) { /* $_SESSION[name2] */
			if (php_get_session_var(name2, name2_len, &value1 TSRMLS_CC) == SUCCESS) {
				ZVAL_ZVAL(result, *value1, 1, 0);
			}
		}
		return 1;
	}

	return 0;
}
/* }}} */

/** {{{ int yod_base_import(char *alias, uint alias_len, char *classext, uint classext_len TSRMLS_DC)
*/
int yod_base_import(char *alias, uint alias_len, char *classext, uint classext_len TSRMLS_DC) {
	zval **ppval;
	char *classfile, *classfile1, *classfile2, *classname, *classpath;
	size_t classfile_len, classname_len;
	zend_class_entry **pce = NULL;
	int depth = 0, depth1 = 0;

	if (!YOD_G(startup)) {
		yod_init_startup(TSRMLS_C);
	}

	if (alias_len == 0) {
		return 0;
	}

	classfile1 = classfile = estrndup(alias, alias_len);
	classfile_len = 0;
	
	while (*classfile != '\0') {
		if (*classfile == '.' || *classfile == '\\') {
			*classfile = '/';
			depth++;
		}
		classfile++;
		classfile_len++;
	}

	while (*classfile == '/') {
		classfile--;
		classfile_len--;
		depth--;
	}

	classfile = classfile - classfile_len;

	while (*classfile == '/') {
		classfile++;
		classfile_len--;
		depth--;
	}

	php_basename(classfile, classfile_len, NULL, 0, &classname, &classname_len TSRMLS_CC);

	if (zend_hash_find(Z_ARRVAL_P(YOD_G(imports)), alias, alias_len + 1, (void **)&ppval) == FAILURE) {
#if PHP_API_VERSION < 20100412
		if (zend_lookup_class_ex(classname, classname_len, 0, &pce TSRMLS_CC) == SUCCESS) {
#else
		if (zend_lookup_class_ex(classname, classname_len, NULL, 0, &pce TSRMLS_CC) == SUCCESS) {
#endif
			add_assoc_bool_ex(YOD_G(imports), alias, alias_len + 1, 1);

			efree(classfile1);
			efree(classname);
			return 1;
		}

		if (classfile_len > 4 && strncasecmp(classfile, "yod/", 4) == 0) {
			if (classext_len) {
				spprintf(&classpath, 0, "%s/%s/Yod/%s%s", yod_libpath(TSRMLS_C), YOD_DIR_EXTEND, classfile + 4, classext);
			} else {
				spprintf(&classpath, 0, "%s/%s/Yod/%s.php", yod_libpath(TSRMLS_C), YOD_DIR_EXTEND, classfile + 4);
			}
		} else {
			if (classext_len) {
				spprintf(&classpath, 0, "%s/%s/%s%s", yod_libpath(TSRMLS_C), YOD_DIR_EXTEND, classfile, classext);
			} else {
				spprintf(&classpath, 0, "%s/%s/%s.php", yod_libpath(TSRMLS_C), YOD_DIR_EXTEND, classfile);
			}
		}

		if (VCWD_ACCESS(classpath, F_OK) == 0) {
			yod_include(classpath, NULL, 1 TSRMLS_CC);
		} else {
			
			classfile2 = classfile;
			while (depth1 < depth) {
				if (*classfile2 == '/') {
					depth1++;
				}
				*classfile2 = tolower(*classfile2);
				classfile2++;
			}

			if (classfile_len > 4 && strncasecmp(classfile, "yod/", 4) == 0) {
				if (classext_len) {
					spprintf(&classpath, 0, "%s/%s/Yod/%s%s", yod_libpath(TSRMLS_C), YOD_DIR_EXTEND, classfile + 4, classext);
				} else {
					spprintf(&classpath, 0, "%s/%s/Yod/%s.php", yod_libpath(TSRMLS_C), YOD_DIR_EXTEND, classfile + 4);
				}
			} else {
				if (classext_len) {
					spprintf(&classpath, 0, "%s/%s/%s%s", yod_libpath(TSRMLS_C), YOD_DIR_EXTEND, classfile, classext);
				} else {
					spprintf(&classpath, 0, "%s/%s/%s.php", yod_libpath(TSRMLS_C), YOD_DIR_EXTEND, classfile);
				}
			}

			if (VCWD_ACCESS(classpath, F_OK) == 0) {
				yod_include(classpath, NULL, 1 TSRMLS_CC);
			}
		}

		efree(classpath);

		add_assoc_string_ex(YOD_G(imports), alias, alias_len + 1, classpath, 1);
	}
	efree(classfile1);

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

/** {{{ int yod_base_plugin(char *alias, uint alias_len, char *classext, uint classext_len, zval *result TSRMLS_DC)
*/
int yod_base_plugin(char *alias, uint alias_len, char *classext, uint classext_len, zval *result TSRMLS_DC) {
	zval *config1, **ppval, *object = NULL;
	char *aliaspath, *classfile, *classfile1, *classfile2, *classname, *classname1, *classpath, *loweralias;
	size_t aliaspath_len, classfile_len, classname_len;
	zend_class_entry **pce = NULL;

	if (!YOD_G(startup)) {
		yod_init_startup(TSRMLS_C);
	}

	if (alias_len == 0) {
		return 0;
	}

	classfile1 = classfile = estrndup(alias, alias_len);
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

	if (zend_hash_find(Z_ARRVAL_P(YOD_G(plugins)), alias, alias_len + 1, (void **)&ppval) == SUCCESS) {
		if (result) {
			ZVAL_ZVAL(result, *ppval, 1, 0);
		}

		efree(classfile1);
		efree(classname);
		return 1;
	}

#if PHP_API_VERSION < 20100412
	if (zend_lookup_class_ex(classname, classname_len, 0, &pce TSRMLS_CC) == FAILURE) {
#else
	if (zend_lookup_class_ex(classname, classname_len, NULL, 0, &pce TSRMLS_CC) == FAILURE) {
#endif
		if (classfile_len > 4 && strncasecmp(classfile, "yod/", 4) == 0) {
			if (strncasecmp(classname, "Yod_", 4)) {
				classname_len = spprintf(&classname1, 0, "Yod_%s", classname);
				efree(classfile1);
				classname = classname1;
			}
			if (classext_len) {
				spprintf(&classpath, 0, "%s/%s/Yod/%s%s", yod_libpath(TSRMLS_C), YOD_DIR_PLUGIN, classfile + 4, classext);
			} else {
				spprintf(&classpath, 0, "%s/%s/Yod/%s.php", yod_libpath(TSRMLS_C), YOD_DIR_PLUGIN, classfile + 4);
			}
		} else if (strncasecmp(classname, "Yod_", 4) == 0) {
			classfile2 = estrndup(classfile, classfile_len - classname_len);
			if (classext_len) {
				spprintf(&classpath, 0, "%s/%s/Yod/%s%s%s", yod_libpath(TSRMLS_C), YOD_DIR_PLUGIN, classfile2, classname + 4, classext);
			} else {
				spprintf(&classpath, 0, "%s/%s/Yod/%s%s.php", yod_libpath(TSRMLS_C), YOD_DIR_PLUGIN, classfile2, classname + 4);
			}
			efree(classfile2);
		} else {
			if (classext_len) {
				spprintf(&classpath, 0, "%s/%s/%s%s", yod_libpath(TSRMLS_C), YOD_DIR_PLUGIN, classfile, classext);
			} else {
				spprintf(&classpath, 0, "%s/%s/%s.php", yod_libpath(TSRMLS_C), YOD_DIR_PLUGIN, classfile);
			}
		}

		if (VCWD_ACCESS(classpath, F_OK) == 0) {
			yod_include(classpath, NULL, 1 TSRMLS_CC);
		} else {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Plugin '%s' not found", classpath);
		}
		efree(classpath);
	}
	efree(classfile1);

#if PHP_API_VERSION < 20100412
	if (zend_lookup_class_ex(classname, classname_len, 0, &pce TSRMLS_CC) == SUCCESS) {
#else
	if (zend_lookup_class_ex(classname, classname_len, NULL, 0, &pce TSRMLS_CC) == SUCCESS) {
#endif
		MAKE_STD_ZVAL(object);
		object_init_ex(object, *pce);
		if (zend_hash_exists(&(*pce)->function_table, ZEND_STRS(ZEND_CONSTRUCTOR_FUNC_NAME))) {
			MAKE_STD_ZVAL(config1);
			loweralias = estrndup(alias, alias_len);
			zend_str_tolower(loweralias, alias_len);
			aliaspath_len = spprintf(&aliaspath, 0, "plugin.%s", loweralias);
			yod_base_config(aliaspath, aliaspath_len, config1 TSRMLS_CC);
			if (config1 && Z_TYPE_P(config1) != IS_NULL) {
				zend_call_method_with_1_params(&object, *pce, &(*pce)->constructor, ZEND_CONSTRUCTOR_FUNC_NAME, NULL, config1);
			} else {
				zend_call_method_with_0_params(&object, *pce, &(*pce)->constructor, ZEND_CONSTRUCTOR_FUNC_NAME, NULL);
			}
			zval_ptr_dtor(&config1);
			efree(loweralias);
			efree(aliaspath);
		}

		add_assoc_zval_ex(YOD_G(plugins), alias, alias_len + 1, object);
		
	} else {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Class '%s' not found", classname);
	}
	efree(classname);

	if (object) {
		if (result) {
			ZVAL_ZVAL(result, object, 1, 0);
		}
		return 1;
	}

	ZVAL_NULL(result);
	return 0;
}
/* }}} */

/** {{{ static int yod_base_autoload(char *classname, uint classname_len TSRMLS_DC)
*/
static int yod_base_autoload(char *classname, uint classname_len TSRMLS_DC) {
	zend_class_entry **pce = NULL;
	char *classfile, *classfile1, *classfile2, *classpath, *classpath1;
	int depth = 0, depth1 = 0;

#if PHP_YOD_DEBUG
	yod_debugf("yod_base_autoload(%s)", classname);
#endif

	classfile1 = classfile = estrndup(classname, classname_len);
	/* class name with namespace in PHP 5.3 */
	if (strstr(classname, "\\")) {
		while (*classfile != '\0') {
			if (*classfile == '\\') {
				*classfile = '/';
				depth++;
			}
			classfile++;
		}
		classfile = classfile - classname_len;
	}

	if (strncasecmp(classfile, "Yod_", 4) == 0) { /* yodphp extends class */
		if (strncasecmp(classfile, "Yod_Db", 6) == 0) {
			spprintf(&classpath, 0, "%s/%s/%s.php", yod_libpath(TSRMLS_C), YOD_DIR_DRIVER, classfile + 4);
		} else {
			spprintf(&classpath, 0, "%s/%s/Yod/%s.php", yod_libpath(TSRMLS_C), YOD_DIR_EXTEND, classfile + 4);
		}
	} else {
		if (classname_len > 10 && strncasecmp(classfile + classname_len - 10, "Controller", 10) == 0) {
			spprintf(&classpath, 0, "%s/%s/%s/%s.php", yod_runpath(TSRMLS_C), YOD_G(modname), YOD_DIR_CONTROLLER, classfile);
		} else if (classname_len > 5 && strncasecmp(classfile + classname_len - 5, "Model", 5) == 0) {
			spprintf(&classpath, 0, "%s/%s/%s/%s.php", yod_runpath(TSRMLS_C), YOD_G(modname), YOD_DIR_MODEL, classfile);
			if (VCWD_ACCESS(classpath, F_OK) != 0) {
				spprintf(&classpath1, 0, "%s/%s/%s.php", yod_libpath(TSRMLS_C), YOD_DIR_MODEL, classfile);
				if (classpath) {
					efree(classpath);
				}
				classpath = classpath1;
			}
		} else if (classname_len > 7 && strncasecmp(classfile + classname_len - 7, "Service", 7) == 0) {
			spprintf(&classpath, 0, "%s/%s/%s/%s.php", yod_runpath(TSRMLS_C), YOD_G(modname), YOD_DIR_SERVICE, classfile);
			if (VCWD_ACCESS(classpath, F_OK) != 0) {
				spprintf(&classpath1, 0, "%s/%s/%s.php", yod_libpath(TSRMLS_C), YOD_DIR_SERVICE, classfile);
				if (classpath) {
					efree(classpath);
				}
				classpath = classpath1;
			}
		} else {
			spprintf(&classpath, 0, "%s/%s/%s.php", yod_libpath(TSRMLS_C), YOD_DIR_EXTEND, classfile);
		}
	}

	if (VCWD_ACCESS(classpath, F_OK) == 0) {
		yod_include(classpath, NULL, 1 TSRMLS_CC);
	}
/*
	else {
		classfile2 = classfile;
		while (depth1 < depth) {
			if (*classfile == '/') {
				depth1++;
			}
			*classfile = tolower(*classfile);
			classfile++;
		}
		classfile = classfile2;

		if (strncasecmp(classfile, "Yod_", 4) == 0) {
			if (strncasecmp(classfile, "Yod_Db", 6) == 0) {
				spprintf(&classpath, 0, "%s/%s/%s.php", yod_libpath(TSRMLS_C), YOD_DIR_DRIVER, classfile + 4);
			} else {
				spprintf(&classpath, 0, "%s/%s/Yod/%s.php", yod_libpath(TSRMLS_C), YOD_DIR_EXTEND, classfile + 4);
			}
		} else {
			if (classname_len > 10 && strncasecmp(classfile + classname_len - 10, "Controller", 10) == 0) {
				spprintf(&classpath, 0, "%s/%s/%s/%s.php", yod_runpath(TSRMLS_C), YOD_G(modname), YOD_DIR_CONTROLLER, classfile);
			} else if (classname_len > 5 && strncasecmp(classfile + classname_len - 5, "Model", 5) == 0) {
				spprintf(&classpath, 0, "%s/%s/%s/%s.php", yod_runpath(TSRMLS_C), YOD_G(modname), YOD_DIR_MODEL, classfile);
				if (VCWD_ACCESS(classpath, F_OK) != 0) {
					spprintf(&classpath1, 0, "%s/%s/%s.php", yod_libpath(TSRMLS_C), YOD_DIR_MODEL, classfile);
					if (classpath) {
						efree(classpath);
					}
					classpath = classpath1;
				}
			} else if (classname_len > 7 && strncasecmp(classfile + classname_len - 7, "Service", 7) == 0) {
				spprintf(&classpath, 0, "%s/%s/%s/%s.php", yod_runpath(TSRMLS_C), YOD_G(modname), YOD_DIR_SERVICE, classfile);
				if (VCWD_ACCESS(classpath, F_OK) != 0) {
					spprintf(&classpath1, 0, "%s/%s/%s.php", yod_libpath(TSRMLS_C), YOD_DIR_SERVICE, classfile);
					if (classpath) {
						efree(classpath);
					}
					classpath = classpath1;
				}
			} else {
				spprintf(&classpath, 0, "%s/%s/%s.php", yod_libpath(TSRMLS_C), YOD_DIR_EXTEND, classfile);
			}
		}

		if (VCWD_ACCESS(classpath, F_OK) == 0) {
			yod_include(classpath, NULL, 1 TSRMLS_CC);
		}
	}
*/
	efree(classfile1);
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

/** {{{ static int yod_base_errorlog(long errnum, char *errstr, uint errstr_len, char *errfile, uint errfile_len, long errline, zval *errcontext TSRMLS_DC)
*/
static int yod_base_errorlog(long errnum, char *errstr, uint errstr_len, char *errfile, uint errfile_len, long errline, zval *errcontext TSRMLS_DC) {
	struct timeval tp = {0};
	struct tm *ta, tmbuf;
	time_t curtime;
	char *datetime, asctimebuf[52];
	uint datetime_len;

	zval *zcontext = NULL;
	php_stream_statbuf ssb;
	php_stream_context *context;
	php_stream *stream;

	char *logdata, *datadir, *logfile, *errtype;
	uint logdata_len;

	switch (errnum) {
	 	case E_ERROR:
	 	case E_CORE_ERROR:
	 	case E_COMPILE_ERROR:
	 	case E_USER_ERROR:
	 	case E_RECOVERABLE_ERROR:
	 		errtype = "Error";
	 		break;
	 	case E_WARNING:
	 	case E_CORE_WARNING:
	 	case E_COMPILE_WARNING:
	 	case E_USER_WARNING:
	 		errtype = "Warning";
	 		break;
	 	case E_PARSE:
	 		errtype = "Parse";
	 		break;
	 	case E_NOTICE:
	 	case E_USER_NOTICE:
	 		errtype = "Notice";
	 		break;
	 	case E_STRICT:
	 		errtype = "Strict";
	 		break;
#ifdef E_DEPRECATED
	 	case E_DEPRECATED:
	 	case E_USER_DEPRECATED:
	 		errtype = "Deprecated";
	 		break;
#endif
	 	default:
	 		errtype = "Unknown";
	 		break;
	}

	datadir = yod_datadir(TSRMLS_C);
	if (php_stream_stat_path(datadir, &ssb) == FAILURE) {
		if (!php_stream_mkdir(datadir, 0750, REPORT_ERRORS, NULL)) {
			return 0;
		}
	}

	spprintf(&logfile, 0, "%s/error.log", datadir);
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

		time(&curtime);
		ta = php_localtime_r(&curtime, &tmbuf);
		datetime = php_asctime_r(ta, asctimebuf);
		datetime_len = strlen(datetime);
		datetime[datetime_len - 1] = 0;

		if (!gettimeofday(&tp, NULL)) {
			logdata_len = spprintf(&logdata, 0, "[%s %06d] %s: %s in %s(%d)\n", datetime, tp.tv_usec, errtype, (errstr_len ? errstr : ""), (errfile_len ? errfile : "Unknown"), errline);
			php_stream_write(stream, logdata, logdata_len);
			efree(logdata);
		}
		php_stream_close(stream);
	}
	efree(logfile);

	return 1;
}
/* }}} */

/** {{{ proto public Yod_Base::app($config = null)
*/
PHP_METHOD(yod_base, app) {
	zval *config = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z!", &config) == FAILURE) {
		return;
	}

	yod_application_app(config TSRMLS_CC);

	RETURN_ZVAL(YOD_G(yodapp), 1, 0);
}
/* }}} */

/** {{{ proto public Yod_Base::config($name = null)
*/
PHP_METHOD(yod_base, config) {
	char *name = NULL;
	uint name_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &name, &name_len) == FAILURE) {
		return;
	}

	yod_base_config(name, name_len, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_Base::session($name = null, $value = null)
*/
PHP_METHOD(yod_base, session) {
	zval *name = NULL, *value = NULL;
	int num_args = 0;

	num_args = ZEND_NUM_ARGS();

	if (zend_parse_parameters(num_args TSRMLS_CC, "|zz!", &name, &value) == FAILURE) {
		return;
	}

	yod_base_session(name, value, return_value, num_args TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_Base::import($alias, $classext = '.class.php')
*/
PHP_METHOD(yod_base, import) {
	char *alias = NULL, *classext = NULL;
	uint alias_len = 0, classext_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s", &alias, &alias_len, &classext, &classext_len) == FAILURE) {
		return;
	}

	if (yod_base_import(alias, alias_len, classext, classext_len TSRMLS_CC)) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yod_Base::plugin($alias, $classext = '.class.php')
*/
PHP_METHOD(yod_base, plugin) {
	char *alias = NULL, *classext = NULL;
	uint alias_len = 0, classext_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|s", &alias, &alias_len, &classext, &classext_len) == FAILURE) {
		return;
	}

	yod_base_plugin(alias, alias_len, classext, classext_len, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ proto protected Yod_Base::model($name = '', $config = '')
*/
PHP_METHOD(yod_base, model) {
	zval *config = NULL;
	char *name = NULL;
	uint name_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sz!", &name, &name_len, &config) == FAILURE) {
		return;
	}

	yod_model_getinstance(name, name_len, config, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_Base::db($config = 'db_dsn')
*/
PHP_METHOD(yod_base, db) {
	zval *config = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z!", &config) == FAILURE) {
		return;
	}

	yod_database_getinstance(config, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ proto public static Yod_Base::autoload($classname)
*/
PHP_METHOD(yod_base, autoload) {
	char *classname;
	uint classname_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &classname, &classname_len) == FAILURE) {
		return;
	}

	if (yod_base_autoload(classname, classname_len TSRMLS_CC)) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public static Yod_Base::errorlog($errno, $errstr, $errfile = '', $errline = 0, $errcontext = array())
*/
PHP_METHOD(yod_base, errorlog) {
	zval *errcontext = NULL;
	char *errstr, *errfile = NULL;
	uint errstr_len, errfile_len = 0;
	long errnum, errline = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls|slz!", &errnum, &errstr, &errstr_len, &errfile, &errfile_len, &errline, &errcontext) == FAILURE) {
		return;
	}

	yod_base_errorlog(errnum, errstr, errstr_len, errfile, errfile_len, errline, errcontext TSRMLS_CC);

	RETURN_FALSE;
}
/* }}} */

/** {{{ yod_base_methods[]
*/
zend_function_entry yod_base_methods[] = {
	PHP_ME(yod_base, app,		yod_base_app_arginfo,		ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod_base, config,	yod_base_config_arginfo,	ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod_base, session,	yod_base_session_arginfo,	ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod_base, import,	yod_base_import_arginfo,	ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod_base, plugin,	yod_base_plugin_arginfo,	ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod_base, model,		yod_base_model_arginfo,		ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod_base, db,		yod_base_db_arginfo,		ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod_base, autoload,	yod_base_autoload_arginfo,	ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod_base, errorlog,	yod_base_errorlog_arginfo,	ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ PHP_MINIT_FUNCTION
*/
PHP_MINIT_FUNCTION(yod_base) {
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "Yod_Base", yod_base_methods);
	yod_base_ce = zend_register_internal_class(&ce TSRMLS_CC);

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
