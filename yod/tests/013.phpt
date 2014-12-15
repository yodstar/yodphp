--TEST--
Check for yod_server class
--SKIPIF--
<?php if (!extension_loaded("yod") || version_compare(PHP_VERSION, '5.4.0') < 0) print "skip"; ?>
--FILE--
<?php
set_time_limit(0);
error_reporting(E_ALL);
date_default_timezone_set('Asia/Shanghai');

define('YOD_RUNMODE', 0);
define('TESTS_PATH', dirname(__FILE__));

$php = null;
if (getenv('TEST_PHP_EXECUTABLE')) {
    $php = getenv('TEST_PHP_EXECUTABLE');
    if ($php=='auto') {
        $php = getcwd() . '/../sapi/cli/php';
    }
}
if (empty($php) || !file_exists($php)) {
    exit('environment variable TEST_PHP_EXECUTABLE must be set to specify PHP executable!');
}

ob_start();
phpinfo(INFO_GENERAL);
$pinfo = ob_get_contents();
ob_end_clean();

preg_match('/Loaded Configuration File => (.+\.ini)/i', $pinfo, $match);
if (empty($match[1])) exit('INI file not found');
$yod = dirname($match[1]) . '/modules/yod.so';

$dir = TESTS_PATH . '/server';
$cmd = "{$php} -d extension={$yod} -S 127.0.0.1:8013 -t {$dir}";
$tsh = TESTS_PATH . '/013_t.sh';

is_dir($dir) or mkdir($dir) or exit('mkdir failed!');
if (file_put_contents($tsh, "#!/bin/sh

{$cmd}
", FILE_BINARY) === false) {
    exit("Cannot create test shell script - 013_t.sh");
}
chmod($tsh, 0755);
exec("nohup sh {$tsh} > /dev/null 2>& 1 &");
usleep(100);

file_put_contents("{$dir}/close.php", <<<PHP
<?php
\$mypid = getmypid();
\$mypid and exec("kill -9 {\$mypid}");

PHP
);

file_put_contents("{$dir}/index.php", <<<PHP
<?php
error_reporting(E_ALL);
date_default_timezone_set('Asia/Shanghai');

defined('YOD_RUNMODE') or define('YOD_RUNMODE', 0);
defined('YOD_RUNPATH') or define('YOD_RUNPATH', dirname(__FILE__));

class TestServer
{
    public \$name = '';

    public function test(\$name)
    {
        \$this->name = \$name;
        return true;
    }

}

\$server = new Yod_Server(new TestServer());
\$server->handle();

PHP
);

$num = 0;
$fp = @fsockopen('127.0.0.1', 8013, $errno, $errmsg);
while (!$fp) {
    usleep(1000);
    $fp = @fsockopen('127.0.0.1', 8013, $errno, $errmsg);
    if (++$num > 1000) {
        break;
    }
}
if ($fp) {
    $data = '{"client":"Yod_Client","method":"test","params":["Yod_Server"],"extra":{"client":"Yod_Client"}}';

    fputs($fp, "POST /index.php HTTP/1.0\n");  
    fputs($fp, "Host: 127.0.0.1\n");
    fputs($fp, "Content-type: application/x-www-form-urlencoded\n");
    fputs($fp, "Content-length: " . strlen($data) . "\n");
    fputs($fp, "Connection: close\n\n");
    fputs($fp, "{$data}\n");

    $line = fgets($fp, 1024);  
    if (!preg_match('/^HTTP\/1\.. 200/', $line)) {
        echo "error: {$line}";
    }

    $header = 1;
    $result = '';
    while (!feof($fp)) {  
        $line = fgets($fp, 1024);
        if ($header && ($line == "\n" || $line == "\r\n")) {  
            $header = 0;  
        } elseif (!$header) {  
            $result .= $line;  
        }
    }
    fclose($fp);  

    echo $result;

}

$fp = @fsockopen('127.0.0.1', 8013, $errno, $errmsg);
if ($fp) {
    fputs($fp, "POST /close.php HTTP/1.0\n");  
    fputs($fp, "Host: 127.0.0.1\n");
    fputs($fp, "Content-type: application/x-www-form-urlencoded\n");
    fputs($fp, "Content-length: 0\n");
    fputs($fp, "Connection: close\n\n");
    fputs($fp, "\n");
    fclose($fp); 
}
@unlink($tsh);

?>
--EXPECTF--
{"server":"Yod_Server","status":1,"data":true,"extra":{"name":"Yod_Server","client":"Yod_Client"}}
