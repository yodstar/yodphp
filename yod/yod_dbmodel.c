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
#include "Zend/zend_interfaces.h"
#include "ext/standard/php_array.h"

#include "php_yod.h"
#include "yod_model.h"
#include "yod_dbmodel.h"
#include "yod_database.h"
#include "yod_dbpdo.h"

#if PHP_YOD_DEBUG
#include "yod_debug.h"
#endif

zend_class_entry *yod_dbmodel_ce;

/** {{{ ARG_INFO
*/
ZEND_BEGIN_ARG_INFO_EX(yod_dbmodel_construct_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, config)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbmodel_getinstance_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, config)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbmodel_find_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, where)
	ZEND_ARG_INFO(0, params)
	ZEND_ARG_INFO(0, select)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbmodel_select_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, where)
	ZEND_ARG_INFO(0, params)
	ZEND_ARG_INFO(0, select)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbmodel_count_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, where)
	ZEND_ARG_INFO(0, params)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbmodel_save_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, data)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbmodel_update_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, data)
	ZEND_ARG_INFO(0, where)
	ZEND_ARG_INFO(0, params)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbmodel_remove_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, where)
	ZEND_ARG_INFO(0, params)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbmodel_field_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, select)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbmodel_from_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, table)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbmodel_join_arginfo, 0, 0, 2)
	ZEND_ARG_INFO(0, table)
	ZEND_ARG_INFO(0, where)
	ZEND_ARG_INFO(0, mode)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbmodel_where_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, where)
	ZEND_ARG_INFO(0, params)
	ZEND_ARG_INFO(0, mode)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbmodel_group_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, group)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbmodel_having_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, having)
	ZEND_ARG_INFO(0, params)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbmodel_order_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, order)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbmodel_limit_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, limit)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbmodel_union_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, union)
	ZEND_ARG_INFO(0, params)
	ZEND_ARG_INFO(0, mode)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbmodel_comment_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, comment)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbmodel_params_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, params)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbmodel_parsequery_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, query)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbmodel_initquery_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()
/* }}} */

/** {{{ int yod_dbmodel_initquery(yod_dbmodel_t *object, zval *retval TSRMLS_DC)
*/
static int yod_dbmodel_initquery(yod_dbmodel_t *object, zval *retval TSRMLS_DC) {
	zval *query, *joins, *unions, *params;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbmodel_initquery()");
#endif
	
	if (!object) {
		if (retval) {
			ZVAL_NULL(retval);
		}
		return 0;
	}

	MAKE_STD_ZVAL(query);
	array_init(query);
	add_assoc_stringl_ex(query, ZEND_STRS("SELECT"), "*", 1, 1);
	add_assoc_stringl_ex(query, ZEND_STRS("FROM"), "", 0, 1);
	MAKE_STD_ZVAL(joins);
	array_init(joins);
	add_assoc_zval_ex(query, ZEND_STRS("JOIN"), joins);
	add_assoc_stringl_ex(query, ZEND_STRS("WHERE"), "", 0, 1);
	add_assoc_stringl_ex(query, ZEND_STRS("GROUP BY"), "", 0, 1);
	add_assoc_stringl_ex(query, ZEND_STRS("HAVING"), "", 0, 1);
	add_assoc_stringl_ex(query, ZEND_STRS("ORDER BY"), "", 0, 1);
	add_assoc_stringl_ex(query, ZEND_STRS("LIMIT"), "", 0, 1);
	MAKE_STD_ZVAL(unions);
	array_init(unions);
	add_assoc_zval_ex(query, ZEND_STRS("UNION"), unions);
	add_assoc_stringl_ex(query, ZEND_STRS("COMMENT"), "", 0, 1);
	zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_query"), query TSRMLS_CC);
	if (retval) {
		ZVAL_ZVAL(retval, query, 1, 0);
	}
	zval_ptr_dtor(&query);

	MAKE_STD_ZVAL(params);
	array_init(params);
	zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_params"), params TSRMLS_CC);
	zval_ptr_dtor(&params);

	return 1;
}
/* }}} */

/** {{{ int yod_dbmodel_parsequery(yod_dbmodel_t *object, zval *query, zval *retval TSRMLS_DC)
*/
static int yod_dbmodel_parsequery(yod_dbmodel_t *object, zval *query, zval *retval TSRMLS_DC) {
	zval *prefix, *table, *zquery, *query1;
	char *table1, *squery1, *squery = NULL;
	uint table1_len, squery_len = 0;
	char *union2, *union1 = NULL;
	uint union1_len = 0;
	zval **data, **data1, **ppval;
	HashPosition pos, pos1;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbmodel_parsequery()");
#endif

	if (!object) {
		if (retval) {
			ZVAL_BOOL(retval, 0);
		}
		return 0;
	}

	MAKE_STD_ZVAL(zquery);
	if (!query || Z_TYPE_P(query) != IS_ARRAY) {
		query1 = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_query"), 1 TSRMLS_CC);
		if (query1 && Z_TYPE_P(query1) == IS_ARRAY) {
			ZVAL_ZVAL(zquery, query1, 1, 0);
		}
	} else {
		ZVAL_ZVAL(zquery, query, 1, 0);
	}
	
	if (Z_TYPE_P(zquery) != IS_ARRAY) {
		if (retval) {
			ZVAL_BOOL(retval, 0);
		}
		zval_ptr_dtor(&zquery);
		return 0;
	}

	if (zend_hash_find(Z_ARRVAL_P(zquery), ZEND_STRS("FROM"), (void **)&ppval) == FAILURE ||
		Z_TYPE_PP(ppval) != IS_STRING || Z_STRLEN_PP(ppval) == 0
	) {
		table = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_table"), 1 TSRMLS_CC);
		if (!table || Z_TYPE_P(table) != IS_STRING || Z_STRLEN_P(table) == 0) {
			if (retval) {
				ZVAL_BOOL(retval, 0);
			}
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "You have an error in your SQL syntax: table name is empty");
			zval_ptr_dtor(&zquery);
			return 0;
		}

		prefix = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_prefix"), 1 TSRMLS_CC);
		if (prefix && Z_TYPE_P(prefix) == IS_STRING) {
			table1_len = spprintf(&table1, 0, "%s%s AS t1", Z_STRVAL_P(prefix), Z_STRVAL_P(table));
		} else {
			table1_len = spprintf(&table1, 0, "%s AS t1", Z_STRVAL_P(table));
		}
		add_assoc_stringl_ex(zquery, ZEND_STRS("FROM"), table1, table1_len, 1);
		efree(table1);
	}

	zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(zquery), &pos);
	while (zend_hash_get_current_data_ex(Z_ARRVAL_P(zquery), (void **)&data, &pos) == SUCCESS) {
		char *str_key = NULL;
		uint key_len;
		ulong num_key;

		if (!data || Z_TYPE_PP(data) == IS_NULL) {
			zend_hash_move_forward_ex(Z_ARRVAL_P(zquery), &pos);
			continue;
		}

		if (zend_hash_get_current_key_ex(Z_ARRVAL_P(zquery), &str_key, &key_len, &num_key, 0, &pos) == HASH_KEY_IS_STRING) {
			if (squery_len && Z_TYPE_PP(data) == IS_ARRAY) {
				if (key_len == 6 && strncmp(str_key, "UNION", 5) == 0) {
					zend_hash_internal_pointer_reset_ex(Z_ARRVAL_PP(data), &pos1);
					while (zend_hash_get_current_data_ex(Z_ARRVAL_PP(data), (void **)&data1, &pos1) == SUCCESS) {
						if (data1 && Z_TYPE_PP(data1) == IS_STRING) {
							if (union1_len) {
								union1_len = spprintf(&union2, 0, "%s %s", union1, Z_STRVAL_PP(data1));
								efree(union1);
							} else {
								union1_len = spprintf(&union2, 0, "%s", Z_STRVAL_PP(data1));
							}
							union1 = union2;
						}
						zend_hash_move_forward_ex(Z_ARRVAL_PP(data), &pos1);
					}
					if (union1_len) {
						squery_len = spprintf(&squery1, 0, "(%s) %s", squery, union1);
						efree(squery);
						squery = squery1;
						efree(union1);
					}
				} else if (key_len == 5 && strncmp(str_key, "JOIN", 4) == 0) {
					zend_hash_internal_pointer_reset_ex(Z_ARRVAL_PP(data), &pos1);
					while (zend_hash_get_current_data_ex(Z_ARRVAL_PP(data), (void **)&data1, &pos1) == SUCCESS) {
						if (data1 && Z_TYPE_PP(data1) == IS_STRING) {
							squery_len = spprintf(&squery1, 0, "%s %s", squery, Z_STRVAL_PP(data1));
							efree(squery);
							squery = squery1;
						}
						zend_hash_move_forward_ex(Z_ARRVAL_PP(data), &pos1);
					}
				}
			} else {
				if (Z_TYPE_PP(data) != IS_STRING) {
					convert_to_string(*data);
				}
				if (Z_STRLEN_PP(data) == 0) {
					zend_hash_move_forward_ex(Z_ARRVAL_P(zquery), &pos);
					continue;
				}

				if (key_len == 7 && strncmp(str_key, "SELECT", 6) == 0) {
					squery_len = spprintf(&squery, 0, "SELECT %s", Z_STRVAL_PP(data));
				} else if (squery_len) {
					if (key_len == 8 && strncmp(str_key, "COMMENT", 7) == 0) {
						squery_len = spprintf(&squery1, 0, "%s /* %s */", squery, Z_STRVAL_PP(data));
					} else {
						squery_len = spprintf(&squery1, 0, "%s %s %s", squery, str_key, Z_STRVAL_PP(data));
					}
					efree(squery);
					squery = squery1;
				}
			}
		}

		zend_hash_move_forward_ex(Z_ARRVAL_P(zquery), &pos);
	}
	zval_ptr_dtor(&zquery);

	if (squery_len == 0) {
		if (retval) {
			ZVAL_BOOL(retval, 0);
		}
		return 0;
	}

	if (retval) {
		ZVAL_STRINGL(retval, squery, squery_len, 1);
	}
	efree(squery);
	return 1;
}
/* }}} */

/** {{{ int yod_dbmodel_construct(yod_dbmodel_t *object, char *name, uint name_len, zval *config TSRMLS_DC)
*/
int yod_dbmodel_construct(yod_dbmodel_t *object, char *name, uint name_len, zval *config TSRMLS_DC) {
	zval *model, *model1;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbmodel_construct(%s)", name ? name : "");
#endif

	if (!object) {
		return 0;
	}

	yod_model_construct(object, name, name_len, config TSRMLS_CC);

	yod_dbmodel_initquery(object, NULL TSRMLS_CC);

	MAKE_STD_ZVAL(model1);
	model = zend_read_static_property(yod_dbmodel_ce, ZEND_STRL("_model"), 1 TSRMLS_CC);
	if (model && Z_TYPE_P(model) == IS_ARRAY) {
		ZVAL_ZVAL(model1, model, 1, 0);
	} else {
		array_init(model1);
	}
	zval_add_ref(&object);
	add_assoc_zval_ex(model1, (name_len ? name : ""), name_len + 1, object);
	zend_update_static_property(yod_dbmodel_ce, ZEND_STRL("_model"), model1 TSRMLS_CC);
	zval_ptr_dtor(&model1);

	return 1;
}
/* }}} */

/** {{{ int yod_dbmodel_getinstance(char *name, uint name_len, zval *config, zval *retval TSRMLS_DC)
*/
int yod_dbmodel_getinstance(char *name, uint name_len, zval *config, zval *retval TSRMLS_DC) {
	yod_dbmodel_t *object;
	zval *model, *model1, **ppval;
	char *upname;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbmodel_getinstance(%s)", name ? name : "");
#endif

	upname = estrndup(name, name_len);

	model = zend_read_static_property(yod_dbmodel_ce, ZEND_STRL("_model"), 1 TSRMLS_CC);
	if (model && Z_TYPE_P(model) == IS_ARRAY) {
		if (zend_hash_find(Z_ARRVAL_P(model), upname, name_len + 1, (void **)&ppval) == SUCCESS) {
			if (retval) {
				ZVAL_ZVAL(retval, *ppval, 1, 0);
			}
			efree(upname);
			return 1;
		}
	}

	MAKE_STD_ZVAL(object);
	object_init_ex(object, yod_dbmodel_ce);
	yod_dbmodel_construct(object, upname, name_len, config TSRMLS_CC);

	MAKE_STD_ZVAL(model1);
	if (model && Z_TYPE_P(model) == IS_ARRAY) {
		ZVAL_ZVAL(model1, model, 1, 0);
	} else {
		array_init(model1);
	}
	add_assoc_zval_ex(model1, upname, name_len + 1, object);
	zend_update_static_property(yod_dbmodel_ce, ZEND_STRL("_model"), model1 TSRMLS_CC);
	if (retval) {
		ZVAL_ZVAL(retval, object, 1, 0);
	}
	zval_ptr_dtor(&model1);
	efree(upname);

	return 1;
}
/* }}} */

/** {{{ int yod_dbmodel_params(yod_dbmodel_t *object, zval *params TSRMLS_DC)
*/
static int yod_dbmodel_params(yod_dbmodel_t *object, zval *params TSRMLS_DC) {
	zval *params1;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbmodel_params()");
#endif

	if (!params || Z_TYPE_P(params) != IS_ARRAY || !object) {
		return 0;
	}

	params1 = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_params"), 1 TSRMLS_CC);
	if (params1 && Z_TYPE_P(params1) == IS_ARRAY) {
		php_array_merge(Z_ARRVAL_P(params), Z_ARRVAL_P(params1), 0 TSRMLS_CC);
	}

	zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_params"), params TSRMLS_CC);

	return 1;
}
/* }}} */

/** {{{ int yod_dbmodel_field(yod_dbmodel_t *object, zval *select TSRMLS_DC)
*/
static int yod_dbmodel_field(yod_dbmodel_t *object, zval *select TSRMLS_DC) {
	zval *query, **data;
	char *str_key, *fields, *fields1 = NULL;
	uint key_len, fields_len = 0;
	int key_type;
	ulong num_key;
	HashPosition pos;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbmodel_field()");
#endif

	if (!select || !object) {
		return 0;
	}

	query = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_query"), 1 TSRMLS_CC);
	if (!query || Z_TYPE_P(query) != IS_ARRAY) {
		return 0;
	}

	if (Z_TYPE_P(select) == IS_ARRAY) {
		zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(select), &pos);
		while (zend_hash_get_current_data_ex(Z_ARRVAL_P(select), (void **)&data, &pos) == SUCCESS) {
			if (Z_TYPE_PP(data) == IS_STRING) {
				key_type = zend_hash_get_current_key_ex(Z_ARRVAL_P(select), &str_key, &key_len, &num_key, 0, &pos);
				if (key_type == HASH_KEY_IS_STRING) {
					fields_len = spprintf(&fields, 0, "%s%s AS %s, ", (fields1 ? fields1 : ""), Z_STRVAL_PP(data), str_key);
				} else {
					fields_len = spprintf(&fields, 0, "%s%s, ", (fields1 ? fields1 : ""), Z_STRVAL_PP(data));
				}

				if (fields1) {
					efree(fields1);
				}
				fields1 = fields;
			}
			zend_hash_move_forward_ex(Z_ARRVAL_P(select), &pos);
		}
		if (fields_len > 2) {
			fields_len = fields_len - 2;
			fields = estrndup(fields1, fields_len);
			add_assoc_stringl_ex(query, ZEND_STRS("SELECT"), fields, fields_len, 1);
			zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_query"), query TSRMLS_CC);
			efree(fields1);
			efree(fields);
			return 1;
		}
	}

	if (Z_TYPE_P(select) == IS_STRING && Z_STRLEN_P(select)) {
		add_assoc_stringl_ex(query, ZEND_STRS("SELECT"), Z_STRVAL_P(select), Z_STRLEN_P(select), 1);
		zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_query"), query TSRMLS_CC);
		return 1;
	}

	return 0;
}
/* }}} */

/** {{{ int yod_dbmodel_from(yod_dbmodel_t *object, char *table, uint table_len TSRMLS_DC)
*/
static int yod_dbmodel_from(yod_dbmodel_t *object, char *table, uint table_len TSRMLS_DC) {
	zval *query, *prefix;
	char *table1;
	uint table1_len;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbmodel_from(%s)", table ? table : "");
#endif

	if (!table || !object) {
		return 0;
	}

	query = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_query"), 1 TSRMLS_CC);
	if (!query || Z_TYPE_P(query) != IS_ARRAY) {
		return 0;
	}

	prefix = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_prefix"), 1 TSRMLS_CC);
	if (prefix && Z_TYPE_P(prefix) == IS_STRING && Z_STRLEN_P(prefix)) {
		table1_len = spprintf(&table1, 0, "%s%s AS t1", Z_STRVAL_P(prefix), table);
	} else {
		table1_len = spprintf(&table1, 0, "%s AS t1", table);
	}
	add_assoc_stringl_ex(query, ZEND_STRS("FROM"), table1, table1_len, 1);
	zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_query"), query TSRMLS_CC);
	efree(table1);

	return 1;
}
/* }}} */

/** {{{ int yod_dbmodel_join(yod_dbmodel_t *object, char *table, uint table_len, char *where, uint where_len, char *mode, uint mode_len TSRMLS_DC)
*/
static int yod_dbmodel_join(yod_dbmodel_t *object, char *table, uint table_len, char *where, uint where_len, char *mode, uint mode_len TSRMLS_DC) {
	zval *query, *join1, *prefix, **ppval;
	char *join = NULL;
	uint join_len = 0;
	long num_elem;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbmodel_join(%s%s%s)", (table ? table : ""), (where_len ? ", " : ""), (where ? where : ""));
#endif

	if (!table || !object) {
		return 0;
	}

	query = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_query"), 1 TSRMLS_CC);
	if (!query || Z_TYPE_P(query) != IS_ARRAY) {
		return 0;
	}

	MAKE_STD_ZVAL(join1);
	if (zend_hash_find(Z_ARRVAL_P(query), ZEND_STRS("JOIN"), (void **)&ppval) == SUCCESS &&
		Z_TYPE_PP(ppval) == IS_ARRAY
	) {
		ZVAL_ZVAL(join1, *ppval, 1, 0);
	} else {
		array_init(join1);
	}
	num_elem = zend_hash_num_elements(Z_ARRVAL_P(join1)) + 2;

	prefix = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_prefix"), 1 TSRMLS_CC);
	if (prefix && Z_TYPE_P(prefix) == IS_STRING && Z_STRLEN_P(prefix)) {
		join_len = spprintf(&join, 0, "%s JOIN %s%s AS t%d%s%s", (mode ? mode : "LEFT"), Z_STRVAL_P(prefix), table, num_elem, (where_len ? " ON " : ""), (where_len ? where : ""));
	} else {
		join_len = spprintf(&join, 0, "%s JOIN %s AS t%d%s%s", (mode ? mode : "LEFT"), table, num_elem, (where_len ? " ON " : ""), (where_len ? where : ""));
	}
	add_next_index_stringl(join1, join, join_len, 1);
	add_assoc_zval_ex(query, ZEND_STRS("JOIN"), join1);
	zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_query"), query TSRMLS_CC);
	efree(join);

	return 1;
}
/* }}} */

/** {{{ int yod_dbmodel_where(yod_dbmodel_t *object, char *where, uint where_len, zval *params, char *mode, uint mode_len TSRMLS_DC)
*/
static int yod_dbmodel_where(yod_dbmodel_t *object, char *where, uint where_len, zval *params, char *mode, uint mode_len TSRMLS_DC) {
	zval *query, **ppval;
	char *where1;
	uint where1_len;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbmodel_where(%s)", where ? where : "");
#endif

	if (where_len == 0 || !where || !object) {
		return 0;
	}

	query = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_query"), 1 TSRMLS_CC);
	if (!query || Z_TYPE_P(query) != IS_ARRAY) {
		return 0;
	}

	if (zend_hash_find(Z_ARRVAL_P(query), ZEND_STRS("WHERE"), (void **)&ppval) == FAILURE) {
		return 0;
	}

	if (params) {
		if (Z_TYPE_P(params) == IS_STRING && Z_STRLEN_P(params)) {
			mode_len = Z_STRLEN_P(params);
			mode = Z_STRVAL_P(params);
		} else if (Z_TYPE_P(params) == IS_ARRAY) {
			yod_dbmodel_params(object, params TSRMLS_CC);
		}
	}

	if (Z_TYPE_PP(ppval) == IS_STRING && Z_STRLEN_PP(ppval)) {
		where1_len = spprintf(&where1, 0, "(%s) %s (%s)", Z_STRVAL_PP(ppval), (mode_len ? mode : "AND"), where);
		add_assoc_stringl_ex(query, ZEND_STRS("WHERE"), where1, where1_len, 1);
		efree(where1);
	} else {
		add_assoc_stringl_ex(query, ZEND_STRS("WHERE"), where, where_len, 1);
	}
	zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_query"), query TSRMLS_CC);

	return 1;
}
/* }}} */

/** {{{ int yod_dbmodel_group(yod_dbmodel_t *object, char *group, uint group_len TSRMLS_DC)
*/
static int yod_dbmodel_group(yod_dbmodel_t *object, char *group, uint group_len TSRMLS_DC) {
	zval *query;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbmodel_group(%s)", group ? group : "");
#endif

	if (!group || !object) {
		return 0;
	}

	query = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_query"), 1 TSRMLS_CC);
	if (!query || Z_TYPE_P(query) != IS_ARRAY) {
		return 0;
	}

	add_assoc_stringl_ex(query, ZEND_STRS("GROUP BY"), group, group_len, 1);
	zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_query"), query TSRMLS_CC);

	return 1;
}
/* }}} */

/** {{{ int yod_dbmodel_having(yod_dbmodel_t *object, char *having, uint having_len, zval *params TSRMLS_DC)
*/
static int yod_dbmodel_having(yod_dbmodel_t *object, char *having, uint having_len, zval *params TSRMLS_DC) {
	zval *query;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbmodel_having(%s)", having ? having : "");
#endif

	if (!having || !object) {
		return 0;
	}

	query = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_query"), 1 TSRMLS_CC);
	if (!query || Z_TYPE_P(query) != IS_ARRAY) {
		return 0;
	}

	add_assoc_stringl_ex(query, ZEND_STRS("HAVING"), having, having_len, 1);
	zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_query"), query TSRMLS_CC);

	if (params && Z_TYPE_P(params) == IS_ARRAY) {
		yod_dbmodel_params(object, params TSRMLS_CC);
	}

	return 1;
}
/* }}} */

/** {{{ int yod_dbmodel_order(yod_dbmodel_t *object, char *order, uint order_len TSRMLS_DC)
*/
static int yod_dbmodel_order(yod_dbmodel_t *object, char *order, uint order_len TSRMLS_DC) {
	zval *query;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbmodel_order(%s)", order ? order : "");
#endif

	if (!order || !object) {
		return 0;
	}

	query = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_query"), 1 TSRMLS_CC);
	if (!query || Z_TYPE_P(query) != IS_ARRAY) {
		return 0;
	}

	add_assoc_stringl_ex(query, ZEND_STRS("ORDER BY"), order, order_len, 1);
	zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_query"), query TSRMLS_CC);

	return 1;
}
/* }}} */

/** {{{ int yod_dbmodel_limit(yod_dbmodel_t *object, char *limit, uint limit_len TSRMLS_DC)
*/
static int yod_dbmodel_limit(yod_dbmodel_t *object, char *limit, uint limit_len TSRMLS_DC) {
	zval *query;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbmodel_limit(%s)", limit ? limit : "");
#endif

	if (!limit || !object) {
		return 0;
	}

	query = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_query"), 1 TSRMLS_CC);
	if (!query || Z_TYPE_P(query) != IS_ARRAY) {
		return 0;
	}

	add_assoc_stringl_ex(query, ZEND_STRS("LIMIT"), limit, limit_len, 1);
	zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_query"), query TSRMLS_CC);

	return 1;
}
/* }}} */

/** {{{ int yod_dbmodel_union(yod_dbmodel_t *object, zval *unions, zval *params, char *mode, uint mode_len TSRMLS_DC)
*/
static int yod_dbmodel_union(yod_dbmodel_t *object, zval *unions, zval *params, char *mode, uint mode_len TSRMLS_DC) {
	zval *query, *union1, *unions1, **ppval;
	char *union2;
	uint union2_len;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbmodel_union()");
#endif

	if (!unions || !object) {
		return 0;
	}

	query = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_query"), 1 TSRMLS_CC);
	if (!query || Z_TYPE_P(query) != IS_ARRAY) {
		return 0;
	}

	MAKE_STD_ZVAL(union1);
	if (zend_hash_find(Z_ARRVAL_P(query), ZEND_STRS("UNION"), (void **)&ppval) == SUCCESS &&
		Z_TYPE_PP(ppval) == IS_ARRAY
	) {
		ZVAL_ZVAL(union1, *ppval, 1, 0);
	} else {
		array_init(union1);
	}

	MAKE_STD_ZVAL(unions1);
	if (Z_TYPE_P(unions) == IS_ARRAY) {
		yod_dbmodel_parsequery(object, unions, unions1 TSRMLS_CC);
	} else {
		ZVAL_ZVAL(unions1, unions, 1, 0);
	}
	if (Z_TYPE_P(unions1) != IS_STRING) {
		convert_to_string(unions1);
	}

	if (mode_len > 0) {
		union2_len = spprintf(&union2, 0, "UNION %s (%s)", mode, Z_STRVAL_P(unions1));
	} else {
		union2_len = spprintf(&union2, 0, "UNION (%s)", Z_STRVAL_P(unions1));
	}
	zval_ptr_dtor(&unions1);

	add_next_index_stringl(union1, union2, union2_len, 1);
	add_assoc_zval_ex(query, ZEND_STRS("UNION"), union1);
	zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_query"), query TSRMLS_CC);
	efree(union2);

	if (params && Z_TYPE_P(params) == IS_ARRAY) {
		yod_dbmodel_params(object, params TSRMLS_CC);
	}

	return 1;
}
/* }}} */

/** {{{ int yod_dbmodel_comment(yod_dbmodel_t *object, char *comment, uint comment_len TSRMLS_DC)
*/
static int yod_dbmodel_comment(yod_dbmodel_t *object, char *comment, uint comment_len TSRMLS_DC) {
	zval *query;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbmodel_comment(%s)", comment ? comment : "");
#endif

	if (!comment || !object) {
		return 0;
	}

	query = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_query"), 1 TSRMLS_CC);
	if (!query || Z_TYPE_P(query) != IS_ARRAY) {
		return 0;
	}

	add_assoc_stringl_ex(query, ZEND_STRS("COMMENT"), comment, comment_len, 1);
	zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_query"), query TSRMLS_CC);

	return 1;
}
/* }}} */

/** {{{ int yod_dbmodel_find(yod_dbmodel_t *object, char *where, uint where_len, zval *params, zval *select, zval *retval TSRMLS_DC)
*/
static int yod_dbmodel_find(yod_dbmodel_t *object, char *where, uint where_len, zval *params, zval *select, zval *retval TSRMLS_DC) {
	yod_database_t *yoddb;
	zval *params1, *query, *result, *data;

#if PHP_YOD_DEBUG
	yod_debugl(1 TSRMLS_CC);
	yod_debugf("yod_dbmodel_find()");
#endif

	if (!object) {
		if (retval) {
			ZVAL_BOOL(retval, 0);
		}
		return 0;
	}

	yoddb = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_db"), 1 TSRMLS_CC);
	if (!yoddb || Z_TYPE_P(yoddb) != IS_OBJECT) {
		if (retval) {
			ZVAL_BOOL(retval, 0);
		}
		yod_dbmodel_initquery(object, NULL TSRMLS_CC);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Call to a member function query() on a non-object");
		return 0;
	}

	if (select && ((Z_TYPE_P(select) == IS_STRING && Z_STRLEN_P(select)) || Z_TYPE_P(select) == IS_ARRAY)) {
		yod_dbmodel_field(object, select TSRMLS_CC);
	}
	if (where_len > 0 || (params && Z_TYPE_P(params) == IS_ARRAY)) {
		yod_dbmodel_where(object, where, where_len, params, NULL, 0 TSRMLS_CC);
	}
	yod_dbmodel_limit(object, "1", 1 TSRMLS_CC);

	MAKE_STD_ZVAL(query);
	yod_dbmodel_parsequery(object, NULL, query TSRMLS_CC);

	if (query && Z_TYPE_P(query) == IS_STRING) {

#if PHP_YOD_DEBUG
		yod_debugl(1 TSRMLS_CC);
		yod_debugf("%s", Z_STRVAL_P(query));
		yod_debugl(1 TSRMLS_CC);
#endif

		MAKE_STD_ZVAL(result);
		params1 = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_params"), 1 TSRMLS_CC);
		if (instanceof_function(Z_OBJCE_P(yoddb), yod_dbpdo_ce TSRMLS_CC)) {
			yod_dbpdo_query(yoddb, query, params1, result TSRMLS_CC);
		} else {
#if PHP_YOD_DEBUG
			yod_debugf("yod_database_query()");
#endif
			zend_call_method_with_2_params(&yoddb, Z_OBJCE_P(yoddb), NULL, "query", &result, query, params1);
		}

		if (result) {
			zend_call_method_with_1_params(&yoddb, Z_OBJCE_P(yoddb), NULL, "fetch", &data, result);
			zend_call_method_with_0_params(&yoddb, Z_OBJCE_P(yoddb), NULL, "free", NULL);
			zval_ptr_dtor(&result);
			if (data) {
				zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_data"), data TSRMLS_CC);
				if (retval) {
					ZVAL_ZVAL(retval, data, 1, 1);
					zval_ptr_dtor(&query);
					yod_dbmodel_initquery(object, NULL TSRMLS_CC);
					return 1;
				}
				zval_ptr_dtor(&data);
			}
		}
	}
	zval_ptr_dtor(&query);

	if (retval) {
		ZVAL_BOOL(retval, 0);
	}
	yod_dbmodel_initquery(object, NULL TSRMLS_CC);
	return 0;
}
/* }}} */

/** {{{ int yod_dbmodel_select(yod_dbmodel_t *object, char *where, uint where_len, zval *params, zval *select, zval *retval TSRMLS_DC)
*/
static int yod_dbmodel_select(yod_dbmodel_t *object, char *where, uint where_len, zval *params, zval *select, zval *retval TSRMLS_DC) {
	yod_database_t *yoddb;
	zval *params1, *query, *result, *data;

#if PHP_YOD_DEBUG
	yod_debugl(1 TSRMLS_CC);
	yod_debugf("yod_dbmodel_select()");
#endif

	if (!object) {
		if (retval) {
			ZVAL_BOOL(retval, 0);
		}
		return 0;
	}

	yoddb = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_db"), 1 TSRMLS_CC);
	if (!yoddb || Z_TYPE_P(yoddb) != IS_OBJECT) {
		if (retval) {
			ZVAL_BOOL(retval, 0);
		}
		yod_dbmodel_initquery(object, NULL TSRMLS_CC);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Call to a member function query() on a non-object");
		return 0;
	}

	if (select && ((Z_TYPE_P(select) == IS_STRING && Z_STRLEN_P(select)) || Z_TYPE_P(select) == IS_ARRAY)) {
		yod_dbmodel_field(object, select TSRMLS_CC);
	}
	if (where_len > 0 || (params && Z_TYPE_P(params) == IS_ARRAY)) {
		yod_dbmodel_where(object, where, where_len, params, NULL, 0 TSRMLS_CC);
	}
	
	MAKE_STD_ZVAL(query);
	yod_dbmodel_parsequery(object, NULL, query TSRMLS_CC);

	if (query && Z_TYPE_P(query) == IS_STRING) {

#if PHP_YOD_DEBUG
		yod_debugl(1 TSRMLS_CC);
		yod_debugf("%s", Z_STRVAL_P(query));
		yod_debugl(1 TSRMLS_CC);
#endif

		MAKE_STD_ZVAL(result);
		params1 = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_params"), 1 TSRMLS_CC);
		if (instanceof_function(Z_OBJCE_P(yoddb), yod_dbpdo_ce TSRMLS_CC)) {
			yod_dbpdo_query(yoddb, query, params1, result TSRMLS_CC);
		} else {
#if PHP_YOD_DEBUG
			yod_debugf("yod_database_query()");
#endif
			zend_call_method_with_2_params(&yoddb, Z_OBJCE_P(yoddb), NULL, "query", &result, query, params1);
		}

		if (result) {
			zend_call_method_with_1_params(&yoddb, Z_OBJCE_P(yoddb), NULL, "fetchall", &data, result);
			zend_call_method_with_0_params(&yoddb, Z_OBJCE_P(yoddb), NULL, "free", NULL);
			zval_ptr_dtor(&result);
			if (data) {
				if (retval) {
					ZVAL_ZVAL(retval, data, 1, 1);
					zval_ptr_dtor(&query);
					yod_dbmodel_initquery(object, NULL TSRMLS_CC);
					return 1;
				}
				zval_ptr_dtor(&data);
			}
		}
	}
	zval_ptr_dtor(&query);

	if (retval) {
		ZVAL_BOOL(retval, 0);
	}
	yod_dbmodel_initquery(object, NULL TSRMLS_CC);
	return 0;
}
/* }}} */

/** {{{ int yod_dbmodel_count(yod_dbmodel_t *object, char *where, uint where_len, zval *params, zval *retval TSRMLS_DC)
*/
static int yod_dbmodel_count(yod_dbmodel_t *object, char *where, uint where_len, zval *params, zval *retval TSRMLS_DC) {
	yod_database_t *yoddb;
	zval *select, *params1, *query, *result, *data, **value;

#if PHP_YOD_DEBUG
	yod_debugl(1 TSRMLS_CC);
	yod_debugf("yod_dbmodel_count()");
#endif

	if (!object) {
		if (retval) {
			ZVAL_BOOL(retval, 0);
		}
		return 0;
	}

	yoddb = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_db"), 1 TSRMLS_CC);
	if (!yoddb || Z_TYPE_P(yoddb) != IS_OBJECT) {
		if (retval) {
			ZVAL_BOOL(retval, 0);
		}
		yod_dbmodel_initquery(object, NULL TSRMLS_CC);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Call to a member function query() on a non-object");
		return 0;
	}

	MAKE_STD_ZVAL(select);
	ZVAL_STRINGL(select, "COUNT(*)", 8, 1);
	yod_dbmodel_field(object, select TSRMLS_CC);
	zval_ptr_dtor(&select);

	yod_dbmodel_where(object, where, where_len, params, NULL, 0 TSRMLS_CC);
	yod_dbmodel_limit(object, "1", 1 TSRMLS_CC);

	MAKE_STD_ZVAL(query);
	yod_dbmodel_parsequery(object, NULL, query TSRMLS_CC);

	if (query && Z_TYPE_P(query) == IS_STRING) {

#if PHP_YOD_DEBUG
		yod_debugl(1 TSRMLS_CC);
		yod_debugf("%s", Z_STRVAL_P(query));
		yod_debugl(1 TSRMLS_CC);
#endif

		MAKE_STD_ZVAL(result);
		params1 = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_params"), 1 TSRMLS_CC);
		if (instanceof_function(Z_OBJCE_P(yoddb), yod_dbpdo_ce TSRMLS_CC)) {
			yod_dbpdo_query(yoddb, query, params1, result TSRMLS_CC);
		} else {
#if PHP_YOD_DEBUG
			yod_debugf("yod_database_query()");
#endif
			zend_call_method_with_2_params(&yoddb, Z_OBJCE_P(yoddb), NULL, "query", &result, query, params1);
		}

		if (result) {
			zend_call_method_with_1_params(&yoddb, Z_OBJCE_P(yoddb), NULL, "fetch", &data, result);
			zend_call_method_with_0_params(&yoddb, Z_OBJCE_P(yoddb), NULL, "free", NULL);
			zval_ptr_dtor(&result);
			if (data) {
				if (retval && Z_TYPE_P(data) == IS_ARRAY) {
					if (zend_hash_get_current_data(Z_ARRVAL_P(data), (void **) &value) == FAILURE) {
						ZVAL_LONG(retval, 0);
					} else {
						ZVAL_ZVAL(retval, *value, 1, 1);
					}
				}

				zval_ptr_dtor(&data);
				zval_ptr_dtor(&query);
				yod_dbmodel_initquery(object, NULL TSRMLS_CC);
				return 1;
			}
		}
	}
	zval_ptr_dtor(&query);

	if (retval) {
		ZVAL_BOOL(retval, 0);
	}
	yod_dbmodel_initquery(object, NULL TSRMLS_CC);
	return 0;
}
/* }}} */

/** {{{ int yod_dbmodel_save(yod_dbmodel_t *object, zval *data, zval *retval TSRMLS_DC)
*/
static int yod_dbmodel_save(yod_dbmodel_t *object, zval *data, zval *retval TSRMLS_DC) {
	yod_database_t *yoddb;
	zval *table, *pzval;

#if PHP_YOD_DEBUG
	yod_debugl(1 TSRMLS_CC);
	yod_debugf("yod_dbmodel_save()");
#endif

	if (!data || Z_TYPE_P(data) != IS_ARRAY|| !object) {
		if (retval) {
			ZVAL_BOOL(retval, 0);
		}
		return 0;
	}

	yoddb = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_db"), 1 TSRMLS_CC);
	if (!yoddb || Z_TYPE_P(yoddb) != IS_OBJECT) {
		if (retval) {
			ZVAL_BOOL(retval, 0);
		}
		yod_dbmodel_initquery(object, NULL TSRMLS_CC);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Call to a member function insert() on a non-object");
		return 0;
	}

	table = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_table"), 1 TSRMLS_CC);
	if (!table || Z_TYPE_P(table) != IS_STRING || Z_STRLEN_P(table) == 0) {
		if (retval) {
			ZVAL_BOOL(retval, 0);
		}
		yod_dbmodel_initquery(object, NULL TSRMLS_CC);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "You have an error in your SQL syntax: table name is empty");
		return 0;
	}

	if (yod_database_insert(yoddb, data, Z_STRVAL_P(table), Z_STRLEN_P(table), 0, NULL TSRMLS_CC)) {
		if (retval) {
			zend_call_method_with_0_params(&yoddb, Z_OBJCE_P(yoddb), NULL, "insertid", &pzval);
			if (pzval) {
				if (Z_TYPE_P(pzval) == IS_STRING && strncmp("0", Z_STRVAL_P(pzval), Z_STRLEN_P(pzval)) == 0) {
					ZVAL_BOOL(retval, 1);
				}
				ZVAL_ZVAL(retval, pzval, 1, 1);
			} else {
				ZVAL_BOOL(retval, 0);
			}
		}
		yod_dbmodel_initquery(object, NULL TSRMLS_CC);
		return 1;
	}

	yod_dbmodel_initquery(object, NULL TSRMLS_CC);
	return 0;
}
/* }}} */

/** {{{ int yod_dbmodel_update(yod_dbmodel_t *object, zval *data, char *where, uint where_len, zval *params, zval *retval TSRMLS_DC)
*/
static int yod_dbmodel_update(yod_dbmodel_t *object, zval *data, char *where, uint where_len, zval *params, zval *retval TSRMLS_DC) {
	yod_database_t *yoddb;
	zval *table, *params1, *params2, *query, **data1, **ppval;
	char *where1, *where2;
	HashPosition pos;

#if PHP_YOD_DEBUG
	yod_debugl(1 TSRMLS_CC);
	yod_debugf("yod_dbmodel_update()");
#endif

	if (!data || Z_TYPE_P(data) != IS_ARRAY|| !object) {
		if (retval) {
			ZVAL_BOOL(retval, 0);
		}
		return 0;
	}

	yoddb = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_db"), 1 TSRMLS_CC);
	if (!yoddb || Z_TYPE_P(yoddb) != IS_OBJECT) {
		if (retval) {
			ZVAL_BOOL(retval, 0);
		}
		yod_dbmodel_initquery(object, NULL TSRMLS_CC);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Call to a member function insert() on a non-object");
		return 0;
	}

	table = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_table"), 1 TSRMLS_CC);
	if (!table || Z_TYPE_P(table) != IS_STRING || Z_STRLEN_P(table) == 0) {
		if (retval) {
			ZVAL_BOOL(retval, 0);
		}
		yod_dbmodel_initquery(object, NULL TSRMLS_CC);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "You have an error in your SQL syntax: table name is empty");
		return 0;
	}

	if (where) {
		where1 = estrndup(where, where_len);
	} else {
		where1 = estrndup("", 0);
	}

	MAKE_STD_ZVAL(params1);
	if (params && Z_TYPE_P(params) == IS_ARRAY) {
		ZVAL_ZVAL(params1, params, 1, 0);
	} else {
		array_init(params1);
	}
	zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(data), &pos);
	while (zend_hash_get_current_data_ex(Z_ARRVAL_P(data), (void **)&data1, &pos) == SUCCESS) {
		char *str_key = NULL;
		uint key_len;
		zval *value = NULL;
		ulong num_key;

		if (zend_hash_get_current_key_ex(Z_ARRVAL_P(data), &str_key, &key_len, &num_key, 0, &pos) == HASH_KEY_IS_STRING) {
			if (strncmp(str_key, ":", 1) == 0) {
				if (where_len) {
					where_len = spprintf(&where2, 0, "%s AND %s = %s", where1, str_key + 1, str_key);
				} else {
					where_len = spprintf(&where2, 0, "%s = %s", str_key + 1, str_key);
				}
				efree(where1);
				where1 = where2;

				MAKE_STD_ZVAL(value);
				ZVAL_ZVAL(value, *data1, 1, 0);
				convert_to_string(value);
				add_assoc_zval_ex(params1, str_key, key_len, value);

				zend_hash_move_forward_ex(Z_ARRVAL_P(data), &pos);
				zend_hash_del_key_or_index(Z_ARRVAL_P(data), str_key, key_len, 0, HASH_DEL_KEY);
				continue;
			}
		}
		zend_hash_move_forward_ex(Z_ARRVAL_P(data), &pos);
	}
	yod_dbmodel_where(object, where1, where_len, params1, NULL, 0 TSRMLS_CC);
	zval_ptr_dtor(&params1);
	efree(where1);

	query = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_query"), 1 TSRMLS_CC);
	if (Z_TYPE_P(query) == IS_ARRAY) {
		if (zend_hash_find(Z_ARRVAL_P(query), ZEND_STRS("WHERE"), (void **)&ppval) == FAILURE) {
			yod_dbmodel_initquery(object, NULL TSRMLS_CC);
			return 0;
		}

		params2 = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_params"), 1 TSRMLS_CC);
		if (!ppval || Z_TYPE_PP(ppval) != IS_STRING || Z_STRLEN_PP(ppval) == 0) {
			yod_database_update(yoddb, data, Z_STRVAL_P(table), Z_STRLEN_P(table), NULL, 0, params2, retval TSRMLS_CC);
		} else {
			yod_database_update(yoddb, data, Z_STRVAL_P(table), Z_STRLEN_P(table), Z_STRVAL_PP(ppval), Z_STRLEN_PP(ppval), params2, retval TSRMLS_CC);
		}

		yod_dbmodel_initquery(object, NULL TSRMLS_CC);
		return 1;
	}

	yod_dbmodel_initquery(object, NULL TSRMLS_CC);
	return 0;
}
/* }}} */

/** {{{ int yod_dbmodel_remove(yod_dbmodel_t *object, char *where, uint where_len, zval *params, zval *retval TSRMLS_DC)
*/
static int yod_dbmodel_remove(yod_dbmodel_t *object, char *where, uint where_len, zval *params, zval *retval TSRMLS_DC) {
	yod_database_t *yoddb;
	zval *table, *where1, *params1, *query, *pzval, **ppval;

#if PHP_YOD_DEBUG
	yod_debugl(1 TSRMLS_CC);
	yod_debugf("yod_dbmodel_remove()");
#endif

	if (!object) {
		if (retval) {
			ZVAL_BOOL(retval, 0);
		}
		return 0;
	}

	yoddb = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_db"), 1 TSRMLS_CC);
	if (!yoddb || Z_TYPE_P(yoddb) != IS_OBJECT) {
		if (retval) {
			ZVAL_BOOL(retval, 0);
		}
		yod_dbmodel_initquery(object, NULL TSRMLS_CC);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Call to a member function insert() on a non-object");
		return 0;
	}

	table = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_table"), 1 TSRMLS_CC);
	if (!table || Z_TYPE_P(table) != IS_STRING || Z_STRLEN_P(table) == 0) {
		if (retval) {
			ZVAL_BOOL(retval, 0);
		}
		yod_dbmodel_initquery(object, NULL TSRMLS_CC);
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "You have an error in your SQL syntax: table name is empty");
		return 0;
	}

	yod_dbmodel_where(object, where, where_len, params, NULL, 0 TSRMLS_CC);
	query = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_query"), 1 TSRMLS_CC);
	if (Z_TYPE_P(query) == IS_ARRAY) {
		if (zend_hash_find(Z_ARRVAL_P(query), ZEND_STRS("WHERE"), (void **)&ppval) == FAILURE) {
			yod_dbmodel_initquery(object, NULL TSRMLS_CC);
			return 0;
		}
		MAKE_STD_ZVAL(where1);
		if (!ppval || Z_TYPE_PP(ppval) != IS_STRING) {
			ZVAL_STRINGL(where1, "", 0, 1);
		} else {
			ZVAL_STRINGL(where1, Z_STRVAL_PP(ppval), Z_STRLEN_PP(ppval), 1);
		}
		params1 = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_params"), 1 TSRMLS_CC);
		yod_call_method_with_3_params(&yoddb, Z_OBJCE_P(yoddb), NULL, "delete", &pzval, table, where1, params1);
		zval_ptr_dtor(&where1);
		if (retval) {
			if (pzval) {
				ZVAL_ZVAL(retval, pzval, 1, 1);
			} else {
				ZVAL_BOOL(retval, 0);
			}	
		}

		yod_dbmodel_initquery(object, NULL TSRMLS_CC);
		return 1;
	}

	yod_dbmodel_initquery(object, NULL TSRMLS_CC);
	return 0;
}
/* }}} */

/** {{{ proto public Yod_DbModel::__construct($name = '', $config = null)
*/
PHP_METHOD(yod_dbmodel, __construct) {
	zval *config = NULL;
	char *name = NULL;
	uint name_len = 0;
	

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sz!", &name, &name_len, &config) == FAILURE) {
		return;
	}

	yod_dbmodel_construct(getThis(), name, name_len, config TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_DbModel::getInstance($name = '', $config = null)
*/
PHP_METHOD(yod_dbmodel, getInstance) {
	zval *config = NULL;
	char *name = NULL;
	uint name_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sz!", &name, &name_len, &config) == FAILURE) {
		return;
	}

	yod_dbmodel_getinstance(name, name_len, config, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_DbModel::find($where = '', $params = array(), $select = null)
*/
PHP_METHOD(yod_dbmodel, find) {
	zval *where = NULL, *params = NULL, *select = NULL;
	char *where1 = NULL;
	uint where1_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zzz!", &where, &params, &select) == FAILURE) {
		return;
	}

	if (where) {
		if (Z_TYPE_P(where) == IS_STRING) {
			if (is_numeric_string(Z_STRVAL_P(where), Z_STRLEN_P(where), NULL, NULL, 0)) {
				where1_len = spprintf(&where1, 0, "id = %s", Z_LVAL_P(where));
			} else {
				where1_len = spprintf(&where1, 0, "%s", Z_STRVAL_P(where));
			}
		} else if (Z_TYPE_P(where) == IS_LONG) {
			where1_len = spprintf(&where1, 0, "id = %ld", Z_LVAL_P(where));
		}
	}
	yod_dbmodel_find(getThis(), where1, where1_len, params, select, return_value TSRMLS_CC);
	if (where1) {
		efree(where1);
	}
}
/* }}} */

/** {{{ proto public Yod_DbModel::select($where = '', $params = array(), $select = null)
*/
PHP_METHOD(yod_dbmodel, select) {
	zval *where = NULL, *params = NULL, *select = NULL;
	char *where1 = NULL;
	uint where1_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zzz!", &where, &params, &select) == FAILURE) {
		return;
	}

	if (where) {
		if (Z_TYPE_P(where) == IS_STRING) {
			if (is_numeric_string(Z_STRVAL_P(where), Z_STRLEN_P(where), NULL, NULL, 0)) {
				where1_len = spprintf(&where1, 0, "id = %s", Z_LVAL_P(where));
			} else {
				where1_len = spprintf(&where1, 0, "%s", Z_STRVAL_P(where));
			}
		} else if (Z_TYPE_P(where) == IS_LONG) {
			where1_len = spprintf(&where1, 0, "id = %ld", Z_LVAL_P(where));
		}
	}
	yod_dbmodel_select(getThis(), where1, where1_len, params, select, return_value TSRMLS_CC);
	if (where1) {
		efree(where1);
	}
}
/* }}} */

/** {{{ proto public Yod_DbModel::count($where = '', $params = array())
*/
PHP_METHOD(yod_dbmodel, count) {
	zval *params = NULL, *select = NULL;
	char *where = NULL;
	uint where_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sz!", &where, &where_len, &params) == FAILURE) {
		return;
	}

	yod_dbmodel_count(getThis(), where, where_len, params, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_DbModel::save($data)
*/
PHP_METHOD(yod_dbmodel, save) {
	zval *data = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &data) == FAILURE) {
		return;
	}

	yod_dbmodel_save(getThis(), data, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_DbModel::update($data, $where = '', $params = array())
*/
PHP_METHOD(yod_dbmodel, update) {
	zval *data = NULL, *params = NULL, *data1;
	char *where = NULL;
	uint where_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|sz!", &data, &where, &where_len, &params) == FAILURE) {
		return;
	}

	if (!data) {
		RETURN_FALSE;
	}

	MAKE_STD_ZVAL(data1);
	ZVAL_ZVAL(data1, data, 1, 0);
	yod_dbmodel_update(getThis(), data1, where, where_len, params, return_value TSRMLS_CC);
	zval_ptr_dtor(&data1);
}
/* }}} */

/** {{{ proto public Yod_DbModel::remove($where, $params = array())
*/
PHP_METHOD(yod_dbmodel, remove) {
	zval *params = NULL;
	char *where = NULL;
	uint where_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sz!", &where, &where_len, &params) == FAILURE) {
		return;
	}

	yod_dbmodel_remove(getThis(), where, where_len, params, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_DbModel::field($select)
*/
PHP_METHOD(yod_dbmodel, field) {
	yod_dbmodel_t *object;
	zval *select = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &select) == FAILURE) {
		return;
	}

	object = getThis();

	yod_dbmodel_field(object, select TSRMLS_CC);

	RETURN_ZVAL(object, 1, 0);
}
/* }}} */

/** {{{ proto public Yod_DbModel::from($table)
*/
PHP_METHOD(yod_dbmodel, from) {
	yod_dbmodel_t *object;
	char *table = NULL;
	uint table_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &table, &table_len) == FAILURE) {
		return;
	}

	object = getThis();

	yod_dbmodel_from(object, table, table_len TSRMLS_CC);

	RETURN_ZVAL(object, 1, 0);
}
/* }}} */

/** {{{ proto public Yod_DbModel::join($table, $where = '', $mode = 'LEFT')
*/
PHP_METHOD(yod_dbmodel, join) {
	yod_dbmodel_t *object;
	char *table = NULL, *where = NULL, *mode = NULL;
	uint table_len = 0, where_len = 0, mode_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|s", &table, &table_len, &where, &where_len, &mode, &mode_len) == FAILURE) {
		return;
	}

	object = getThis();

	yod_dbmodel_join(object, table, table_len, where, where_len, mode, mode_len TSRMLS_CC);

	RETURN_ZVAL(object, 1, 0);
}
/* }}} */

/** {{{ proto public Yod_DbModel::where($where, $params = array(), $mode = 'AND')
*/
PHP_METHOD(yod_dbmodel, where) {
	yod_dbmodel_t *object;
	zval *params = NULL;
	char *where = NULL, *mode = NULL;
	uint where_len = 0, mode_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|zs", &where, &where_len, &params, &mode, &mode_len) == FAILURE) {
		return;
	}

	object = getThis();

	yod_dbmodel_where(object, where, where_len, params, mode, mode_len TSRMLS_CC);

	RETURN_ZVAL(object, 1, 0);
}
/* }}} */

/** {{{ proto public Yod_DbModel::group($group)
*/
PHP_METHOD(yod_dbmodel, group) {
	yod_dbmodel_t *object;
	char *group = NULL;
	uint group_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &group, &group_len) == FAILURE) {
		return;
	}

	object = getThis();

	yod_dbmodel_group(object, group, group_len TSRMLS_CC);

	RETURN_ZVAL(object, 1, 0);
}
/* }}} */

/** {{{ proto public Yod_DbModel::having($having, $params = array())
*/
PHP_METHOD(yod_dbmodel, having) {
	yod_dbmodel_t *object;
	zval *params = NULL;
	char *having = NULL;
	uint having_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|z!", &having, &having_len, &params) == FAILURE) {
		return;
	}

	object = getThis();

	yod_dbmodel_having(object, having, having_len, params TSRMLS_CC);

	RETURN_ZVAL(object, 1, 0);
}
/* }}} */

/** {{{ proto public Yod_DbModel::order($order)
*/
PHP_METHOD(yod_dbmodel, order) {
	yod_dbmodel_t *object;
	char *order = NULL;
	uint order_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &order, &order_len) == FAILURE) {
		return;
	}

	object = getThis();

	yod_dbmodel_order(object, order, order_len TSRMLS_CC);

	RETURN_ZVAL(object, 1, 0);
}
/* }}} */

/** {{{ proto public Yod_DbModel::limit($limit)
*/
PHP_METHOD(yod_dbmodel, limit) {
	yod_dbmodel_t *object;
	char *limit = NULL;
	uint limit_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &limit, &limit_len) == FAILURE) {
		return;
	}

	object = getThis();

	yod_dbmodel_limit(object, limit, limit_len TSRMLS_CC);

	RETURN_ZVAL(object, 1, 0);
}
/* }}} */

/** {{{ proto public Yod_DbModel::union($union, $params = array(), $mode = '')
*/
PHP_METHOD(yod_dbmodel, union) {
	yod_dbmodel_t *object;
	zval *unions = NULL, *params = NULL;
	char *mode = NULL;
	uint mode_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|zs", &unions, &params, &mode, &mode_len) == FAILURE) {
		return;
	}

	object = getThis();

	yod_dbmodel_union(object, unions, params, mode, mode_len TSRMLS_CC);

	RETURN_ZVAL(object, 1, 0);
}
/* }}} */

/** {{{ proto public Yod_DbModel::comment($comment)
*/
PHP_METHOD(yod_dbmodel, comment) {
	yod_dbmodel_t *object;
	char *comment = NULL;
	uint comment_len = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &comment, &comment_len) == FAILURE) {
		return;
	}

	object = getThis();

	yod_dbmodel_comment(object, comment, comment_len TSRMLS_CC);

	RETURN_ZVAL(object, 1, 0);
}
/* }}} */

/** {{{ proto public Yod_DbModel::params($params)
*/
PHP_METHOD(yod_dbmodel, params) {
	yod_dbmodel_t *object;
	zval *params = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &params) == FAILURE) {
		return;
	}

	object = getThis();

	yod_dbmodel_params(object, params TSRMLS_CC);

	RETURN_ZVAL(object, 1, 0);
}
/* }}} */

/** {{{ proto public Yod_DbModel::parseQuery($query = null)
*/
PHP_METHOD(yod_dbmodel, parseQuery) {
	zval *query = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z!", &query) == FAILURE) {
		return;
	}

	yod_dbmodel_parsequery(getThis(), query, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_DbModel::initQuery()
*/
PHP_METHOD(yod_dbmodel, initQuery) {
	yod_dbmodel_t *object;

	object = getThis();

	yod_dbmodel_initquery(object, NULL TSRMLS_CC);

	RETURN_ZVAL(object, 1, 0);
}
/* }}} */

/** {{{ yod_dbmodel_methods[]
*/
zend_function_entry yod_dbmodel_methods[] = {
	PHP_ME(yod_dbmodel, __construct,	yod_dbmodel_construct_arginfo,		ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(yod_dbmodel, getInstance,	yod_dbmodel_getinstance_arginfo,	ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	PHP_ME(yod_dbmodel, find,			yod_dbmodel_find_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbmodel, select,			yod_dbmodel_select_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbmodel, count,			yod_dbmodel_count_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbmodel, save,			yod_dbmodel_save_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbmodel, update,			yod_dbmodel_update_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbmodel, remove,			yod_dbmodel_remove_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbmodel, field,			yod_dbmodel_field_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbmodel, from,			yod_dbmodel_from_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbmodel, join,			yod_dbmodel_join_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbmodel, where,			yod_dbmodel_where_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbmodel, group,			yod_dbmodel_group_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbmodel, having,			yod_dbmodel_having_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbmodel, order,			yod_dbmodel_order_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbmodel, limit,			yod_dbmodel_limit_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbmodel, union,			yod_dbmodel_union_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbmodel, comment,		yod_dbmodel_comment_arginfo,		ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbmodel, params,			yod_dbmodel_params_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbmodel, parseQuery,		yod_dbmodel_parsequery_arginfo,		ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbmodel, initQuery,		yod_dbmodel_initquery_arginfo,		ZEND_ACC_PROTECTED)
	PHP_MALIAS(yod_dbmodel, findAll,	select,		yod_dbmodel_select_arginfo,			ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ PHP_MINIT_FUNCTION
*/
PHP_MINIT_FUNCTION(yod_dbmodel) {
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "Yod_DbModel", yod_dbmodel_methods);
	yod_dbmodel_ce = zend_register_internal_class_ex(&ce, yod_model_ce, NULL TSRMLS_CC);

	zend_declare_property_null(yod_dbmodel_ce, ZEND_STRL("_model"), ZEND_ACC_PROTECTED|ZEND_ACC_STATIC TSRMLS_CC);
	zend_declare_property_null(yod_dbmodel_ce, ZEND_STRL("_query"), ZEND_ACC_PROTECTED TSRMLS_CC);
	zend_declare_property_null(yod_dbmodel_ce, ZEND_STRL("_params"), ZEND_ACC_PROTECTED TSRMLS_CC);

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
