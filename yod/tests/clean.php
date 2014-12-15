<?php
defined('TESTS_PATH') or define('TESTS_PATH', dirname(__FILE__));
$tests = array(
	TESTS_PATH . '/actions/index/IndexAction.php',
	TESTS_PATH . '/actions/index',
	TESTS_PATH . '/actions',
	TESTS_PATH . '/configs/tpl_data.config.php',
	TESTS_PATH . '/configs/db_dsn.config.php',
	TESTS_PATH . '/configs',
	TESTS_PATH . '/controllers/IndexController.php',
	TESTS_PATH . '/controllers',
	TESTS_PATH . '/extends/Tests.class.php',
	TESTS_PATH . '/extends',
	TESTS_PATH . '/models/TestsModel.php',
	TESTS_PATH . '/models',
	TESTS_PATH . '/views/tests/hello.php',
	TESTS_PATH . '/views/tests/widget.php',
	TESTS_PATH . '/views/tests',
	TESTS_PATH . '/views',
	TESTS_PATH . '/widgets/public/footer.php',
	TESTS_PATH . '/widgets/public/header.php',
	TESTS_PATH . '/widgets/public',
	TESTS_PATH . '/widgets/PublicWidget.php',
	TESTS_PATH . '/widgets',
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
