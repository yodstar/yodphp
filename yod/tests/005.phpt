--TEST--
Check for yod action
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
is_dir(TESTS_PATH . '/Home/Action') or mkdir(TESTS_PATH . '/Home/Action');
is_dir(TESTS_PATH . '/Home/Action/index') or mkdir(TESTS_PATH . '/Home/Action/index');
file_put_contents(TESTS_PATH . '/Home/Action/index/IndexAction.php', <<<PHP
<?php
class IndexAction extends Yod_Action
{
	public function run()
	{
		Tests::yodphp();
		\$this->assign(array(
			'yodphp' => 'Yod PHP Framework',
			'hello' => 'Hello World!',
		));
		\$this->display('/tests/widget');
	}
}

PHP
);

is_dir(TESTS_PATH . '/Extend') or mkdir(TESTS_PATH . '/Extend');
file_put_contents(TESTS_PATH . '/Extend/Tests.php', <<<PHP
<?php
class Tests
{
	public static function yodphp()
	{
		echo 'yodphp';
	} 
}

PHP
);

is_dir(TESTS_PATH . '/Home/View') or mkdir(TESTS_PATH . '/Home/View');
is_dir(TESTS_PATH . '/Home/View/tests') or mkdir(TESTS_PATH . '/Home/View/tests');
file_put_contents(TESTS_PATH . '/Home/View/tests/widget.php', <<<PHP
<?php echo \$yodphp; ?>

<?php \$this->widget('public/header'); ?>

<?php echo \$hello; ?>

<?php \$this->widget('public/footer'); ?>

PHP
);

is_dir(TESTS_PATH . '/Widget') or mkdir(TESTS_PATH . '/Widget');
is_dir(TESTS_PATH . '/Widget/public') or mkdir(TESTS_PATH . '/Widget/public');
file_put_contents(TESTS_PATH . '/Widget/PublicWidget.php', <<<PHP
<?php
class PublicWidget extends Yod_Widget
{
    public function headerAction()
    {
        \$this->display('header', array('yodphp' => '<sup>Beta</sup>'));
    }

    public function footerAction()
    {
        \$this->display('footer', array('footer' => 'Copyright'));
    }
}

PHP
);

file_put_contents(TESTS_PATH . '/Widget/public/header.php', 'Header');
file_put_contents(TESTS_PATH . '/Widget/public/footer.php', "Footer\r\n");

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

}
?>
--EXPECTF--
yodphpYod PHP Framework
Header
Hello World!
Footer
