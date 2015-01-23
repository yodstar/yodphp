--TEST--
Check for yod_client class
--SKIPIF--
<?php if (!extension_loaded("yod")) print "skip"; ?>
--FILE--
<?php
error_reporting(E_ALL);
date_default_timezone_set('Asia/Shanghai');

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

$dir = TESTS_PATH . '/client';
$srv = $dir . '/server.php';
$cmd = "{$php} -f {$srv}";
$tsh = TESTS_PATH . '/014_t.sh';

is_dir($dir) or mkdir($dir);
file_put_contents($srv, <<<PHP
<?php
set_time_limit(0);
error_reporting(E_ALL);

\$host = '127.0.0.1';
\$port = 8014;

if (\$socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP)) {
    socket_bind(\$socket, \$host, \$port);
    socket_listen(\$socket, 5);

    if ((\$accept = socket_accept(\$socket)) === false) {
        \$errer = socket_strerror(socket_last_error(\$socket));
        exit(\$errer);
    }

    socket_read(\$accept, 8192);

    \$data = "HTTP/1.0 200 OK\\nContent-Type: text/json;charset=UTF-8\\r\\nContent-Length: 86\\n\\n";
    \$data .= '{"server":"Yod_Server","status":1,"data":true,"extra":{"name":"Yod_Server"}}';

    socket_write(\$accept, \$data, strlen(\$data));

    socket_close(\$accept);

    socket_close(\$socket);
}

PHP
);

if (file_put_contents($tsh, "#!/bin/sh

{$cmd}
", FILE_BINARY) === false) {
    exit("Cannot create test shell script - 014_t.sh");
}
chmod($tsh, 0755);
exec("nohup sh {$tsh} > /dev/null 2>& 1 &");

defined('YOD_RUNMODE') or define('YOD_RUNMODE', 0);
defined('YOD_RUNPATH') or define('YOD_RUNPATH', dirname(__FILE__));

$num = 0;
$errmsg = null;
$client = new Yod_Client('http://127.0.0.1:8014', 'test', 1);
try{
    $result = $client->test();
}catch(Exception $e){
    $errmsg = $e->getMessage();
}

while ($errmsg) {
    usleep(1000);
    $errmsg = null;
    $client = new Yod_Client('http://127.0.0.1:8014', 'test', 1);
    try{
        $result = $client->test();
        break;
    }catch(Exception $e){
        $errmsg = $e->getMessage();
        //echo $errmsg . PHP_EOL;
    }
    if (++$num > 1000) {
        break;
    }
}
print_r($client);
@unlink($tsh);

?>
--EXPECTF--
Yod_Client Object
(
    [_url:protected] => http://127.0.0.1:8014
    [_extra:protected] => Array
        (
            [name] => Yod_Server
        )

    [_handle:protected] => test
    [_timeout:protected] => 1
    [_debug:protected] => 
)
