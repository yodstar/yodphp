<?php
defined('TESTS_PATH') or define('TESTS_PATH', dirname(__FILE__));
$tests = array(
	TESTS_PATH . '/Config/tpl_data.config.php',
	TESTS_PATH . '/Config/db_dsn.config.php',
	TESTS_PATH . '/Config',
	TESTS_PATH . '/Home/Action/index/IndexAction.php',
	TESTS_PATH . '/Home/Action/index',
	TESTS_PATH . '/Home/Action',
	TESTS_PATH . '/Home/Controller/IndexController.php',
	TESTS_PATH . '/Home/Controller',
	TESTS_PATH . '/Home/Model/TestsModel.php',
	TESTS_PATH . '/Home/Model',
	TESTS_PATH . '/Home/View/tests/hello.php',
	TESTS_PATH . '/Home/View/tests/widget.php',
	TESTS_PATH . '/Home/View/tests',
	TESTS_PATH . '/Home/View',
	TESTS_PATH . '/Home/Widget/public/footer.php',
	TESTS_PATH . '/Home/Widget/public/header.php',
	TESTS_PATH . '/Home/Widget/public',
	TESTS_PATH . '/Home/Widget/PublicWidget.php',
	TESTS_PATH . '/Home/Widget',
	TESTS_PATH . '/Home',
	TESTS_PATH . '/Extend/Tests.php',
	TESTS_PATH . '/Extend',
	TESTS_PATH . '/server/index.php',
	TESTS_PATH . '/server/close.php',
	TESTS_PATH . '/server',
	TESTS_PATH . '/client/server.php',
	TESTS_PATH . '/client',
);
foreach ($tests as $file) {
	if (is_file($file)) {
		@unlink($file);
	} elseif (is_dir($file)) {
		@rmdir($file);
	}
}
