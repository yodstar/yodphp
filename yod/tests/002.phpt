--TEST--
Check for yod application
--SKIPIF--
<?php if (!extension_loaded("yod")) print "skip"; ?>
--FILE--
<?php
error_reporting(E_ALL);
date_default_timezone_set('Asia/Shanghai');

defined('YOD_RUNMODE') or define('YOD_RUNMODE', 1);

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

$yodapp = Yod_Application::app();
$yodapp->run();

class IndexController extends Yod_Controller
{
	public function indexAction()
	{
		$this->_view['tpl_path'] = './tests/views';
		print_r($this);
	}
}
?>
--EXPECTF--
IndexController Object
(
    [_name:protected] => index
    [_action:protected] => index
    [_request:protected] => Yod_Request Object
        (
            [_routed:protected] => 1
            [_dispatched:protected] => 1
            [uri] => 
            [controller] => Index
            [action] => index
            [params] => Array
                (
                )

            [method] => Cli
        )

    [_view:protected] => Array
        (
            [tpl_data] => Array
                (
                    [_PUBLIC_] => /Public/
                )

            [tpl_path] => ./tests/views
            [tpl_file] => 
        )

)
