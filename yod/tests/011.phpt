--TEST--
Check for yod urlrules2
--SKIPIF--
<?php if (!extension_loaded("yod")) print "skip"; ?>
--FILE--
<?php
error_reporting(E_ALL);
date_default_timezone_set('Asia/Shanghai');

defined('YOD_RUNMODE') or define('YOD_RUNMODE', 1);
defined('YOD_RUNPATH') or define('YOD_RUNPATH', dirname(__FILE__));

$config = array(
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
		$this->forward('en/index/rule1');
	}

	public function rule1Action()
	{
		print_r($this->_request);

		$this->forward('zh/index/rule2/1');
	}

	public function rule2Action()
	{
		print_r($this->_request);
    }
}

?>
--EXPECTF--
Yod_Request Object
(
    [_routed:protected] => 1
    [_dispatched:protected] => 1
    [uri] => en/index/rule1
    [controller] => Index
    [action] => rule1
    [params] => Array
        (
        )

    [method] => Cli
)
Yod_Request Object
(
    [_routed:protected] => 1
    [_dispatched:protected] => 1
    [uri] => zh/index/rule2/1
    [controller] => Index
    [action] => rule2
    [params] => Array
        (
            [lang] => zh
            [id] => 1
        )

    [method] => Cli
)
