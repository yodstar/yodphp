--TEST--
Check for yod request
--SKIPIF--
<?php if (!extension_loaded("yod")) print "skip"; ?>
--FILE--
<?php
error_reporting(E_ALL ^ E_WARNING);
date_default_timezone_set('Asia/Shanghai');

defined('YOD_RUNMODE') or define('YOD_RUNMODE', 1);

define('TESTS_PATH', dirname(__FILE__));
include TESTS_PATH . '/clean.php';

is_dir(TESTS_PATH . '/Home') or mkdir(TESTS_PATH . '/Home');
is_dir(TESTS_PATH . '/Home/View') or mkdir(TESTS_PATH . '/Home/View');
is_dir(TESTS_PATH . '/Home/View/tests') or mkdir(TESTS_PATH . '/Home/View/tests');
file_put_contents(TESTS_PATH . '/Home/View/tests/hello.php', <<<PHP
<?php echo \$yodphp; ?>

<?php echo \$hello; ?>


PHP
);

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
$request = new Yod_Request('tests/request');
$request->dispatch();

class TestsController extends Yod_Controller
{
	public function indexAction()
	{
		
	}

    public function requestAction()
    {
        $this->_view['tpl_path'] = './tests/Home/View';
        $this->forward('hello');
    }

    public function helloAction()
    {
        print_r($this);
        $this->assign('yodphp', 'Yod PHP Framework');
        $this->assign(array('hello' => 'Hello World!'));
        $this->display();
    }
}
?>
--EXPECTF--
TestsController Object
(
    [_name:protected] => tests
    [_action:protected] => hello
    [_request:protected] => Yod_Request Object
        (
            [_routed:protected] => 1
            [_dispatched:protected] => 1
            [uri] => tests/request
            [module] => Home
            [controller] => Tests
            [action] => request
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

            [tpl_path] => ./tests/Home/View
            [tpl_file] => 
        )

)
Yod PHP Framework
Hello World!
