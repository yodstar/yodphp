--TEST--
Check for yod instantiate
--SKIPIF--
<?php if (!extension_loaded("yod")) print "skip"; ?>
--FILE--
<?php
error_reporting(E_ALL);
date_default_timezone_set('Asia/Shanghai');

defined('YOD_RUNMODE') or define('YOD_RUNMODE', 0);

$yodapp = new Yod_Application();
$yodapp->run();

class IndexController extends Yod_Controller
{
    public function indexAction()
    {
        $yodapp = Yod_Application::app();
        var_dump($GLOBALS['yodapp'] === $yodapp);
    }
}
?>
--EXPECTF--
bool(true)
