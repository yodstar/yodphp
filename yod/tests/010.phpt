--TEST--
Check for yod urlrules1
--SKIPIF--
<?php if (!extension_loaded("yod")) print "skip"; ?>
--FILE--
<?php
error_reporting(E_ALL);
date_default_timezone_set('Asia/Shanghai');

defined('YOD_RUNMODE') or define('YOD_RUNMODE', 1);
defined('YOD_RUNPATH') or define('YOD_RUNPATH', dirname(__FILE__));

$config = array(
	'urlrules' => array(
		'rule1' => 'index/rule1/p/v',
		'index/rule2/:id' => 'index/rule2',
		'index/rule3/:id/edit' => 'index/rule3/action/edit',
	),
);

class IndexController extends Yod_Controller
{
	public function indexAction()
	{
		$this->forward('rule1');
	}

	public function rule1Action()
	{
		print_r($this->_request);

		$this->forward('index/rule2/1');
	}

	public function rule2Action()
	{
		print_r($this->_request);

		$this->forward('index/rule3/1/edit');
	}

	public function rule3Action()
	{
		print_r($this->_request);

        $this->forward('index/rule4/id/2');
	}

    public function rule4Action()
    {
        print_r($_REQUEST);
    }
}

?>
--EXPECTF--
Yod_Request Object
(
    [_routed:protected] => 1
    [_dispatched:protected] => 1
    [uri] => 
    [module] => Home
    [controller] => Index
    [action] => index
    [params] => Array
        (
        )

    [method] => Cli
)
Yod_Request Object
(
    [_routed:protected] => 1
    [_dispatched:protected] => 1
    [uri] => index/rule2/1
    [module] => Home
    [controller] => Index
    [action] => rule2
    [params] => Array
        (
            [id] => 1
        )

    [method] => Cli
)
Yod_Request Object
(
    [_routed:protected] => 1
    [_dispatched:protected] => 1
    [uri] => index/rule3/1/edit
    [module] => Home
    [controller] => Index
    [action] => rule3
    [params] => Array
        (
            [id] => 1
            [action] => edit
        )

    [method] => Cli
)
Array
(
    [id] => 2
    [action] => edit
)
