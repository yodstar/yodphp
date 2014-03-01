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
#include "yod_model.h"
#include "yod_dbmodel.h"
#include "yod_database.h"

#if PHP_YOD_DEBUG
#include "yod_debug.h"
#endif

zend_class_entry *yod_model_ce;

/** {{{ ARG_INFO
*/
ZEND_BEGIN_ARG_INFO_EX(yod_model_construct_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, config)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_model_init_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_model_getinstance_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, config)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_model_find_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, where)
	ZEND_ARG_INFO(0, params)
	ZEND_ARG_INFO(0, select)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_model_findall_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, where)
	ZEND_ARG_INFO(0, params)
	ZEND_ARG_INFO(0, select)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_model_count_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, where)
	ZEND_ARG_INFO(0, params)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_model_save_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, data)
	ZEND_ARG_INFO(0, where)
	ZEND_ARG_INFO(0, params)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_model_remove_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, where)
	ZEND_ARG_INFO(0, params)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_model_lastquery_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_model_config_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_model_import_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, alias)
	ZEND_ARG_INFO(0, classext)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_model_model_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, config)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_model_dbmodel_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, config)
ZEND_END_ARG_INFO()
/* }}} */

/** {{{ int yod_model_construct(yod_model_t *object, char *name, uint name_len, zval *config TSRMLS_DC)
*/
int yod_model_construct(yod_model_t *object, char *name, uint name_len, zval *config TSRMLS_DC) {
	yod_database_t *yoddb;
	zval *config1, *name1, *table1, *p_dsn, *prefix;
	char *table, *tname;
	uint table_len, tname_len;

#if PHP_YOD_DEBUG
	yod_debugf("yod_model_construct(%s)", name ? name : "");
#endif

	if (!object) {
		return 0;
	}

	if (!name || name_len == 0) {
		name1 = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_name"), 1 TSRMLS_CC);
		if (name1 && Z_TYPE_P(name1) == IS_STRING && Z_STRLEN_P(name1) > 0) {
			tname_len = Z_STRLEN_P(name1);
			tname = estrndup(Z_STRVAL_P(name1), tname_len);
		} else {
			tname_len = strlen(Z_OBJCE_P(object)->name) - 5;
			if (tname_len > 0 && strncmp(Z_OBJCE_P(object)->name, "Yod_", 4)) {
				tname = estrndup(Z_OBJCE_P(object)->name, tname_len);
			} else {
				tname_len = 0;
				tname = NULL;
			}
		}
	} else {
		tname_len = name_len;
		tname = estrndup(name, name_len);
	}
	zend_update_property_stringl(Z_OBJCE_P(object), object, ZEND_STRL("_name"), tname, tname_len TSRMLS_CC);

	table1 = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_table"), 1 TSRMLS_CC);
	if (!table1 || Z_TYPE_P(table1) != IS_STRING || Z_STRLEN_P(table1) == 0) {
		if (tname_len > 0) {
			table = (char *) emalloc(tname_len*2);
			*table = tolower(*tname);
			table_len = 1;
			table++;
			tname++;
			while (*tname != '\0') {
				if (isupper(*tname)) {
					*table = '_';
					table_len++;
					table++;
				}
				*table = tolower(*tname);
				table_len++;
				table++;
				tname++;
			}
			*table = '\0';
			table = table - table_len;
			tname = tname - tname_len;
			zend_update_property_stringl(Z_OBJCE_P(object), object, ZEND_STRL("_table"), table, table_len TSRMLS_CC);
			efree(table);
		}
	}
	if (tname) {
		efree(tname);
	}

	MAKE_STD_ZVAL(config1);
	if (!config || Z_TYPE_P(config) == IS_NULL || (Z_TYPE_P(config) == IS_STRING && Z_STRLEN_P(config) == 0)) {
		p_dsn = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_dsn"), 1 TSRMLS_CC);
		if (p_dsn && Z_TYPE_P(p_dsn) == IS_STRING || Z_STRLEN_P(p_dsn) > 0) {
			yod_application_config(Z_STRVAL_P(p_dsn), Z_STRLEN_P(p_dsn), config1 TSRMLS_CC);
		}
	} else {
		ZVAL_ZVAL(config1, config, 1, 0);
	}

	MAKE_STD_ZVAL(yoddb);
	if (yod_database_getinstance(config1, yoddb TSRMLS_CC)) {
		MAKE_STD_ZVAL(prefix);
		yod_database_config(yoddb, ZEND_STRL("prefix"), NULL, prefix TSRMLS_CC);
		zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_prefix"), prefix TSRMLS_CC);
		zval_ptr_dtor(&prefix);
	}
	zval_ptr_dtor(&config1);

	zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_db"), yoddb TSRMLS_CC);
	zval_ptr_dtor(&yoddb);

#if PHP_YOD_DEBUG
	yod_debugf("yod_model_init()");
#endif

	if (zend_hash_exists(&(Z_OBJCE_P(object)->function_table), ZEND_STRS("init"))) {
		zend_call_method_with_0_params(&object, Z_OBJCE_P(object), NULL, "init", NULL);
	}

	return 1;
}
/* }}} */

/** {{{ int yod_model_getinstance(char *name, uint name_len, zval *config, zval *retval TSRMLS_DC)
*/
int yod_model_getinstance(char *name, uint name_len, zval *config, zval *retval TSRMLS_DC) {
	yod_model_t *object;
	zval *model, *model1, *name1, *pzval, **ppval;
	char *upname, *classname, *classpath;
	uint classname_len;
	zend_class_entry **pce = NULL;

#if PHP_YOD_DEBUG
	yod_debugf("yod_model_getinstance(%s)", name ? name : "");
#endif

	upname = estrndup(name, name_len);
	if (name_len == 0) {
		classname_len = spprintf(&classname, 0, "Yod_Model");
	} else {
		*upname = toupper(*upname);
		classname_len = spprintf(&classname, 0, "%sModel", upname);
	}
	
	model = zend_read_static_property(yod_model_ce, ZEND_STRL("_model"), 1 TSRMLS_CC);
	if (model && Z_TYPE_P(model) == IS_ARRAY) {
		if (zend_hash_find(Z_ARRVAL_P(model), upname, name_len + 1, (void **)&ppval) == SUCCESS) {
			if (retval) {
				ZVAL_ZVAL(retval, *ppval, 1, 0);
			}
			efree(classname);
			efree(upname);
			return 1;
		}
	}

	MAKE_STD_ZVAL(object);
	if (name_len > 0) {
		spprintf(&classpath, 0, "%s/models/%s.php", yod_runpath(TSRMLS_C), classname);
		if (VCWD_ACCESS(classpath, F_OK) == 0) {
			yod_include(classpath, &pzval, 1 TSRMLS_CC);
#if PHP_API_VERSION < 20100412
			if (zend_lookup_class_ex(classname, classname_len, 0, &pce TSRMLS_CC) == SUCCESS) {
#else
			if (zend_lookup_class_ex(classname, classname_len, NULL, 0, &pce TSRMLS_CC) == SUCCESS) {
#endif
				object_init_ex(object, *pce);
				if (zend_hash_exists(&(*pce)->function_table, ZEND_STRS(ZEND_CONSTRUCTOR_FUNC_NAME))) {
					MAKE_STD_ZVAL(name1);
					ZVAL_STRINGL(name1, upname, name_len, 1);
					if (config) {
						zend_call_method_with_2_params(&object, *pce, &(*pce)->constructor, ZEND_CONSTRUCTOR_FUNC_NAME, NULL, name1, config);
					} else {
						zend_call_method_with_1_params(&object, *pce, &(*pce)->constructor, ZEND_CONSTRUCTOR_FUNC_NAME, NULL, name1);
					}
					zval_ptr_dtor(&name1);
				}
			} else {
				php_error_docref(NULL TSRMLS_CC, E_ERROR, "Class '%s' not found", classname);
				if (retval) {
					ZVAL_BOOL(retval, 0);
				}
				efree(classname);
				efree(upname);
				return 0;
			}
		} else {
#if PHP_API_VERSION < 20100412
			if (zend_lookup_class_ex(classname, classname_len, 0, &pce TSRMLS_CC) == SUCCESS) {
#else
			if (zend_lookup_class_ex(classname, classname_len, NULL, 0, &pce TSRMLS_CC) == SUCCESS) {
#endif
				object_init_ex(object, *pce);
				if (zend_hash_exists(&(*pce)->function_table, ZEND_STRS(ZEND_CONSTRUCTOR_FUNC_NAME))) {
					MAKE_STD_ZVAL(name1);
					ZVAL_STRINGL(name1, upname, name_len, 1);
					if (config) {
						zend_call_method_with_2_params(&object, *pce, &(*pce)->constructor, ZEND_CONSTRUCTOR_FUNC_NAME, NULL, name1, config);
					} else {
						zend_call_method_with_1_params(&object, *pce, &(*pce)->constructor, ZEND_CONSTRUCTOR_FUNC_NAME, NULL, name1);
					}
					zval_ptr_dtor(&name1);
				}
			} else {
				object_init_ex(object, yod_model_ce);
				yod_model_construct(object, upname, name_len, config TSRMLS_CC);
			}
		}
		efree(classpath);
	} else {
		object_init_ex(object, yod_model_ce);
		yod_model_construct(object, upname, name_len, config TSRMLS_CC);
	}

	if (Z_TYPE_P(object) == IS_OBJECT) {
		MAKE_STD_ZVAL(model1);
		if (model && Z_TYPE_P(model) == IS_ARRAY) {
			ZVAL_ZVAL(model1, model, 1, 0);
		} else {
			array_init(model1);
		}
		add_assoc_zval_ex(model1, upname, name_len + 1, object);
		zend_update_static_property(yod_model_ce, ZEND_STRL("_model"), model1 TSRMLS_CC);
		if (retval) {
			if (Z_TYPE_P(object) == IS_OBJECT) {
				ZVAL_ZVAL(retval, object, 1, 0);
			}
		}
		zval_ptr_dtor(&model1);
		efree(classname);
		efree(upname);
		return 1;
	}
	efree(classname);
	efree(upname);

	if (retval) {
		ZVAL_BOOL(retval, 0);
	}
	return 0;
}
/* }}} */

/** {{{ proto public Yod_Model::__construct($name = '', $config = null)
*/
PHP_METHOD(yod_model, __construct) {
	char *name = NULL;
	uint name_len = 0;
	zval *config = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sz!", &name, &name_len, &config) == FAILURE) {
		return;
	}

	yod_model_construct(getThis(), name, name_len, config TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_Model::init()
*/
PHP_METHOD(yod_model, init) {

}
/* }}} */

/** {{{ proto public Yod_Model::getInstance($name = '', $config = null)
*/
PHP_METHOD(yod_model, getInstance) {
	char *name = NULL;
	uint name_len = 0;
	zval *config = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sz!", &name, &name_len, &config) == FAILURE) {
		return;
	}

	yod_model_getinstance(name, name_len, config, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_Model::find($where = '', $params = array(), $select = '*')
*/
PHP_METHOD(yod_model, find) {
	yod_database_t *yoddb;
	yod_model_t *object;
	zval *table, *result, *retval;
	zval *params = NULL, *select = NULL;
	char *where = NULL;
	uint where_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|szz!", &where, &where_len, &params, &select) == FAILURE) {
		return;
	}

#if PHP_YOD_DEBUG
	yod_debugf("yod_model_find(%s)", where ? where : "");
#endif

	object = getThis();

	yoddb = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_db"), 1 TSRMLS_CC);
	if (!yoddb || Z_TYPE_P(yoddb) != IS_OBJECT) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Call to a member function select() on a non-object");
		RETURN_FALSE;
	}

	table = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_table"), 1 TSRMLS_CC);
	if (table) {
		convert_to_string(table);

		MAKE_STD_ZVAL(result);
		yod_database_select(yoddb, select, Z_STRVAL_P(table), Z_STRLEN_P(table), where, where_len, params, ZEND_STRL("LIMIT 1"), result TSRMLS_CC);
		if (result) {
			zend_call_method_with_1_params(&yoddb, Z_OBJCE_P(yoddb), NULL, "fetch", &retval, result);
			zend_call_method_with_0_params(&yoddb, Z_OBJCE_P(yoddb), NULL, "free", NULL);
			zval_ptr_dtor(&result);
			if (retval) {
				RETURN_ZVAL(retval, 1, 1);
			}
		}
	}
	
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yod_Model::findAll($where = '', $params = array(), $select = '*')
*/
PHP_METHOD(yod_model, findAll) {
	yod_database_t *yoddb;
	yod_model_t *object;
	zval *table, *result, *retval;
	zval *params = NULL, *select = NULL;
	char *where = NULL;
	uint where_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|szz!", &where, &where_len, &params, &select) == FAILURE) {
		return;
	}

#if PHP_YOD_DEBUG
	yod_debugf("yod_model_findall(%s)", where ? where : "");
#endif

	object = getThis();

	yoddb = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_db"), 1 TSRMLS_CC);
	if (!yoddb || Z_TYPE_P(yoddb) != IS_OBJECT) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Call to a member function select() on a non-object");
		RETURN_FALSE;
	}

	table = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_table"), 1 TSRMLS_CC);
	if (table) {
		convert_to_string(table);

		MAKE_STD_ZVAL(result);
		yod_database_select(yoddb, select, Z_STRVAL_P(table), Z_STRLEN_P(table), where, where_len, params, ZEND_STRL(""), result TSRMLS_CC);
		if (result) {
			zend_call_method_with_1_params(&yoddb, Z_OBJCE_P(yoddb), NULL, "fetchall", &retval, result);
			zend_call_method_with_0_params(&yoddb, Z_OBJCE_P(yoddb), NULL, "free", NULL);
			zval_ptr_dtor(&result);
			if (retval) {
				RETURN_ZVAL(retval, 1, 1);
			}
		}
	}
	
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yod_Model::count($where = '', $params = array())
*/
PHP_METHOD(yod_model, count) {
	yod_database_t *yoddb;
	yod_model_t *object;
	zval *table, *result, *data, **data1;
	zval *params = NULL, *select;
	char *where = NULL;
	uint where_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sz!", &where, &where_len, &params) == FAILURE) {
		return;
	}

#if PHP_YOD_DEBUG
	yod_debugf("yod_model_count(%s)", where ? where : "");
#endif

	object = getThis();

	yoddb = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_db"), 1 TSRMLS_CC);
	if (!yoddb || Z_TYPE_P(yoddb) != IS_OBJECT) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Call to a member function select() on a non-object");
		RETURN_FALSE;
	}

	table = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_table"), 1 TSRMLS_CC);
	if (table) {
		convert_to_string(table);

		MAKE_STD_ZVAL(result);
		MAKE_STD_ZVAL(select);
		ZVAL_STRING(select, "COUNT(*)", 1);
		yod_database_select(yoddb, select, Z_STRVAL_P(table), Z_STRLEN_P(table), where, where_len, params, ZEND_STRL("LIMIT 1"), result TSRMLS_CC);
		zval_ptr_dtor(&select);
		if (result) {
			zend_call_method_with_1_params(&yoddb, Z_OBJCE_P(yoddb), NULL, "fetch", &data, result);
			zend_call_method_with_0_params(&yoddb, Z_OBJCE_P(yoddb), NULL, "free", NULL);
			zval_ptr_dtor(&result);
			if (data) {
				if (Z_TYPE_P(data) == IS_ARRAY &&
					zend_hash_get_current_data(Z_ARRVAL_P(data), (void **) &data1) == SUCCESS
				) {
					ZVAL_ZVAL(return_value, *data1, 1, 1);
					zval_ptr_dtor(&data);
					return;
				}
				zval_ptr_dtor(&data);
			}
		}
	}
	
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yod_Model::save($data, $where = '', $params = array())
*/
PHP_METHOD(yod_model, save) {
	yod_database_t *yoddb;
	yod_model_t *object;
	zval *table, *data = NULL, *params = NULL;
	char *where = NULL;
	uint where_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|sz!", &data, &where, &where_len, &params) == FAILURE) {
		return;
	}

#if PHP_YOD_DEBUG
	yod_debugf("yod_model_save(%s)", where ? where : "");
#endif

	if (!data || Z_TYPE_P(data) != IS_ARRAY) {
		RETURN_FALSE;
	}
	
	object = getThis();

	yoddb = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_db"), 1 TSRMLS_CC);
	if (!yoddb || Z_TYPE_P(yoddb) != IS_OBJECT) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Call to a member function update() on a non-object");
		RETURN_FALSE;
	}

	table = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_table"), 1 TSRMLS_CC);
	if (table) {
		convert_to_string(table);
		if (where_len == 0) {
			yod_database_insert(yoddb, data, Z_STRVAL_P(table), Z_STRLEN_P(table), 0, return_value TSRMLS_CC);
		} else {
			yod_database_update(yoddb, data, Z_STRVAL_P(table), Z_STRLEN_P(table), where, where_len, params, return_value TSRMLS_CC);
		}
		return;
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yod_Model::remove($where, $params = array())
*/
PHP_METHOD(yod_model, remove) {
	yod_database_t *yoddb;
	yod_model_t *object;
	zval *table, *params = NULL;
	char *where = NULL;
	uint where_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sz!", &where, &where_len, &params) == FAILURE) {
		return;
	}

#if PHP_YOD_DEBUG
	yod_debugf("yod_model_remove(%s)", where ? where : "");
#endif

	object = getThis();

	yoddb = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_db"), 1 TSRMLS_CC);
	if (!yoddb || Z_TYPE_P(yoddb) != IS_OBJECT) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Call to a member function delete() on a non-object");
		RETURN_FALSE;
	}

	table = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_table"), 1 TSRMLS_CC);
	if (table) {
		convert_to_string(table);
		yod_database_delete(yoddb, Z_STRVAL_P(table), Z_STRLEN_P(table), where, where_len, params, return_value TSRMLS_CC);
		return;
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yod_Model::lastQuery()
*/
PHP_METHOD(yod_model, lastQuery) {
	yod_database_t *yoddb;
	yod_model_t *object;
	zval *retval;

#if PHP_YOD_DEBUG
	yod_debugf("yod_model_lastquery()");
#endif

	object = getThis();

	yoddb = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_db"), 1 TSRMLS_CC);
	if (yoddb && Z_TYPE_P(yoddb) == IS_OBJECT) {
		zend_call_method_with_0_params(&yoddb, Z_OBJCE_P(yoddb), NULL, "lastquery", &retval);
		if (retval) {
			RETURN_ZVAL(retval, 1, 1);
		}
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Call to a member function lastQuery() on a non-object");
		RETURN_NULL();
	}
}
/* }}} */

/** {{{ proto public Yod_Model::config($name = null)
*/
PHP_METHOD(yod_model, config) {
	char *name = NULL;
	uint name_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &name, &name_len) == FAILURE) {
		return;
	}

	yod_application_config(name, name_len, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_Model::import($alias, $classext = '.class.php')
*/
PHP_METHOD(yod_model, import) {
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

/** {{{ proto public Yod_Model::model($name = '', $config = null)
*/
PHP_METHOD(yod_model, model) {
	zval *config = NULL;
	char *name = NULL;
	uint name_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sz!", &name, &name_len, &config) == FAILURE) {
		return;
	}

#if PHP_YOD_DEBUG
	yod_debugf("yod_model_model(%s)", name ? name : "");
#endif

	if (name_len == 0) {
		RETURN_ZVAL(getThis(), 1, 0);
	}

	yod_model_getinstance(name, name_len, config, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_Model::dbmodel($name = '', $config = null)
*/
PHP_METHOD(yod_model, dbmodel) {
	zval *config = NULL;
	char *name = NULL;
	uint name_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sz!", &name, &name_len, &config) == FAILURE) {
		return;
	}

#if PHP_YOD_DEBUG
	yod_debugf("yod_model_dbmodel(%s)", name ? name : "");
#endif

	yod_dbmodel_getinstance(name, name_len, config, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_Model::__destruct()
*/
PHP_METHOD(yod_model, __destruct) {

}
/* }}} */

/** {{{ yod_model_methods[]
*/
zend_function_entry yod_model_methods[] = {
	PHP_ME(yod_model, __construct,		yod_model_construct_arginfo,	ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(yod_model, init,				yod_model_init_arginfo,			ZEND_ACC_PROTECTED)
	PHP_ME(yod_model, getInstance,		yod_model_getinstance_arginfo,	ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod_model, find,				yod_model_find_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_model, findAll,			yod_model_findall_arginfo,		ZEND_ACC_PUBLIC)
	PHP_ME(yod_model, count,			yod_model_count_arginfo,		ZEND_ACC_PUBLIC)
	PHP_ME(yod_model, save,				yod_model_save_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_model, remove,			yod_model_remove_arginfo,		ZEND_ACC_PUBLIC)
	PHP_ME(yod_model, lastQuery,		yod_model_lastquery_arginfo,	ZEND_ACC_PUBLIC)
	PHP_ME(yod_model, config,			yod_model_config_arginfo,		ZEND_ACC_PROTECTED)
	PHP_ME(yod_model, import,			yod_model_import_arginfo,		ZEND_ACC_PROTECTED)
	PHP_ME(yod_model, model,			yod_model_model_arginfo,		ZEND_ACC_PROTECTED)
	PHP_ME(yod_model, dbmodel,			yod_model_dbmodel_arginfo,		ZEND_ACC_PROTECTED)
	PHP_ME(yod_model, __destruct,		NULL,	ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ PHP_MINIT_FUNCTION
*/
PHP_MINIT_FUNCTION(yod_model) {
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "Yod_Model", yod_model_methods);
	yod_model_ce = zend_register_internal_class(&ce TSRMLS_CC);

	zend_declare_property_null(yod_model_ce, ZEND_STRL("_model"), ZEND_ACC_PROTECTED|ZEND_ACC_STATIC TSRMLS_CC);
	zend_declare_property_null(yod_model_ce, ZEND_STRL("_db"), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_string(yod_model_ce, ZEND_STRL("_dsn"), "db_dsn", ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yod_model_ce, ZEND_STRL("_name"), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yod_model_ce, ZEND_STRL("_table"), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yod_model_ce, ZEND_STRL("_prefix"), ZEND_ACC_PROTECTED TSRMLS_CC);

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
