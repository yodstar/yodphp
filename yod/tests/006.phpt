--TEST--
Check for yod model
--SKIPIF--
<?php if (!extension_loaded("yod")) print "skip"; ?>
--FILE--
<?php
error_reporting(E_ALL);
date_default_timezone_set('Asia/Shanghai');

defined('YOD_RUNMODE') or define('YOD_RUNMODE', 1);

define('TESTS_PATH', dirname(__FILE__));
include TESTS_PATH . '/clean.php';

is_dir(TESTS_PATH . '/Home') or mkdir(TESTS_PATH . '/Home');
is_dir(TESTS_PATH . '/Home/Model') or mkdir(TESTS_PATH . '/Home/Model');
file_put_contents(TESTS_PATH . '/Home/Model/TestsModel.php', <<<PHP
<?php
class TestsModel extends Yod_Model
{
	public function yodphp()
	{
		echo 'Yod ';
	}

	public function show()
	{
		echo 'Framework ';
	}
}

PHP
);

define('YOD_RUNPATH', dirname(__FILE__));

$config = array(
	//db
	'db_dsn' => array(
		'type' => 'pdo',
		'dsn' => 'mysql:host=localhost;port=3306;dbname=test',
		'host' => 'localhost',
		'user' => 'root',
		'pass' => '123456',
		'dbname' => 'test',
		'prefix' => 'yod_',
		'slaves' => array(

		),
	),
	//tpl
	'tpl_data' => array(
		'_PUBLIC_' => '/Public/',
	),
);

class IndexController extends Yod_Controller
{
	public function indexAction()
	{
		$tests = Yod::model('Tests');
		$tests->yodphp();

		$tests = Yod::model('Tests');
		$tests->show();

		$hello = Yod::model('Hello');
		$hello->show();
	}
}

class HelloModel extends Yod_Model
{
	public function show()
	{
		echo 'Hello World!';
	}
}
?>
--EXPECTF--
Yod Framework Hello World!
