--TEST--
Check for yod database
--SKIPIF--
<?php
if (!extension_loaded("yod") || !class_exists('PDO', false)) {
	print "skip";
}
?>
--FILE--
<?php
error_reporting(E_ALL);
date_default_timezone_set('Asia/Shanghai');

defined('YOD_RUNMODE') or define('YOD_RUNMODE', 1);

define('TESTS_PATH', dirname(__FILE__));
include TESTS_PATH . '/clean.php';

is_dir(TESTS_PATH . '/configs') or mkdir(TESTS_PATH . '/configs');
file_put_contents(TESTS_PATH . '/configs/db_dsn.config.php', <<<PHP
<?php
return array(
	'type' => 'pdo',
	'dsn' => 'mysql:host=localhost;port=3306;dbname=test',
	'host' => 'localhost',
	'user' => 'root',
	'pass' => '123456',
	'dbname' => 'test',
	'prefix' => 'yod_',
	'slaves' => array(
		
	),
);

PHP
);

file_put_contents(TESTS_PATH . '/configs/tpl_data.config.php', <<<PHP
<?php
return array(
	'_PUBLIC_' => '/Public/',
);

PHP
);

define('YOD_RUNPATH', dirname(__FILE__));

$config = array(
	'db_dsn2' => array(
		'type' => 'pdo',
		'dsn' => 'mysql:host=localhost;port=3306;dbname=test',
		'host' => 'localhost',
		'user' => 'root',
		'pass' => '123456',
		'dbname' => 'test',
		'prefix' => 'yod_',
		'slaves' => array(
			'dsn' => 'mysql:host=localhost;port=3306;dbname=test',
			'user' => 'root',
			'pass' => '123456',
		),
	),
);

class IndexController extends Yod_Controller
{
	public function indexAction()
	{

		$db = Yod_Database::db();

		$fields = array(
			'id' => 'int(11) NOT NULL AUTO_INCREMENT COMMENT \'ID\'',
			'title' => 'varchar(255) NOT NULL COMMENT \'标题\'',
			'content' => 'text DEFAULT NULL COMMENT \'内容\'',
			'updated' => 'int(11) NOT NULL DEFAULT \'0\' COMMENT \'更新时间\'',
			'created' => 'int(11) NOT NULL DEFAULT \'0\' COMMENT \'创建时间\'',
			'status' => 'tinyint(2) NOT NULL DEFAULT \'0\' COMMENT \'状态\'',
			'PRIMARY' => 'KEY (`id`)',
		);
		$extend = 'ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT=\'Demo\' AUTO_INCREMENT=1';
		$create = $db->create($fields, 'tests', $extend);
		echo "create:"; var_dump($create);

		$tests = Yod::model('Tests');

		$data = array(
			'title' => 'Tests',
			'content' => 'Yod Framework',
			'created' => 1234567890,
		);
		$save = $tests->save($data);
		echo "save:"; var_dump($save);

		echo "find:"; print_r($tests->find());

		$demo = Yod::model('demo');

		$data['updated'] = 1234567891;
		$update = $demo->update($data, 'id = :id', array(':id' => 1));
		echo "update:"; var_dump($update);

		$find = $demo->find('id = :id', array(':id' => 1));
		echo "find:"; print_r($find);

		echo "select:"; print_r($demo->select());

		echo "count:"; var_dump($demo->count());

		echo "remove:"; var_dump($demo->remove());

		echo "count:"; var_dump($demo->count());

		$execute = $db->execute('DROP TABLE yod_tests');
		echo "execute:"; var_dump($execute);

	}
}

class DemoModel extends Yod_Model
{
	public $_dsn = 'db_dsn2';
	public $_table = 'tests';
	
}
?>
--EXPECTF--
create:bool(true)
save:string(1) "1"
find:Array
(
    [id] => 1
    [title] => Tests
    [content] => Yod Framework
    [updated] => 0
    [created] => 1234567890
    [status] => 0
)
update:int(1)
find:Array
(
    [id] => 1
    [title] => Tests
    [content] => Yod Framework
    [updated] => 1234567891
    [created] => 1234567890
    [status] => 0
)
select:Array
(
    [0] => Array
        (
            [id] => 1
            [title] => Tests
            [content] => Yod Framework
            [updated] => 1234567891
            [created] => 1234567890
            [status] => 0
        )

)
count:string(1) "1"
remove:int(1)
count:string(1) "0"
execute:bool(true)
