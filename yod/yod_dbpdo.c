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

#include "php_yod.h"
#include "yod_database.h"
#include "yod_dbpdo.h"

#if PHP_YOD_DEBUG
#include "yod_debug.h"
#endif

zend_class_entry *yod_dbpdo_ce;

/** {{{ ARG_INFO
 */
ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_construct_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, config)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_db_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, config)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_getinstance_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, config)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_config_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_create_arginfo, 0, 0, 2)
	ZEND_ARG_INFO(0, fields)
	ZEND_ARG_INFO(0, table)
	ZEND_ARG_INFO(0, extend)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_insert_arginfo, 0, 0, 2)
	ZEND_ARG_INFO(0, data)
	ZEND_ARG_INFO(0, table)
	ZEND_ARG_INFO(0, replace)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_update_arginfo, 0, 0, 2)
	ZEND_ARG_INFO(0, data)
	ZEND_ARG_INFO(0, table)
	ZEND_ARG_INFO(0, where)
	ZEND_ARG_INFO(0, params)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_delete_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, table)
	ZEND_ARG_INFO(0, where)
	ZEND_ARG_INFO(0, params)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_select_arginfo, 0, 0, 2)
	ZEND_ARG_INFO(0, select)
	ZEND_ARG_INFO(0, table)
	ZEND_ARG_INFO(0, where)
	ZEND_ARG_INFO(0, params)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_lastquery_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_dbconfig_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, config)
	ZEND_ARG_INFO(0, linknum)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_connect_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, config)
	ZEND_ARG_INFO(0, linknum)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_fields_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, table)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_execute_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, query)
	ZEND_ARG_INFO(0, params)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_query_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, query)
	ZEND_ARG_INFO(0, params)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_count_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, result)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_fetch_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, result)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_fetchall_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, result)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_transaction_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_commit_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_rollback_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_insertid_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_quote_arginfo, 0, 0, 1)
	ZEND_ARG_INFO(0, string)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_free_arginfo, 0, 0, 0)
	ZEND_ARG_INFO(0, result)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_close_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_errno_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(yod_dbpdo_error_arginfo, 0, 0, 0)
ZEND_END_ARG_INFO()
/* }}} */

/** {{{ static int yod_dbpdo_connect(yod_dbpdo_t *object, zval *config, long linknum, zval *retval TSRMLS_DC)
*/
static int yod_dbpdo_connect(yod_dbpdo_t *object, zval *config, long linknum, zval *retval TSRMLS_DC) {
	zval *dbconfig, *linkids, *linkids1, *linkid, *linkid1, *argv[4], *query, **ppval;
	zval persist, emulate, zvtrue, errmode, warning;
	zend_class_entry **pce = NULL;
	char *squery;
	uint squery_len;
	

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbpdo_connect()");
#endif

	if (!object) {
		if (retval) {
			ZVAL_BOOL(retval, 0);
		}
		return 0;
	}

	MAKE_STD_ZVAL(dbconfig);
	yod_database_dbconfig(object, config, linknum, dbconfig TSRMLS_CC);
	if (Z_TYPE_P(dbconfig) == IS_ARRAY) {
		if (zend_hash_find(Z_ARRVAL_P(dbconfig), ZEND_STRS("linknum"), (void **)&ppval) == SUCCESS &&
			Z_TYPE_P(dbconfig) == IS_LONG
		) {
			linknum = Z_LVAL_PP(ppval);
		} else {
			linknum = 0;
		}
	}

	linkids = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_linkids"), 1 TSRMLS_CC);
	if (linkids && Z_TYPE_P(linkids) == IS_ARRAY) {
		if (zend_hash_index_find(Z_ARRVAL_P(linkids), linknum, (void **)&ppval) == SUCCESS) {
			MAKE_STD_ZVAL(linkid);
			ZVAL_ZVAL(linkid, *ppval, 1, 0)
			zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_linkid"), linkid TSRMLS_CC);
			zval_ptr_dtor(&linkid);
			zval_ptr_dtor(&dbconfig);
			if (retval) {
				ZVAL_ZVAL(retval, *ppval, 1, 0);
			}
			return 1;
		}
	}

	if (Z_TYPE_P(dbconfig) == IS_ARRAY) {
		/* db_dsn.pdsn */
		if (zend_hash_find(Z_ARRVAL_P(dbconfig), ZEND_STRS("pdsn"), (void **)&ppval) == FAILURE ||
			Z_TYPE_PP(ppval) != IS_STRING || Z_STRLEN_PP(ppval) == 0
		) {
			/* db_dsn.dsn */
			if (zend_hash_find(Z_ARRVAL_P(dbconfig), ZEND_STRS("dsn"), (void **)&ppval) == FAILURE ||
				Z_TYPE_PP(ppval) != IS_STRING || Z_STRLEN_PP(ppval) == 0
			) {
				php_error_docref(NULL TSRMLS_CC, E_ERROR, "PDO DSN configure error");
				zval_ptr_dtor(&dbconfig);
				if (retval) {
					ZVAL_BOOL(retval, 0);
				}
				return 0;
			}
		}

#if PHP_API_VERSION < 20100412
		if (zend_lookup_class_ex(ZEND_STRL("PDO"), 0, &pce TSRMLS_CC) == SUCCESS) {
#else
		if (zend_lookup_class_ex(ZEND_STRL("PDO"), NULL, 0, &pce TSRMLS_CC) == SUCCESS) {
#endif
			MAKE_STD_ZVAL(linkid);
			object_init_ex(linkid, *pce);

			/* argv */
			MAKE_STD_ZVAL(argv[0]);
			MAKE_STD_ZVAL(argv[1]);
			MAKE_STD_ZVAL(argv[2]);
			MAKE_STD_ZVAL(argv[3]);

			/* argv.pdsn */
			ZVAL_STRINGL(argv[0] , Z_STRVAL_PP(ppval), Z_STRLEN_PP(ppval), 1);

			/* argv.user */
			if (zend_hash_find(Z_ARRVAL_P(dbconfig), ZEND_STRS("user"), (void **)&ppval) == SUCCESS) {
				convert_to_string(*ppval);
				ZVAL_STRINGL(argv[1], Z_STRVAL_PP(ppval), Z_STRLEN_PP(ppval), 1);
			} else {
				ZVAL_NULL(argv[1]);
			}

			/* argv.pass */
			if (zend_hash_find(Z_ARRVAL_P(dbconfig), ZEND_STRS("pass"), (void **)&ppval) == SUCCESS) {
				convert_to_string(*ppval);
				ZVAL_STRINGL(argv[2], Z_STRVAL_PP(ppval), Z_STRLEN_PP(ppval), 1);
			} else {
				ZVAL_NULL(argv[2]);
			}

			/* argv.options */
			if (zend_hash_find(Z_ARRVAL_P(dbconfig), ZEND_STRS("options"), (void **)&ppval) == SUCCESS &&
				Z_TYPE_PP(ppval) == IS_ARRAY
			) {
				ZVAL_ZVAL(argv[3], *ppval, 1, 1);
			} else {
				array_init(argv[3]);
			}

			/* pconnect */
			if (zend_hash_find(Z_ARRVAL_P(dbconfig), ZEND_STRS("pconnect"), (void **)&ppval) == SUCCESS &&
				(Z_TYPE_PP(ppval) == IS_BOOL && Z_BVAL_PP(ppval)) || (Z_TYPE_PP(ppval) == IS_LONG && Z_LVAL_PP(ppval))
			) {
#ifndef ZEND_FETCH_CLASS_SILENT
				if (zend_get_constant_ex(ZEND_STRL("PDO::ATTR_PERSISTENT"), &persist, NULL TSRMLS_CC)) {
#else
				if (zend_get_constant_ex(ZEND_STRL("PDO::ATTR_PERSISTENT"), &persist, NULL, ZEND_FETCH_CLASS_SILENT TSRMLS_CC)) {
#endif
					add_index_bool(argv[3], Z_LVAL(persist), 1);
				} else {
					add_index_bool(argv[3], 12, 1);
				}
			}

			yod_call_method_with_4_params(&linkid, *pce, &(*pce)->constructor, ZEND_CONSTRUCTOR_FUNC_NAME, NULL, argv[0], argv[1], argv[2], argv[3]);

			zval_ptr_dtor(&argv[0]);
			zval_ptr_dtor(&argv[1]);
			zval_ptr_dtor(&argv[2]);
			zval_ptr_dtor(&argv[3]);

			/* emulate */
#ifndef ZEND_FETCH_CLASS_SILENT
			if (!zend_get_constant_ex(ZEND_STRL("PDO::ATTR_EMULATE_PREPARES"), &emulate, NULL TSRMLS_CC)) {
#else
			if (!zend_get_constant_ex(ZEND_STRL("PDO::ATTR_EMULATE_PREPARES"), &emulate, NULL, ZEND_FETCH_CLASS_SILENT TSRMLS_CC)) {
#endif
				INIT_PZVAL(&emulate);
				ZVAL_LONG(&emulate, 20);
			}
			INIT_PZVAL(&zvtrue);
			ZVAL_BOOL(&zvtrue, 1);

			zend_call_method_with_2_params(&linkid, Z_OBJCE_P(linkid), NULL, "setattribute", NULL, &emulate, &zvtrue);

			/* charset */
			if (zend_hash_find(Z_ARRVAL_P(dbconfig), ZEND_STRS("charset"), (void **)&ppval) == SUCCESS) {
				convert_to_string(*ppval);
				squery_len = spprintf(&squery, 0, "SET NAMES %s", Z_STRVAL_PP(ppval));
			} else {
				squery_len = spprintf(&squery, 0, "SET NAMES utf8");
			}

			MAKE_STD_ZVAL(query);
			ZVAL_STRINGL(query, squery, squery_len, 1);
			zend_call_method_with_1_params(&linkid, Z_OBJCE_P(linkid), NULL, "exec", NULL, query);	
			zval_ptr_dtor(&query);
			efree(squery);

			/* errmode */
#ifndef ZEND_FETCH_CLASS_SILENT
			if (!zend_get_constant_ex(ZEND_STRL("PDO::ATTR_ERRMODE"), &errmode, NULL TSRMLS_CC)) {
#else
			if (!zend_get_constant_ex(ZEND_STRL("PDO::ATTR_ERRMODE"), &errmode, NULL, ZEND_FETCH_CLASS_SILENT TSRMLS_CC)) {
#endif
				INIT_PZVAL(&errmode);
				ZVAL_LONG(&errmode, 3);
			}
#ifndef ZEND_FETCH_CLASS_SILENT
			if (!zend_get_constant_ex(ZEND_STRL("PDO::ERRMODE_WARNING"), &warning, NULL TSRMLS_CC)) {
#else
			if (!zend_get_constant_ex(ZEND_STRL("PDO::ERRMODE_WARNING"), &warning, NULL, ZEND_FETCH_CLASS_SILENT TSRMLS_CC)) {
#endif
				INIT_PZVAL(&warning);
				ZVAL_LONG(&warning, 1);
			}
			zend_call_method_with_2_params(&linkid, Z_OBJCE_P(linkid), NULL, "setattribute", NULL, &errmode, &warning);

			/* linkid */
			zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_linkid"), linkid TSRMLS_CC);
			MAKE_STD_ZVAL(linkids1);
			if (linkids && Z_TYPE_P(linkids) == IS_ARRAY) {
				ZVAL_ZVAL(linkids1, linkids, 1, 0);
			} else {
				array_init(linkids1);
			}
			MAKE_STD_ZVAL(linkid1);
			ZVAL_ZVAL(linkid1, linkid, 1, 0);
			add_index_zval(linkids1, linknum, linkid1);
			zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_linkids"), linkids1 TSRMLS_CC);
			zval_ptr_dtor(&linkids1);

			if (retval) {
				ZVAL_ZVAL(retval, linkid, 1, 0);
			}
			zval_ptr_dtor(&linkid);
		} else {
			php_error_docref(NULL TSRMLS_CC, E_ERROR, "Class 'PDO' not found");
			zval_ptr_dtor(&dbconfig);
			if (retval) {
				ZVAL_BOOL(retval, 0);
			}
			return 0;
		}
	}
	zval_ptr_dtor(&dbconfig);
	
	return 1;
}
/* }}} */

/** {{{ int yod_dbpdo_execute(yod_dbpdo_t *object, zval *query, zval *params, int affected, zval *retval TSRMLS_DC)
*/
int yod_dbpdo_execute(yod_dbpdo_t *object, zval *query, zval *params, int affected, zval *retval TSRMLS_DC) {
	zval *config, *linkid, *result, *pzval;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbpdo_execute()");
#endif

	if (!query || Z_TYPE_P(query) != IS_STRING || !object) {
		if (retval) {
			ZVAL_BOOL(retval, 0);
		}
		return 0;
	}

	config = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_config"), 1 TSRMLS_CC);
	yod_dbpdo_connect(object, config, 0, NULL TSRMLS_CC);

	zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_lastquery"), query TSRMLS_CC);

	linkid = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_linkid"), 1 TSRMLS_CC);
	if (linkid && Z_TYPE_P(linkid) == IS_OBJECT) {
		if (!params || Z_TYPE_P(params) != IS_ARRAY) {
			zend_call_method_with_1_params(&linkid, Z_OBJCE_P(linkid), NULL, "exec", &pzval, query);
			if (pzval) {
				if (params && Z_TYPE_P(params) == IS_BOOL) {
					affected = Z_BVAL_P(params);
				}
				if (retval) {
					if (affected) {
						ZVAL_ZVAL(retval, pzval, 1, 0);
					} else {
						ZVAL_BOOL(retval, 1);
					}
				}
				zval_ptr_dtor(&pzval);
				return 1;
			}
		} else {
			zend_call_method_with_1_params(&linkid, Z_OBJCE_P(linkid), NULL, "prepare", &result, query);
			if (result && Z_TYPE_P(result) == IS_OBJECT) {
				zend_call_method_with_1_params(&result, Z_OBJCE_P(result), NULL, "execute", &pzval, params);
				if (pzval) {
					if (affected) {
						zval_ptr_dtor(&pzval);
						zend_call_method_with_0_params(&result, Z_OBJCE_P(result), NULL, "rowcount", &pzval);
					}
					zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_result"), result TSRMLS_CC);
				}
				zval_ptr_dtor(&result);

				if (pzval) {
					if (retval) {
						ZVAL_ZVAL(retval, pzval, 1, 0);
					}
					zval_ptr_dtor(&pzval);
					return 1;
				}
			} else {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Call to a member function execute() on a non-object");
			}
		}
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Call to a member function execute() on a non-object");
	}

	ZVAL_BOOL(retval, 0);
	return 0;
}
/* }}} */

/** {{{ int yod_dbpdo_query(yod_dbpdo_t *object, zval *query, zval *params, zval *retval TSRMLS_DC)
*/
int yod_dbpdo_query(yod_dbpdo_t *object, zval *query, zval *params, zval *retval TSRMLS_DC) {
	zval *config, *linkid, *result, *pzval;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbpdo_query()");
#endif

	if (!query || Z_TYPE_P(query) != IS_STRING || !object) {
		ZVAL_BOOL(retval, 0);
		return 0;
	}

	config = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_config"), 1 TSRMLS_CC);
	yod_dbpdo_connect(object, config, 0, NULL TSRMLS_CC);

	zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_lastquery"), query TSRMLS_CC);

	linkid = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_linkid"), 1 TSRMLS_CC);
	if (linkid && Z_TYPE_P(linkid) == IS_OBJECT) {
		if (!params || Z_TYPE_P(params) != IS_ARRAY) {
			zend_call_method_with_1_params(&linkid, Z_OBJCE_P(linkid), NULL, "query", &result, query);
			zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_result"), result TSRMLS_CC);
			if (result) {
				ZVAL_ZVAL(retval, result, 1, 1);
				return 1;
			}
		} else {
			zend_call_method_with_1_params(&linkid, Z_OBJCE_P(linkid), NULL, "prepare", &result, query);
			if (result && Z_TYPE_P(result) == IS_OBJECT) {
				zend_call_method_with_1_params(&result, Z_OBJCE_P(result), NULL, "execute", &pzval, params);
				zend_update_property(Z_OBJCE_P(object), object, ZEND_STRL("_result"), result TSRMLS_CC);
				if (pzval) {
					ZVAL_ZVAL(retval, result, 1, 1);
					zval_ptr_dtor(&pzval);
					return 1;
				}
			} else {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "Call to a member function query() on a non-object");
			}
		}
		if (result) {
			zval_ptr_dtor(&result);
		}
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Call to a member function query() on a non-object");
	}

	ZVAL_BOOL(retval, 0);
	return 0;
}
/* }}} */

/** {{{ int yod_dbpdo_count(yod_dbpdo_t *object, zval *result, zval *retval TSRMLS_DC)
*/
int yod_dbpdo_count(yod_dbpdo_t *object, zval *result, zval *retval TSRMLS_DC) {
	zval *pzval;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbpdo_count()");
#endif

	if (!object) {
		ZVAL_BOOL(retval, 0);
		return 0;
	}

	if (!result) {
		result = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_result"), 1 TSRMLS_CC);
	}

	if (result && Z_TYPE_P(result) == IS_OBJECT) {
		zend_call_method_with_0_params(&result, Z_OBJCE_P(result), NULL, "rowcount", &pzval);
		if (pzval) {
			ZVAL_ZVAL(retval, pzval, 1, 1);
			return 1;
		}
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Call to a member function count() on a non-object");
	}

	ZVAL_BOOL(retval, 0);
	return 0;
}
/* }}} */

/** {{{ int yod_dbpdo_fetch(yod_dbpdo_t *object, zval *result, zval *retval TSRMLS_DC)
*/
int yod_dbpdo_fetch(yod_dbpdo_t *object, zval *result, zval *retval TSRMLS_DC) {
	zval assoc, *pzval;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbpdo_fetch()");
#endif

	if (!object) {
		ZVAL_BOOL(retval, 0);
		return 0;
	}

	if (!result) {
		result = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_result"), 1 TSRMLS_CC);
	}

#ifndef ZEND_FETCH_CLASS_SILENT
	if (!zend_get_constant_ex(ZEND_STRL("PDO::FETCH_ASSOC"), &assoc, NULL TSRMLS_CC)) {
#else
	if (!zend_get_constant_ex(ZEND_STRL("PDO::FETCH_ASSOC"), &assoc, NULL, ZEND_FETCH_CLASS_SILENT TSRMLS_CC)) {
#endif
		INIT_PZVAL(&assoc);
		ZVAL_LONG(&assoc, 2);
	}

	if (result && Z_TYPE_P(result) == IS_OBJECT) {
		zend_call_method_with_1_params(&result, Z_OBJCE_P(result), NULL, "fetch", &pzval, &assoc);
		if (pzval) {
			ZVAL_ZVAL(retval, pzval, 1, 1);
			return 1;
		}
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Call to a member function fetch() on a non-object");
	}

	ZVAL_BOOL(retval, 0);
	return 0;
}
/* }}} */

/** {{{ int yod_dbpdo_fetchall(yod_dbpdo_t *object, zval *result, zval *retval TSRMLS_DC)
*/
int yod_dbpdo_fetchall(yod_dbpdo_t *object, zval *result, zval *retval TSRMLS_DC) {
	zval assoc, *pzval;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbpdo_fetchall()");
#endif

	if (!object) {
		ZVAL_BOOL(retval, 0);
		return 0;
	}

	if (!result) {
		result = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_result"), 1 TSRMLS_CC);
	}

#ifndef ZEND_FETCH_CLASS_SILENT
	if (!zend_get_constant_ex(ZEND_STRL("PDO::FETCH_ASSOC"), &assoc, NULL TSRMLS_CC)) {
#else
	if (!zend_get_constant_ex(ZEND_STRL("PDO::FETCH_ASSOC"), &assoc, NULL, ZEND_FETCH_CLASS_SILENT TSRMLS_CC)) {
#endif
		INIT_PZVAL(&assoc);
		ZVAL_LONG(&assoc, 2);
	}

	if (result && Z_TYPE_P(result) == IS_OBJECT) {
		zend_call_method_with_1_params(&result, Z_OBJCE_P(result), NULL, "fetchall", &pzval, &assoc);
		if (pzval) {
			ZVAL_ZVAL(retval, pzval, 1, 1);
			return 1;
		}
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Call to a member function fetchAll() on a non-object");
	}

	ZVAL_BOOL(retval, 0);
	return 0;
}
/* }}} */

/** {{{ int yod_dbpdo_free(yod_dbpdo_t *object, zval *result TSRMLS_DC)
*/
int yod_dbpdo_free(yod_dbpdo_t *object, zval *result TSRMLS_DC) {

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbpdo_free()");
#endif

	if (!object) {
		return 0;
	}

	if (result) {
		zend_object_store_ctor_failed(result TSRMLS_CC);
		zval_ptr_dtor(&result);
		ZVAL_NULL(result);
	} else {
		zend_update_property_null(Z_OBJCE_P(object), object, ZEND_STRL("_result") TSRMLS_CC);
	}

	return 1;
}
/* }}} */

/** {{{ proto public Yod_DbPdo::__construct($config)
*/
PHP_METHOD(yod_dbpdo, __construct) {
	yod_dbpdo_t *object;
	zval *config = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &config) == FAILURE) {
		return;
	}

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbpdo_construct()");
#endif

	object = getThis();

	yod_database_construct(object, config TSRMLS_CC);

	zend_update_property_string(Z_OBJCE_P(object), object, ZEND_STRL("_driver"), Z_OBJCE_P(object)->name TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_DbPdo::connect($config = null, $linknum = 0)
*/
PHP_METHOD(yod_dbpdo, connect) {
	zval *config = NULL;
	long linknum = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zl", &config, &linknum) == FAILURE) {
		return;
	}

	yod_dbpdo_connect(getThis(), config, linknum, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_DbPdo::fields($table)
*/
PHP_METHOD(yod_dbpdo, fields) {
	yod_dbpdo_t *object;
	zval *query, *prefix, *table = NULL;
	zval *result, *data, **data1, **ppval;
	char *squery, *squery1;
	uint squery_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &table) == FAILURE) {
		return;
	}

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbpdo_fields()");
#endif

	object = getThis();

	array_init(return_value);

	squery_len = spprintf(&squery, 0, "SHOW COLUMNS FROM ");
	prefix = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_prefix"), 1 TSRMLS_CC);
	if (prefix && Z_TYPE_P(prefix) == IS_STRING) {
		squery_len = spprintf(&squery1, 0, "%s%s", squery, Z_STRVAL_P(prefix));
		efree(squery);
		squery = squery1;
	}
	
	if (table && Z_TYPE_P(table) == IS_STRING) {
		squery_len = spprintf(&squery1, 0, "%s%s", squery, Z_STRVAL_P(table));
		efree(squery);
		squery = squery1;
	}

	MAKE_STD_ZVAL(query);
	ZVAL_STRINGL(query, squery, squery_len, 1);
	efree(squery);

	MAKE_STD_ZVAL(result);
	yod_dbpdo_query(object, query, NULL, result TSRMLS_CC);
	zval_ptr_dtor(&query);

	MAKE_STD_ZVAL(data);
	if (result) {
		yod_dbpdo_fetchall(object, result, data TSRMLS_CC);
		zval_ptr_dtor(&result);
	}
	if (!data || Z_TYPE_P(data) != IS_ARRAY) {
		zval_ptr_dtor(&data);
		RETURN_NULL();
	}

	zend_hash_internal_pointer_reset(Z_ARRVAL_P(data));
	while (zend_hash_get_current_data(Z_ARRVAL_P(data), (void **)&data1) == SUCCESS) {
		zval *field = NULL;

		if (Z_TYPE_PP(data1) != IS_ARRAY) {
			zend_hash_move_forward(Z_ARRVAL_P(data));
			continue;
		}

		MAKE_STD_ZVAL(field);
		array_init(field);
		/* name */
		add_assoc_null_ex(field, ZEND_STRS("name"));
		/* type */
		if (zend_hash_find(Z_ARRVAL_PP(data1), ZEND_STRS("Type"), (void**)&ppval) == SUCCESS) {
			zval_add_ref(ppval);
			add_assoc_zval_ex(field, ZEND_STRS("type"), *ppval);
		}
		/* notnull */
		if (zend_hash_find(Z_ARRVAL_PP(data1), ZEND_STRS("Null"), (void**)&ppval) == SUCCESS) {
			if (Z_TYPE_PP(ppval) == IS_STRING && Z_STRLEN_PP(ppval) == 0) {
				add_assoc_bool_ex(field, ZEND_STRS("notnull"), 1);
			} else {
				add_assoc_bool_ex(field, ZEND_STRS("notnull"), 0);
			}
		}
		/* default */
		if (zend_hash_find(Z_ARRVAL_PP(data1), ZEND_STRS("Default"), (void**)&ppval) == SUCCESS) {
			zval_add_ref(ppval);
			add_assoc_zval_ex(field, ZEND_STRS("default"), *ppval);
		}
		/* primary */
		if (zend_hash_find(Z_ARRVAL_PP(data1), ZEND_STRS("Key"), (void**)&ppval) == SUCCESS) {
			if (Z_TYPE_PP(ppval) == IS_STRING && strcasecmp(Z_STRVAL_PP(ppval), "pri") == 0) {
				add_assoc_bool_ex(field, ZEND_STRS("primary"), 1);
			} else {
				add_assoc_bool_ex(field, ZEND_STRS("primary"), 0);
			}
		}
		/* autoinc */
		if (zend_hash_find(Z_ARRVAL_PP(data1), ZEND_STRS("Extra"), (void**)&ppval) == SUCCESS) {
			if (Z_TYPE_PP(ppval) == IS_STRING && strcasecmp(Z_STRVAL_PP(ppval), "auto_increment") == 0) {
				add_assoc_bool_ex(field, ZEND_STRS("autoinc"), 1);
			} else {
				add_assoc_bool_ex(field, ZEND_STRS("autoinc"), 0);
			}
		}
		if (zend_hash_find(Z_ARRVAL_PP(data1), ZEND_STRS("Field"), (void**)&ppval) == SUCCESS) {
			zval_add_ref(ppval);
			add_assoc_zval_ex(field, ZEND_STRS("name"), *ppval);
			add_assoc_zval_ex(return_value, Z_STRVAL_PP(ppval), Z_STRLEN_PP(ppval) + 1, field);
		}

		zend_hash_move_forward(Z_ARRVAL_P(data));
	}
	zval_ptr_dtor(&data);
}
/* }}} */

/** {{{ proto public Yod_DbPdo::execute($query, $params = array(), $affected = false)
*/
PHP_METHOD(yod_dbpdo, execute) {
	zval *query = NULL, *params = NULL;
	int affected = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|zb", &query, &params, &affected) == FAILURE) {
		return;
	}

	yod_dbpdo_execute(getThis(), query, params, affected, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_DbPdo::query($query, $params = array())
*/
PHP_METHOD(yod_dbpdo, query) {
	zval *query = NULL, *params = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|z!", &query, &params) == FAILURE) {
		return;
	}

	yod_dbpdo_query(getThis(), query, params, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_DbPdo::count($result = null)
*/
PHP_METHOD(yod_dbpdo, count) {
	zval *result = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z!", &result) == FAILURE) {
		return;
	}

	yod_dbpdo_count(getThis(), result, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_DbPdo::fetch($result = null)
*/
PHP_METHOD(yod_dbpdo, fetch) {
	zval *result = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z!", &result) == FAILURE) {
		return;
	}

	yod_dbpdo_fetch(getThis(), result, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_DbPdo::fetchAll($result = null)
*/
PHP_METHOD(yod_dbpdo, fetchAll) {
	zval *result = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z!", &result) == FAILURE) {
		return;
	}

	yod_dbpdo_fetchall(getThis(), result, return_value TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_DbPdo::transaction()
*/
PHP_METHOD(yod_dbpdo, transaction) {
	yod_dbpdo_t *object;
	zval *config, *linkid, *retval;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbpdo_transaction()");
#endif

	object = getThis();

	zend_update_property_bool(Z_OBJCE_P(object), object, ZEND_STRL("_locked"), 1 TSRMLS_CC);
	config = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_config"), 1 TSRMLS_CC);
	yod_dbpdo_connect(object, config, 0, NULL TSRMLS_CC);
	linkid = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_linkid"), 1 TSRMLS_CC);
	if (linkid && Z_TYPE_P(linkid) == IS_OBJECT) {
		zend_call_method_with_0_params(&linkid, Z_OBJCE_P(linkid), NULL, "begintransaction", &retval);
		if (retval) {
			RETURN_ZVAL(retval, 1, 1);
		}
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Call to a member function begintransaction() on a non-object");
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yod_DbPdo::commit()
*/
PHP_METHOD(yod_dbpdo, commit) {
	yod_dbpdo_t *object;
	zval *config, *linkid, *retval;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbpdo_commit()");
#endif

	object = getThis();

	zend_update_property_bool(Z_OBJCE_P(object), object, ZEND_STRL("_locked"), 0 TSRMLS_CC);
	config = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_config"), 1 TSRMLS_CC);
	yod_dbpdo_connect(object, config, 0, NULL TSRMLS_CC);
	linkid = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_linkid"), 1 TSRMLS_CC);
	if (linkid && Z_TYPE_P(linkid) == IS_OBJECT) {
		zend_call_method_with_0_params(&linkid, Z_OBJCE_P(linkid), NULL, "commit", &retval);
		if (retval) {
			RETURN_ZVAL(retval, 1, 1);
		}
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Call to a member function commit() on a non-object");
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yod_DbPdo::rollback()
*/
PHP_METHOD(yod_dbpdo, rollback) {
	yod_dbpdo_t *object;
	zval *config, *linkid, *retval;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbpdo_rollback()");
#endif

	object = getThis();

	zend_update_property_bool(Z_OBJCE_P(object), object, ZEND_STRL("_locked"), 0 TSRMLS_CC);
	config = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_config"), 1 TSRMLS_CC);
	yod_dbpdo_connect(object, config, 0, NULL TSRMLS_CC);
	linkid = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_linkid"), 1 TSRMLS_CC);
	if (linkid && Z_TYPE_P(linkid) == IS_OBJECT) {
		zend_call_method_with_0_params(&linkid, Z_OBJCE_P(linkid), NULL, "rollback", &retval);
		if (retval) {
			RETURN_ZVAL(retval, 1, 1);
		}
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Call to a member function rollBack() on a non-object");
	}

	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yod_DbPdo::insertId()
*/
PHP_METHOD(yod_dbpdo, insertId) {
	yod_dbpdo_t *object;
	zval *linkid, *retval;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbpdo_insertid()");
#endif

	object = getThis();

	linkid = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_linkid"), 1 TSRMLS_CC);
	if (linkid && Z_TYPE_P(linkid) == IS_OBJECT) {
		zend_call_method_with_0_params(&linkid, Z_OBJCE_P(linkid), NULL, "lastinsertid", &retval);
		if (retval) {
			RETURN_ZVAL(retval, 1, 1);
		}
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Call to a member function lastInsertId() on a non-object");
	}

	RETURN_NULL();
}
/* }}} */

/** {{{ proto public Yod_DbPdo::quote($string)
*/
PHP_METHOD(yod_dbpdo, quote) {
	yod_dbpdo_t *object;
	zval *linkid, *string, *retval;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &string) == FAILURE) {
		return;
	}

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbpdo_quote()");
#endif

	object = getThis();

	linkid = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_linkid"), 1 TSRMLS_CC);
	if (linkid && Z_TYPE_P(linkid) == IS_OBJECT) {
		zend_call_method_with_1_params(&linkid, Z_OBJCE_P(linkid), NULL, "quote", &retval, string);
		if (retval) {
			RETURN_ZVAL(retval, 1, 1);
		}
	} else {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Call to a member function quote() on a non-object");
	}

	RETURN_NULL();
}
/* }}} */

/** {{{ proto public Yod_DbPdo::free($result = null)
*/
PHP_METHOD(yod_dbpdo, free) {
	zval *result = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|z!", &result) == FAILURE) {
		return;
	}

	yod_dbpdo_free(getThis(), result TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_DbPdo::close()
*/
PHP_METHOD(yod_dbpdo, close) {
	yod_dbpdo_t *object;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbpdo_close()");
#endif

	object = getThis();
	
	zend_update_property_null(Z_OBJCE_P(object), object, ZEND_STRL("_linkid") TSRMLS_CC);
	zend_update_property_null(Z_OBJCE_P(object), object, ZEND_STRL("_linkids") TSRMLS_CC);
}
/* }}} */

/** {{{ proto public Yod_DbPdo::errno()
*/
PHP_METHOD(yod_dbpdo, errNo) {
	yod_dbpdo_t *object;
	zval *result, *linkid, *errcode;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbpdo_errno()");
#endif

	object = getThis();

	result = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_result"), 1 TSRMLS_CC);
	if (result && Z_TYPE_P(result) == IS_OBJECT) {
		zend_call_method_with_0_params(&result, Z_OBJCE_P(result), NULL, "errorcode", &errcode);
		if (errcode) {
			RETURN_ZVAL(errcode, 1, 1);
		}
	}

	linkid = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_linkid"), 1 TSRMLS_CC);
	if (!linkid || Z_TYPE_P(linkid) != IS_OBJECT) {
		RETURN_FALSE;
	}
	zend_call_method_with_0_params(&linkid, Z_OBJCE_P(linkid), NULL, "errorcode", &errcode);
	if (errcode) {
		RETURN_ZVAL(errcode, 1, 1);
	}
	
	RETURN_LONG(0);
}
/* }}} */

/** {{{ proto public Yod_DbPdo::error()
*/
PHP_METHOD(yod_dbpdo, error) {
	yod_dbpdo_t *object;
	zval *result, *linkid, *errinfo, **ppval;

#if PHP_YOD_DEBUG
	yod_debugf("yod_dbpdo_error()");
#endif

	object = getThis();
	
	result = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_result"), 1 TSRMLS_CC);
	if (result && Z_TYPE_P(result) == IS_OBJECT) {
		zend_call_method_with_0_params(&result, Z_OBJCE_P(result), NULL, "errorinfo", &errinfo);
		if (errinfo) {
			if (Z_TYPE_P(errinfo) == IS_ARRAY &&
				zend_hash_index_find(Z_ARRVAL_P(errinfo), 2, (void**)&ppval) == SUCCESS &&
				Z_TYPE_PP(ppval) == IS_STRING
			) {
				ZVAL_STRINGL(return_value, Z_STRVAL_PP(ppval), Z_STRLEN_PP(ppval), 1);
				zval_ptr_dtor(&errinfo);
				return;
			}
			zval_ptr_dtor(&errinfo);
		}
	}

	linkid = zend_read_property(Z_OBJCE_P(object), object, ZEND_STRL("_linkid"), 1 TSRMLS_CC);
	if (!linkid || Z_TYPE_P(linkid) != IS_OBJECT) {
		RETURN_FALSE;
	}
	zend_call_method_with_0_params(&linkid, Z_OBJCE_P(linkid), NULL, "errorinfo", &errinfo);
	if (errinfo) {
		if (Z_TYPE_P(errinfo) == IS_ARRAY &&
			zend_hash_index_find(Z_ARRVAL_P(errinfo), 2, (void**)&ppval) == SUCCESS &&
			Z_TYPE_PP(ppval) == IS_STRING
		) {
			ZVAL_STRINGL(return_value, Z_STRVAL_PP(ppval), Z_STRLEN_PP(ppval), 1);
			zval_ptr_dtor(&errinfo);
			return;
		}
	}

	RETURN_NULL();
}
/* }}} */

/** {{{ yod_dbpdo_methods[]
*/
zend_function_entry yod_dbpdo_methods[] = {
	PHP_ME(yod_dbpdo, __construct,	yod_dbpdo_construct_arginfo,		ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(yod_dbpdo, connect,		yod_dbpdo_connect_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbpdo, fields,		yod_dbpdo_fields_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbpdo, execute,		yod_dbpdo_execute_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbpdo, query,		yod_dbpdo_query_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbpdo, count,		yod_dbpdo_count_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbpdo, fetch,		yod_dbpdo_fetch_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbpdo, fetchAll,		yod_dbpdo_fetchall_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbpdo, transaction,	yod_dbpdo_transaction_arginfo,		ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbpdo, commit,		yod_dbpdo_commit_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbpdo, rollback,		yod_dbpdo_rollback_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbpdo, insertId,		yod_dbpdo_insertid_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbpdo, quote,		yod_dbpdo_quote_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbpdo, free,			yod_dbpdo_free_arginfo,				ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbpdo, close,		yod_dbpdo_close_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbpdo, errNo,		yod_dbpdo_errno_arginfo,			ZEND_ACC_PUBLIC)
	PHP_ME(yod_dbpdo, error,		yod_dbpdo_error_arginfo,			ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ PHP_MINIT_FUNCTION
*/
PHP_MINIT_FUNCTION(yod_dbpdo) {
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, "Yod_DbPdo", yod_dbpdo_methods);
	yod_dbpdo_ce = zend_register_internal_class_ex(&ce, yod_database_ce, NULL TSRMLS_CC);

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
