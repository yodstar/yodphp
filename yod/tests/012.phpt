--TEST--
Check for yod_base class
--SKIPIF--
<?php if (!extension_loaded("yod")) print "skip"; ?>
--FILE--
<?php
error_reporting(E_ALL);
date_default_timezone_set('Asia/Shanghai');

defined('YOD_RUNMODE') or define('YOD_RUNMODE', 1);
defined('YOD_RUNPATH') or define('YOD_RUNPATH', dirname(__FILE__));

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
    //url
	'url_rules' => array(
		'en/*' => '*',
        'zh/*' => array('*', array('lang' => 'zh')),
        'index/rule2/:id' => 'index/rule2',
	),
);

class IndexController extends Yod_Controller
{
	public function indexAction()
	{
		$config = Yod::config('url_rules');
        print_r($config);

        $this->forward('model');
	}

	public function modelAction()
	{
		$model = Yod::model();
        print_r($model);
	}

}

?>
--EXPECTF--
Array
(
    [en/*] => *
    [zh/*] => Array
        (
            [0] => *
            [1] => Array
                (
                    [lang] => zh
                )

        )

    [index/rule2/:id] => index/rule2
)
Yod_Model Object
(
    [_db:protected] => Yod_DbPdo Object
        (
            [_config:protected] => Array
                (
                    [type] => pdo
                    [dsn] => mysql:host=localhost;port=3306;dbname=test
                    [host] => localhost
                    [user] => root
                    [pass] => 123456
                    [dbname] => test
                    [prefix] => yod_
                    [slaves] => Array
                        (
                        )

                )

            [_driver:protected] => Yod_DbPdo
            [_prefix:protected] => yod_
            [_result:protected] => 
            [_linkid:protected] => 
            [_linkids:protected] => Array
                (
                )

            [_locked:protected] => 
            [_lastquery:protected] => 
        )

    [_dsn:protected] => db_dsn
    [_name:protected] => 
    [_table:protected] => 
    [_prefix:protected] => yod_
)
