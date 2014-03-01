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

is_dir(TESTS_PATH . '/actions') or mkdir(TESTS_PATH . '/actions');
is_dir(TESTS_PATH . '/actions/index') or mkdir(TESTS_PATH . '/actions/index');
file_put_contents(TESTS_PATH . '/actions/index/IndexAction.php', <<<PHP
<?php
class IndexAction extends Yod_Action
{
	public function run()
	{
		\$this->import('Tests');
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

is_dir(TESTS_PATH . '/extends') or mkdir(TESTS_PATH . '/extends');
file_put_contents(TESTS_PATH . '/extends/Tests.class.php', <<<PHP
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

is_dir(TESTS_PATH . '/views') or mkdir(TESTS_PATH . '/views');
is_dir(TESTS_PATH . '/views/tests') or mkdir(TESTS_PATH . '/views/tests');
file_put_contents(TESTS_PATH . '/views/tests/widget.php', <<<PHP
<?php echo \$yodphp; ?>

<?php \$this->widget('public/header'); ?>

<?php echo \$hello; ?>

<?php \$this->widget('public/footer'); ?>

PHP
);

is_dir(TESTS_PATH . '/widgets') or mkdir(TESTS_PATH . '/widgets');
is_dir(TESTS_PATH . '/widgets/public') or mkdir(TESTS_PATH . '/widgets/public');
file_put_contents(TESTS_PATH . '/widgets/PublicWidget.php', <<<PHP
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

file_put_contents(TESTS_PATH . '/widgets/public/header.php', 'Header');
file_put_contents(TESTS_PATH . '/widgets/public/footer.php', "Footer\r\n");

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
