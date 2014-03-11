# yodphp - Yod Framework for PHP

It is a simple PHP framework which is written in C and build as PHP extension.

## Requirement
- PHP 5.2 +

## Homepage
- http://yodphp.com

## Tutorial

### installation
- Compiling Yod:

```
$PHP_BIN/phpize
./configure --with-php-config=$PHP_BIN/php-config
make
make install
```

- Compiling Yod with debug mode:

```
$PHP_BIN/phpize
./configure --enable-yod-debug --with-php-config=$PHP_BIN/php-config
make
make install
```

- Cross compiling Yod for i686-linux:

```
$PHP_BIN/phpize
./configure --enable-yod-debug --with-php-config=$PHP_BIN/php-config \
CCFLAGS="-m32" CPPFLAGS="-m32" CXXFLAGS="-m32" CFLAGS="-m32"
make
make install
```

### layout
- A classic Application directory layout:

```
- index.php	// Application entry
+ public
+ yodphp
	+ drivers	// Database Drivers
	- yodphp.php	// Yodphp entry
+ app
	+ actions	// Other actions
	+ configs
		- config.php	// Configure 
	+ controllers
		- IndexController.php	// Default controller
	+ extends	// Extends
	+ models	// Models
		- DemoModel.php
	+ views
		+ index	// View templates for default controller
			- index.php

```

- Multiple entry Application directory layout:

```
- index.php	// Application index entry
- hello.php	// Application hello entry
+ public
+ app

```

### index.php
index.php is the application entry

```php
<?php
//set_time_limit(0);
error_reporting(E_ALL);
date_default_timezone_set('Asia/Shanghai');

defined('YOD_RUNPATH') or define('YOD_RUNPATH', dirname(__FILE__) . '/app');

```

### hello.php
hello.php is the application hello entry

```php
<?php
//set_time_limit(0);
error_reporting(E_ALL);
date_default_timezone_set('Asia/Shanghai');

defined('YOD_RUNPATH') or define('YOD_RUNPATH', dirname(__FILE__) . '/app');

class HelloController extends Yod_Controller
{
	public function indexAction()
	{
		$this->assign('content', $this->model('Hello')->content());
		$this->display('/index/index');
	}

	public function errorAction()
	{
		echo '<pre>';
		print_r($this);
	}
}

class HelloModel extends Yod_Model
{
    public function content()
    {
        return 'Hello World!';
    }
}

```

### config.php
config.php is the application config file

```php
<?php
return array(
	// db_dsn
	'db_dsn' => array(
		'type'   => 'pdo',
		'pdsn'   => 'mysql:host=localhost;port=3306;dbname=test',
		'host'   => 'localhost',
		'port'   => '3306',
		'user'   => 'root',
		'pass'   => '123456',
		'dbname' => 'test',
		'prefix' => 'yod_',
		'charset' => 'utf8',
		// slaves
		'slaves' => array(
			array(
				'dsn'    => 'mysql:host=localhost;port=3306;dbname=test',
				'user'   => 'root',
				'pass'   => '123456',
			),
		),
	),
	// tpl_data
	'tpl_data' => array(
		'_PUBLIC_' => '/public/'
	),
	// url_rules (v1.3.1+)
	'url_rules' => array(
		'en/*' => '*',
		'zh/*' => array('*', array('lang' => 'zh')),
		'rule1' => 'index/rule1/p/v',
		'index/rule1' => 'index/rule1/id/1',
		'index/rule2/:id' => 'index/rule2',
		'index/rule3/:id/edit' => 'index/rule3/action/edit',
		'index/rule4/:id/remove' => array('index/rule4', array('action' => 'remove'),
	),
);

```

### default controller
In Yodphp, the default controller is named IndexController

```php
<?php
class IndexController extends Yod_Controller {
	// default action name
	public function indexAction() {
		$this->assign('content', 'Hello World');
		$this->display();
	}
}

```

###view script
The view script for default controller and default action is in the app/views/index/index.php

```php
<html>
  <head>
    <title>Hello World</title>
  </head>
  <body>
    <?php echo $content; ?>
  </body>
</html>
```
