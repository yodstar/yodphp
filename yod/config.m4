PHP_ARG_ENABLE(yod, whether to enable yod support,
[  --enable-yod           Enable yod support])

AC_ARG_ENABLE(yod-debug,
[  --enable-yod-debug     Enable yod debug mode default=no],
[PHP_YOD_DEBUG=$enableval],
[PHP_YOD_DEBUG="no"])  

if test "$PHP_YOD" != "no"; then
	if test "$PHP_YOD_DEBUG" = "yes"; then
		AC_DEFINE(PHP_YOD_DEBUG,1,[define to 1 if you want to change the POST/GET by php script])
	else
		AC_DEFINE(PHP_YOD_DEBUG,0,[define to 1 if you want to change the POST/GET by php script])
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

	ext_files="yod.c yod_application.c yod_request.c yod_controller.c yod_action.c yod_widget.c yod_model.c yod_dbmodel.c yod_database.c yod_dbpdo.c yod_plugin.c"

	if test "$PHP_YOD_DEBUG" = "yes"; then
		ext_files=$ext_files" yod_debug.c"
	fi

	PHP_NEW_EXTENSION(yod, $ext_files, $ext_shared)
fi
