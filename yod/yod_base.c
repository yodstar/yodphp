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
#include "ext/standard/php_string.h"

#include "php_yod.h"
#include "yod_application.h"
#include "yod_model.h"
#include "yod_dbmodel.h"
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

ZEND_BEGIN_ARG_INFO_EX(yod_base_dmodel_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, config)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_base_db_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, config)
ZEND_END_ARG_INFO()
/* }}} */

/** {{{ int yod_base_config(char *name, uint name_len, zval *result TSRMLS_DC)
*/
int yod_base_config(char *name, uint name_len, zval *result TSRMLS_DC) {
	zval *pzval, **ppval;
	char *skey, *name1, *token;
	uint skey_len;

	if (!YOD_G(loading)) {
		yod_loading(TSRMLS_C);
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

/** {{{ int yod_base_import(char *alias, uint alias_len, char *classext, uint classext_len TSRMLS_DC)
*/
int yod_base_import(char *alias, uint alias_len, char *classext, uint classext_len TSRMLS_DC) {
	zval **ppval;
	char *classfile, *classfile1, *classfile2, *classname, *classpath;
	size_t classfile_len, classname_len;
	zend_class_entry **pce = NULL;
	int depth = 0, depth1 = 0;

	if (!YOD_G(loading)) {
		yod_loading(TSRMLS_C);
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

	classfile2 = classfile;
	while (depth1 < depth) {
		if (*classfile2 == '/') {
			depth1++;
		}
		*classfile2 = tolower(*classfile2);
		classfile2++;
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
				spprintf(&classpath, 0, "%s/%s/%s%s", yod_extpath(TSRMLS_C), YOD_DIR_EXTEND, classfile + 4, classext);
			} else {
				spprintf(&classpath, 0, "%s/%s/%s.class.php", yod_extpath(TSRMLS_C), YOD_DIR_EXTEND, classfile + 4);
			}
		} else {
			if (classext_len) {
				spprintf(&classpath, 0, "%s/%s/%s%s", yod_runpath(TSRMLS_C), YOD_DIR_EXTEND, classfile, classext);
			} else {
				spprintf(&classpath, 0, "%s/%s/%s.class.php", yod_runpath(TSRMLS_C), YOD_DIR_EXTEND, classfile);
			}
		}

		if (VCWD_ACCESS(classpath, F_OK) == 0) {
			yod_include(classpath, NULL, 1 TSRMLS_CC);
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
	int depth = 0, depth1 = 0;

	if (!YOD_G(loading)) {
		yod_loading(TSRMLS_C);
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

	classfile2 = classfile;
	while (depth1 < depth) {
		if (*classfile2 == '/') {
			depth1++;
		}
		*classfile2 = tolower(*classfile2);
		classfile2++;
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
				spprintf(&classpath, 0, "%s/%s/%s%s", yod_extpath(TSRMLS_C), YOD_DIR_PLUGIN, classfile + 4, classext);
			} else {
				spprintf(&classpath, 0, "%s/%s/%s.class.php", yod_extpath(TSRMLS_C), YOD_DIR_PLUGIN, classfile + 4);
			}
		} else if (strncasecmp(classname, "Yod_", 4) == 0) {
			classfile2 = estrndup(classfile, classfile_len - classname_len);
			if (classext_len) {
				spprintf(&classpath, 0, "%s/%s/%s%s%s", yod_extpath(TSRMLS_C), YOD_DIR_PLUGIN, classfile2, classname + 4, classext);
			} else {
				spprintf(&classpath, 0, "%s/%s/%s%s.class.php", yod_extpath(TSRMLS_C), YOD_DIR_PLUGIN, classfile2, classname + 4);
			}
			efree(classfile2);
		} else {
			if (classext_len) {
				spprintf(&classpath, 0, "%s/%s/%s%s", yod_runpath(TSRMLS_C), YOD_DIR_PLUGIN, classfile, classext);
			} else {
				spprintf(&classpath, 0, "%s/%s/%s.class.php", yod_runpath(TSRMLS_C), YOD_DIR_PLUGIN, classfile);
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
			aliaspath_len = spprintf(&aliaspath, 0, "plugins.%s", loweralias);
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

/** {{{ proto protected Yod_Base::dmodel($name = '', $config = '')
*/
PHP_METHOD(yod_base, dmodel) {
	zval *config = NULL;
	char *name = NULL;
	uint name_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sz!", &name, &name_len, &config) == FAILURE) {
		return;
	}

	yod_dbmodel_getinstance(name, name_len, config, return_value TSRMLS_CC);
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

/** {{{ yod_base_methods[]
*/
zend_function_entry yod_base_methods[] = {
	PHP_ME(yod_base, app,        yod_base_app_arginfo,      ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod_base, config,     yod_base_config_arginfo,   ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod_base, import,     yod_base_import_arginfo,   ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod_base, plugin,     yod_base_plugin_arginfo,   ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod_base, model,      yod_base_model_arginfo,    ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod_base, dmodel,     yod_base_dmodel_arginfo,   ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod_base, db,         yod_base_db_arginfo,       ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
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
