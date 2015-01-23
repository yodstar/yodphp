PHP_ARG_ENABLE(yod, whether to enable yod support,
[  --enable-yod            Enable yod support])

PHP_ARG_WITH(yod-crypt, for yod crypt support,
[  --with-yod-crypt        Include crypt support [[default=no]]], no, no)

PHP_ARG_ENABLE(yod-debug, whether to enable yod debug mode,
[  --enable-yod-debug      Enable yod debug mode [[default=no]]], no, no)

if test "$PHP_YOD" != "no"; then
	if test "$PHP_YOD_CRYPT" != "no"; then
		AC_DEFINE(PHP_YOD_CRYPT, 1, [define to 1 if you want to running in crypt mode])
	else
		AC_DEFINE(PHP_YOD_CRYPT, 0, [define to 1 if you want to running in crypt mode])
	fi

	if test "$PHP_YOD_DEBUG" = "yes"; then
		AC_DEFINE(PHP_YOD_DEBUG, 1, [define to 1 if you want to running in debug mode])
	else
		AC_DEFINE(PHP_YOD_DEBUG, 0, [define to 1 if you want to running in debug mode])
	fi

	AC_MSG_CHECKING([PHP version])

	tmp_version=$PHP_VERSION
	if test -z "$tmp_version"; then
		if test -z "$PHP_CONFIG"; then
			AC_MSG_ERROR([php-config not found])
		fi
		php_version=`$PHP_CONFIG --version 2>/dev/null|head -n 1|sed -e 's#\([0-9]\.[0-9]*\.[0-9]*\)\(.*\)#\1#'`
	else
		php_version=`echo "$tmp_version"|sed -e 's#\([0-9]\.[0-9]*\.[0-9]*\)\(.*\)#\1#'`
	fi

	if test -z "$php_version"; then
		AC_MSG_ERROR([failed to detect PHP version, please report])
	fi

	ac_IFS=$IFS
	IFS="."
	set $php_version
	IFS=$ac_IFS
	yod_php_version=`expr [$]1 \* 1000000 + [$]2 \* 1000 + [$]3`

	if test "$yod_php_version" -le "5002000"; then
		AC_MSG_ERROR([You need at least PHP 5.2.0 to be able to use this version of Yod. PHP $php_version found])
	else
		AC_MSG_RESULT([$php_version, ok])
	fi

	ext_files="yod.c yod_application.c yod_request.c yod_controller.c yod_action.c yod_widget.c yod_server.c yod_client.c yod_model.c yod_dbmodel.c yod_database.c yod_dbpdo.c yod_plugin.c yod_base.c"

	if test "$PHP_YOD_CRYPT" != "no"; then
		if test "$PHP_YOD_CRYPT" = "yes"; then
			ext_files=$ext_files" crypt/screw.c"
			PHP_YOD_CRYPT_TYPE = "screw"
		else
			ext_files=$ext_files" crypt/"$PHP_YOD_CRYPT".c"
			PHP_YOD_CRYPT_TYPE = "$PHP_YOD_CRYPT"
		fi
	fi

	if test "$PHP_YOD_DEBUG" = "yes"; then
		ext_files=$ext_files" yod_debug.c"
	fi

	PHP_NEW_EXTENSION(yod, $ext_files, $ext_shared)
fi
