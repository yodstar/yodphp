<?php
// +----------------------------------------------------------------------
// | yodphp [ Yod Framework for PHP ]
// +----------------------------------------------------------------------
// | Copyright (c) 2013 http://yodphp.com All rights reserved.
// +----------------------------------------------------------------------
// | Licensed ( http://www.apache.org/licenses/LICENSE-2.0 )
// +----------------------------------------------------------------------
// | Author: Baoqiang Su <zmrnet@qq.com>
// +----------------------------------------------------------------------

// yodphp constant
defined('YOD_RUNTIME') or define('YOD_RUNTIME', microtime(true));
defined('YOD_VERSION') or define('YOD_VERSION', '1.3.3');
defined('YOD_FORWARD') or define('YOD_FORWARD', 5);
defined('YOD_RUNMODE') or define('YOD_RUNMODE', 3);
defined('YOD_CHARSET') or define('YOD_CHARSET', 'utf-8');
defined('YOD_VIEWEXT') or define('YOD_VIEWEXT', '.php');
defined('YOD_PATHVAR') or define('YOD_PATHVAR', '');
defined('YOD_EXTPATH') or define('YOD_EXTPATH', dirname(__FILE__));

// yodphp autorun
Yod_Application::autorun();

/**
 * Yod_Application
 * 
 */
final class Yod_Application
{
	protected static $_app;
	protected static $_config = array();
	protected static $_imports = array();
	protected static $_plugins = array();
	protected static $_running = false;

	protected $_request = null;

	/**
	 * __construct
	 * @access public
	 * @param mixed $config
	 * @return void
	 */
	public function __construct($config = null)
	{
		if (is_object(self::$_app)) {
			trigger_error('Only one application can be initialized', E_USER_ERROR);
			return;
		}

		defined('YOD_RUNPATH') or define('YOD_RUNPATH', dirname(__FILE__));
		
		// errorlog
		if ((YOD_RUNMODE & 2) && defined('YOD_LOGPATH')) {
			set_error_handler(array('Yod_Application', 'errorlog'));
		}

		// autoload
		spl_autoload_register(array('Yod_Application', 'autoload'));

		// config
		if (is_array($config)) {
			self::$_config = $config;
		} else {
			if (!is_string($config)) {
				$config = YOD_RUNPATH . '/configs/config.php';
			}
			if (is_file($config)) {
				self::$_config = include($config);
			} else {
				self::$_config = array();
				$scandir = dirname($config);
				if (is_dir($scandir) && ($handle = opendir($scandir))) {
					while (($file = readdir($handle)) != false) {
						if (substr($file, -11) == '.config.php') {
							$key = substr($file, 0, -11);
							$value = include($scandir .'/'. $file);
							if (is_array($value)) {
								if ($key == 'base') {
									self::$_config = array_merge(self::$_config, $value);
								} elseif (isset(self::$_config[$key])){
									if (is_array(self::$_config[$key])) {
										$value = array_merge(self::$_config[$key], $value);
									}
								}
								self::$_config[$key] = $value;
							}
						}
					}
					closedir($handle);
				}
			}
		}
		if (is_file(YOD_RUNPATH . '/config.php')) {
			$develop = include YOD_RUNPATH . '/config.php';
			if (is_array($develop)) {
				foreach ($develop as $key => $value) {
					if (isset(self::$_config[$key]) && is_array(self::$_config[$key]) && is_array($value)) {
						self::$_config[$key] = array_merge(self::$_config[$key], $value);
					} else {
						self::$_config[$key] = $develop[$key];
					}
				}
			}
		}
		if (isset($GLOBALS['config']) && is_array($GLOBALS['config'])) {
			foreach ($GLOBALS['config'] as $key => $value) {
				if (isset(self::$_config[$key]) && is_array(self::$_config[$key]) && is_array($value)) {
					self::$_config[$key] = array_merge(self::$_config[$key], $value);
				} else {
					self::$_config[$key] = $GLOBALS['config'][$key];
				}
			}
		}

		// request
		$this->_request = new Yod_Request();

		self::$_app = $this;
	}

	/**
	 * run
	 * @access public
	 * @param void
	 * @return void
	 */
	public function run()
	{
		if (self::$_running) {
			trigger_error('An application instance already running', E_USER_WARNING);
			return;
		}
		self::$_running = true;

		// dispatch
		$this->_request->dispatch();
	}

	/**
	 * app
	 * @access public
	 * @param mixed $config
	 * @return Yod_Application
	 */
	public static function app($config = null)
	{
		if (self::$_app) {
			return self::$_app;
		}
		return new self($config);
	}

	/**
	 * config
	 * @access public
	 * @param string $name
	 * @return mixed
	 */
	public static function config($name = null)
	{
		if (is_null($name)) {
			return self::$_config;
		}
		if (isset(self::$_config[$name])) {
			return self::$_config[$name];
		} elseif (strstr($name, '.')) {
			$name = explode('.', $name);
			$value = self::$_config;
			foreach ($name as $skey) {
				if (isset($value[$skey])) {
					$value = $value[$skey];
				} else {
					return null;
				}
			}
			return $value;
		}
		return null;
	}

	/**
	 * import
	 * @access public
	 * @param string $alias
	 * @param string $classext
	 * @return boolean
	 */
	public static function import($alias, $classext = '.class.php')
	{
		$classfile = trim(str_replace('\\', '/', str_replace('.', '/', $alias)), '/');
		$classname = basename($classfile);

		if (empty(self::$_imports[$alias])) {
			if (class_exists($classname, false) || interface_exists($classname, false)) {
				return self::$_imports[$alias] = true;
			}

			if (strncasecmp($classfile, 'yod/', 4) == 0) {
				$classpath = YOD_EXTPATH . '/extends/' . substr($classfile, 4) . $classext;
			} else {
				$classpath = YOD_RUNPATH . '/extends/' . $classfile . $classext;
			}
			if (is_file($classpath)) include $classpath;
			self::$_imports[$alias] = $classpath;
		}

		return class_exists($classname, false) || interface_exists($classname, false);
	}

	/**
	 * plugin
	 * @access public
	 * @param string $alias
	 * @param string $classext
	 * @return mixed
	 */
	public static function plugin($alias, $classext = '.class.php')
	{
		$classfile = trim(str_replace('\\', '/', str_replace('.', '/', $alias)), '/');
		$classname = basename($classfile);

		if (empty(self::$_plugins[$alias])) {
			if (!class_exists($classname, false) && !interface_exists($classname, false)) {
				if (strncasecmp($classfile, 'yod/', 4) == 0) {
					if (strncasecmp($classname, 'Yod_', 4)) {
						$classname = 'Yod_' . $classname;
					}
					$classpath = YOD_EXTPATH . '/plugins/' . substr($classfile, 4) . $classext;
				} elseif (strncasecmp($classname, 'Yod_', 4) == 0) {
					$classpath = YOD_EXTPATH . '/plugins/' . substr($classfile, 0, -strlen($classname)) . substr($classname, 4) . $classext;
				} else {
					$classpath = YOD_RUNPATH . '/plugins/' . $classfile . $classext;
				}
				if (is_file($classpath)) include $classpath;
			}
			
			$config = self::config('plugins.' . strtolower($alias));
			if (is_null($config)) {
				self::$_plugins[$alias] = new $classname();
			} else {
				self::$_plugins[$alias] = new $classname($config);
			}
		}

		return self::$_plugins[$alias];
	}

	/**
	 * autorun
	 * @access public
	 * @param void
	 * @return void
	 */
	public static function autorun()
	{
		if (self::$_running) {
			return;
		}
		if (YOD_RUNMODE & 1) {
			if (defined('YOD_RUNPATH')) {
				if (is_file($_SERVER['SCRIPT_FILENAME'])) {
					include $_SERVER['SCRIPT_FILENAME'];
				}
				if (isset($config)) {
					$GLOBALS['config'] = $config;
				}
				Yod_Application::app()->run();
				exit;
			} else {
				define('YOD_RUNPATH', dirname(__FILE__));
			}
		}
	}

	/**
	 * autoload
	 * @access public
	 * @param string $classname
	 * @return boolean
	 */
	public static function autoload($classname)
	{
		$classfile = $classname;
		// class name with namespace in PHP 5.3
		if (strpos($classname, '\\') !== false) {
			$classfile = str_replace('\\', '_', $classname);
		}

		if (strncasecmp($classfile, 'Yod_', 4) == 0) { // yodphp extends class
			if (strncasecmp($classfile, 'Yod_Db', 6) == 0) {
				$directory = '/drivers/';
			} else {
				$directory = '/extends/';
			}
			$classpath = YOD_EXTPATH . $directory . substr($classfile, 4) . '.class.php';
		} else {
			if (strncasecmp(substr($classname, -10), 'Controller', 10) == 0) {
				$directory = '/controllers/';
			} elseif (strncasecmp(substr($classname, -5), 'Model', 5) == 0) {
				$directory = '/models/';
			} else {
				$directory = '/extends/';
				$classfile = $classfile . '.class';
			}
			$classpath = YOD_RUNPATH . $directory . $classfile . '.php';
		}

		if (is_file($classpath)) include $classpath;

		return class_exists($classname, false) || interface_exists($classname, false);
	}

	/**
	 * errorlog
	 * @access public
	 * @param int $errno
	 * @param string $errstr
	 * @param string $errfile
	 * @param int $errline
	 * @param array $errcontext
	 * @return boolean
	 */
	public static function errorlog($errno, $errstr, $errfile = '', $errline = 0, $errcontext = array())
	{
		switch ($errno) {
		 	case E_ERROR:
		 	case E_CORE_ERROR:
		 	case E_COMPILE_ERROR:
		 	case E_USER_ERROR:
		 	case E_RECOVERABLE_ERROR:
		 		$errtype = 'Error';
		 		break;
		 	case E_WARNING:
		 	case E_CORE_WARNING:
		 	case E_COMPILE_WARNING:
		 	case E_USER_WARNING:
		 		$errtype = 'Warning';
		 		break;
		 	case E_PARSE:
		 		$errtype = 'Parse';
		 		break;
		 	case E_NOTICE:
		 	case E_USER_NOTICE:
		 		$errtype = 'Notice';
		 		break;
		 	case E_STRICT:
		 		$errtype = 'Strict';
		 		break;
		 	case E_DEPRECATED:
		 	case E_USER_DEPRECATED:
		 		$errtype = 'Deprecated';
		 		break;
		 	default:
		 		$errtype = 'Unknown';
		 		break;
		}
		$logtime = date('Y-m-d H:i:s');
		$logusec = (microtime(true) - time()) * 1000000;
		$errfile = empty($errfile) ? 'Unknown' : $errfile;
		$logdata = sprintf("[%s %06d] %s: %s in %s(%d)\n", $logtime, $logusec, $errtype, $errstr, $errfile, $errline);
		$logfile = YOD_LOGPATH . '/errors.log';
		is_dir(YOD_LOGPATH) or mkdir(YOD_LOGPATH);
		file_put_contents($logfile, $logdata, FILE_APPEND);

		return false;
	}

	/**
	 * __destruct
	 * @access public
	 * @param void
	 */
	public function __destruct()
	{
		$runtime = (microtime(true) - YOD_RUNTIME) * 1000;
		$logdata = '<hr>runtime:' . $runtime . 'ms'. PHP_EOL;
		if (YOD_RUNMODE & 4) {
			echo $logdata;
		}
		if ((YOD_RUNMODE & 8) && defined('YOD_LOGPATH')) {
			$logfile = YOD_LOGPATH . '/debugs.log';
			is_dir(YOD_LOGPATH) or mkdir(YOD_LOGPATH);
			file_put_contents($logfile, $logdata, FILE_APPEND);
		}
	}

}

/**
 * Yod_Request
 * 
 */
final class Yod_Request
{
	protected $_routed = false;
	protected $_dispatched = false;

	public $uri;
	public $controller;
	public $action;
	public $params;
	public $method;

	/**
	 * __construct
	 * @access public
	 * @param string $route
	 * @return void
	 */
	public function __construct($route = null)
	{
		if (isset($_SERVER['REQUEST_METHOD'])) {
			$this->method = $_SERVER['REQUEST_METHOD'];
		} else {
			if (!strncasecmp(PHP_SAPI, 'cli', 3)) {
				$this->method = 'CLI';
			} else {
				$this->method = 'Unknown';
			}
		}

		empty($route) or $this->route($route);
	}

	/**
	 * route
	 * @access public
	 * @param string $route
	 * @return Yod_Request
	 */
	public function route($route = null)
	{
		$this->_routed = true;
		$this->params = array();

		if (empty($route)) {
			if (strtoupper($this->method) == 'CLI') {
				$route = isset($_SERVER['argv'][1]) ? $_SERVER['argv'][1] : '';
			} elseif(strtoupper($this->method) == 'UNKNOWN') {
				$route = isset($_SERVER['argv'][1]) ? $_SERVER['argv'][1] : (empty($_SERVER['PATH_INFO']) ? '' : $_SERVER['PATH_INFO']);
			} elseif (empty($_GET[YOD_PATHVAR])) {
				$route = empty($_SERVER['PATH_INFO']) ? '' : $_SERVER['PATH_INFO'];
			} else {
				$route = $_GET[YOD_PATHVAR];
			}
		}
		$this->uri = $route = trim(str_replace('//', '/', str_replace('\\', '/', $route)), '/');

		// rules
		($rules = Yod_Application::config('urlrules')) or ($rules = Yod_Application::config('url_rules'));
		if ($rules && is_array($rules)) {
			foreach ($rules as $rule1 => $route1) {
				if (substr($rule1, -2) == '/*') {
					$len1 = strlen($route);
					$pos1 = strlen($rule1) - 2;
					if (strncasecmp($route, $rule1, $pos1) || ($len1 > $pos1 && strncasecmp($route, $rule1, $pos1 + 1))) {
						continue;
					}
					if (is_array($route1)) {
						if (isset($route1[1]) && is_array($route1[1])) {
							$_GET = array_merge($_GET, $route1[1]);
							$_REQUEST = array_merge($_REQUEST, $route1[1]);
							$this->params = array_merge($this->params, $route1[1]);
						}
						$route1 = (isset($route1[0]) && is_string($route1[0])) ? $route1[0] : '';
					}
					if ($route1 == '*') {
						$route1 = substr($route, $pos1 + 1);
					}
					$route = trim(str_replace('//', '/', str_replace('\\', '/', $route1)), '/');
					break;
				}
			}
			$count = substr_count($route, '/');
			foreach ($rules as $rule1 => $route1) {
				if (strncmp(substr($rule1, -2), '/*', 2) && substr_count($rule1, '/') == $count) {
					if ($pos1 = strpos($rule1, ':')) {
						if (strncasecmp($route, $rule1, $pos1)) {
							continue;
						}
						if (($pos2 = strpos($rule1, ':')) && ($pos2 = strpos($rule1, '/', $pos2))) {
							$tail = substr($rule1, $pos2);
							$tail_len = strlen($tail);
							if (strncasecmp(substr($route, -$tail_len), $tail, $tail_len)) {
								continue;
							}
						}
					} elseif (strncasecmp($route, $rule1, strlen($rule1) + 1)) {
						continue;
					}

					if ($pos1) {
						$rule1 = explode('/', substr($rule1, $pos1));
						$temp1 = explode('/', substr($route, $pos1));
						foreach ($rule1 as $key => $param) {
							if ($param[0] == ':') {
								$name = substr($param, 1);
								$_GET[$name] = isset($temp1[$key]) ? $temp1[$key] : null;
								$this->params[$name] = $_GET[$name];
								$_REQUEST[$name] = $_GET[$name];
							}
						}
					}
					
					if (is_array($route1)) {
						if (isset($route1[1]) && is_array($route1[1])) {
							$_GET = array_merge($_GET, $route1[1]);
							$_REQUEST = array_merge($_REQUEST, $route1[1]);
							$this->params = array_merge($this->params, $route1[1]);
						}
						$route1 = (isset($route1[0]) && is_string($route1[0])) ? $route1[0] : '';
					}
					$route = trim(str_replace('//', '/', str_replace('\\', '/', $route1)), '/');
					break;
				}
			}
		}

		$route = explode('/', $route);
		if (isset($_SERVER['SCRIPT_FILENAME'])) {
			$controller = basename($_SERVER['SCRIPT_FILENAME'], '.php');
			$classname = $controller . 'Controller';
			if (class_exists($classname, false)) {
				array_unshift($route, $controller);
			}
		}

		$this->controller = empty($route[0]) ? 'Index' : ucfirst(strtolower($route[0]));
		$this->action = empty($route[1]) ? 'index' : strtolower($route[1]);
		$count = count($route);
		for ($i=2; $i<$count; $i+=2) {
			$_GET[$route[$i]] = isset($route[$i+1]) ? $route[$i+1] : null;
			$this->params[$route[$i]] = $_GET[$route[$i]];
			$_REQUEST[$route[$i]] = $_GET[$route[$i]];
		}

		return $this;
	}

	/**
	 * dispatch
	 * @access public
	 * @param void
	 * @return void
	 */
	public function dispatch()
	{
		if ($this->_dispatched) {
			return;
		}
		$this->_dispatched = true;

		$this->_routed or $this->route();

		$controller = empty($this->controller) ? 'Index' : $this->controller;
		$classname = $controller . 'Controller';
		if (class_exists($classname, false)) {
			new $classname($this);
		} else {
			$classpath = YOD_RUNPATH . '/controllers/' . $controller . 'Controller.php';
			if (is_file($classpath)) {
				require $classpath;
				new $classname($this);
			} else {
				$action = empty($this->action) ? 'Index' : ucfirst($this->action);
				$classpath = YOD_RUNPATH . '/actions/' . strtolower($controller) . '/' . $action . 'Action.php';
				if (is_file($classpath)) {
					require $classpath;
					$classname = $action . 'Action';
					new $classname($this);
				} else {
					$classpath = YOD_RUNPATH . '/controllers/ErrorController.php';
					if (is_file($classpath)) {
						require $classpath;
						new ErrorController($this, 'error');
					} else {
						$this->errorAction();
					}
				}
			}
		}
	}

	/**
	 * errorAction
	 * @access public
	 * @return void
	 */
	public function errorAction()
	{
		$controller = empty($this->controller) ? 'index' : strtolower($this->controller);
		$classpath = YOD_RUNPATH . '/actions/' . $controller . '/ErrorAction.php';
		if (is_file($classpath)) {
			require $classpath;
			new ErrorAction($this);
		} else {
			$this->controller = 'Error';
			$classpath = YOD_RUNPATH . '/actions/ErrorAction.php';
			if (is_file($classpath)) {
				require $classpath;
				new ErrorAction($this);
			} else {
				$this->error404();
			}
		}
	}

	/**
	 * error404
	 * @access public
	 * @param string $html
	 * @return void
	 */
	public function error404($html = null)
	{
		header('HTTP/1.0 404 Not Found');
		if (is_null($html)) {
			if (strtoupper($this->method) == 'CLI') {
				$html = 'HTTP/1.0 404 Not Found';
			} else {
				$html = array(
					'<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML 2.0//EN">',
					'<html><head>',
					'<title>404 Not Found</title>',
					'</head><body>',
					'<h1>Not Found</h1>',
					'<p>The requested URL ' . $_SERVER['PHP_SELF'] . ' was not found on this server.</p>',
					'</body></html>',
				);
				$html = implode(PHP_EOL, $html);
			}
		}
		exit($html);
	}

	/**
	 * isGet
	 * @access public
	 * @param void
	 * @return boolean
	 */
	public function isGet()
	{
		return (strtoupper($this->method) == 'GET');
	}

	/**
	 * isPost
	 * @access public
	 * @param void
	 * @return boolean
	 */
	public function isPost()
	{
		return (strtoupper($this->method) == 'POST');
	}

	/**
	 * isPut
	 * @access public
	 * @param void
	 * @return boolean
	 */
	public function isPut()
	{
		return (strtoupper($this->method) == 'PUT');
	}

	/**
	 * isHead
	 * @access public
	 * @param void
	 * @return boolean
	 */
	public function isHead()
	{
		return (strtoupper($this->method) == 'HEAD');
	}

	/**
	 * isOptions
	 * @access public
	 * @param void
	 * @return boolean
	 */
	public function isOptions()
	{
		return (strtoupper($this->method) == 'OPTIONS');
	}

	/**
	 * isCli
	 * @access public
	 * @param void
	 * @return boolean
	 */
	public function isCli()
	{
		return (strtoupper($this->method) == 'CLI');
	}

	/**
	 * isAjax
	 * @access public
	 * @param void
	 * @return boolean
	 */
	public function isAjax()
	{
		return empty($_SERVER['HTTP_X_REQUESTED_WITH']) ?
			false : (strtolower($_SERVER['HTTP_X_REQUESTED_WITH']) == 'xmlhttprequest');
	}
}

/**
 * Yod_Controller
 * 
 */
abstract class Yod_Controller
{
	protected static $_forward = 0;

	protected $_name;
	protected $_action;
	protected $_request;

	protected $_view = array(
		'tpl_data' => array(),
		'tpl_path' => '',
		'tpl_file' => '',
	);

	/**
	 * __construct
	 * @access public
	 * @param Yod_Request $request
	 * @param string $action
	 * @return void
	 */
	public function __construct($request, $action = null)
	{
		$this->_name = strtolower($request->controller);
		$this->_action = empty($action) ? $request->action : strtolower($action);
		$this->_request = $request;
		$this->_view['tpl_path'] = YOD_RUNPATH . '/views';
		($tpl_data = $this->config('tpldata')) or ($tpl_data = $this->config('tpl_data'));
		if ($tpl_data) {
			$this->_view['tpl_data'] = $tpl_data;
		}

		$this->init();
		$this->run();
	}

	/**
	 * init
	 * @access protected
	 * @return void
	 */
	protected function init()
	{

	}

	/**
	 * run
	 * @access protected
	 * @return void
	 */
	protected function run()
	{
		$this->_action = empty($this->_action) ? 'index' : strtolower($this->_action);
		$method = $this->_action . 'Action';
		if (method_exists($this, $method)) {
			call_user_func(array($this, $method), $this->_request->params);
		} else {
			$cname = empty($this->_name) ? 'index' : strtolower($this->_name);
			$classname = ucfirst($this->_action) . 'Action';
			$classpath = YOD_RUNPATH . '/actions/' . $cname . '/' . $classname . '.php';
			if (is_file($classpath)) {
				require $classpath;
				$action = new $classname($this->_request);
			} elseif (method_exists($this, 'errorAction')) {
				$this->errorAction($this->_request->params);
			} else {
				$this->_request->errorAction();
			}
		}
	}

	/**
	 * assign
	 * @access protected
	 * @param string $name
	 * @param mixed $value
	 * @return Yod_Controller
	 */
	protected function assign($name, $value = null)
	{
		if (is_array($name)) {
			foreach ($name as $key => $value) {
				$this->_view['tpl_data'][$key] = $value;
			}
		} elseif(is_string($name)) {
			$this->_view['tpl_data'][$name] = $value;
		}
		return $this;
	}

	/**
	 * render
	 * @access protected
	 * @param string $view
	 * @param array $data
	 * @return string
	 */
	protected function render($view = null, $data = array())
	{
		// tpl_file
		$view = empty($view) ? $this->_action : $view;
		if ($view) {
			$view = str_replace('..', '', $view);
			$view = str_replace('\\', '/', $view);
			$view = str_replace('//', '/', $view);
		} else {
			$view = 'index';
		}

		if (substr($view, 0, 1) != '/') {
			if (empty($this->_name)) {
				$view = '/' . $view;
			} else {
				$view = '/' . $this->_name . '/' . $view;
			}
		}
		if (empty($this->_view['tpl_path']) || !is_string($this->_view['tpl_path'])) {
			trigger_error('Unavailable property ' . __CLASS__ . '::$_view', E_USER_WARNING);
			return null;
		}
		$this->_view['tpl_file'] = $this->_view['tpl_path'] . strtolower($view) . YOD_VIEWEXT;
		unset($view);

		// tpl_data
		$this->_tpl_data = $data;
		$tpl_path = $this->_view['tpl_path'];
		unset($this->_view['tpl_data']['this'], $this->_tpl_data['this']);
		extract($this->_view['tpl_data']);
		if (is_array($this->_tpl_data)) extract($this->_tpl_data);
		unset($this->_tpl_data);

		// response
		ob_start();
		include $this->_view['tpl_file'];
		$response = ob_get_contents();
		ob_end_clean();

		return $response;
	}

	/**
	 * display
	 * @access protected
	 * @param string $view
	 * @param array $data
	 * @return void
	 */
	protected function display($view = null, $data = array())
	{
		headers_sent() or header('Content-type: text/html; charset=' . YOD_CHARSET);
		echo $this->render($view, $data);
	}

	/**
	 * widget
	 * @access protected
	 * @param string $route
	 * @param array $params
	 * @return void
	 */
	protected function widget($route, $params = array())
	{
		$route = str_replace('\\', '/', $route);
		$route = str_replace('//', '/', $route);
		$route = explode('/', trim($route, '/'));

		$widget = empty($route[0]) ? 'Index' : ucfirst(strtolower($route[0]));
		$action = empty($route[1]) ? 'index' : strtolower($route[1]);
		$params1 = array();
		$count = count($route);
		for ($i=2; $i<$count; $i+=2) {
			$params1[$route[$i]] = empty($route[$i+1]) ? '' : $route[$i+1];
		}
		if (is_array($params1)) {
			$params1 = array_merge($params1, $params);
		}

		$classname = $widget . 'Widget';
		if (class_exists($classname, false)) {
			new $classname($this->_request, $action, $params1);
		} else {
			$classpath = YOD_RUNPATH . '/widgets/' . $widget . 'Widget.php';
			if (is_file($classpath)) {
				require $classpath;
				new $classname($this->_request, $action, $params);
			} else {
				trigger_error("Widget '{$widget}Widget' not found", E_USER_WARNING);
			}
		}

	}

	/**
	 * forward
	 * @access protected
	 * @param string $route
	 * @return void
	 */
	protected function forward($route, $exited = false)
	{
		if (self::$_forward++ > YOD_FORWARD) return;

		if (strpos($route,'/') === false) {
			$this->_action = strtolower($route);
			$this->run();
		} else {
			$request = new Yod_Request($route);
			$request->dispatch();
		}

		if ($exited) exit;
	}

	/**
	 * redirect
	 * @access protected
	 * @param string $url
	 * @param integer $code
	 * @return void
	 */
	protected function redirect($url, $code = 302)
	{
		headers_sent() or header('Location:' . $url, true, $code);
	}

	/**
	 * error404
	 * @access protected
	 * @param mixed $html
	 * @param boolean $exit
	 * @return void
	 */
	protected function error404($html = null)
	{
		$this->_request->error404($html);
	}

	/**
	 * __destruct
	 * 
	 * @return void
	 */
	public function __destruct() {

	}
}

/**
 * Yod_Action
 * 
 */
abstract class Yod_Action extends Yod_Controller
{
	/**
	 * run
	 * @access protected
	 * @return void
	 */
	protected function run()
	{

	}
}

/**
 * Yod_Widget
 * 
 */
abstract class Yod_Widget extends Yod_Controller
{
	/**
	 * __construct
	 * @access public
	 * @param Yod_Request $request
	 * @param string $action
	 * @param array $params
	 * @return void
	 */
	public function __construct($request, $action = null, $params = null)
	{
		$name = get_class($this);
		$this->_name = strtolower(substr($name, 0, -6));
		$this->_action = empty($action) ? 'index' : strtolower($action);
		$this->_request = $request;
		$this->_view['tpl_path'] = YOD_RUNPATH . '/widgets';
		($tpl_data = $this->config('tpldata')) or ($tpl_data = $this->config('tpl_data'));
		if ($tpl_data) {
			$this->_view['tpl_data'] = $tpl_data;
		}

		$this->init();

		$this->_action = empty($this->_action) ? 'index' : strtolower($this->_action);
		$method = $this->_action . 'Action';
		if (method_exists($this, $method)) {
			call_user_func(array($this, $method), $params);
		} else {
			trigger_error('Unavailable action ' . $name . '::' . $method . '()', E_USER_WARNING);
		}
	}

	/*
	 * run
	 * @access protected
	 * @return void
	 */
	protected function run()
	{

	}
}

/**
 * Yod_Model
 * 
 */
class Yod_Model
{
	protected static $_model = array();

	protected $_db;
	protected $_dsn = 'db_dsn';

	protected $_name;
	protected $_table;
	protected $_prefix;


	/**
	 * __construct
	 * @access public
	 * @param string $name
	 * @param mixed $config
	 * @return void
	 */
	public function __construct($name = '', $config = null)
	{
		if (empty($name)) {
			if (empty($this->_name)) {
				if (substr(get_class($this), 0, 4) == 'Yod_') {
					$this->_name = '';
				} else {
					$this->_name = substr(get_class($this), 0, -5);
				}
			}
		} else {
			$this->_name = $name;
		}
		if (empty($this->_table)) {
			$this->_table = strtolower(trim(preg_replace('/[A-Z]/', '_\\0', $this->_name), '_'));
		}

		if (empty($config) && $this->_dsn) {
			$config = $this->config($this->_dsn);
		}
		if ($this->_db = Yod_Database::getInstance($config)) {
			$this->_prefix = $this->_db->config('prefix');
		}

		$this->init();
	}

	/**
	 * init
	 * @access protected
	 * @param void
	 * @return void
	 */
	protected function init()
	{

	}

	/**
	 * getInstance
	 * @access public
	 * @param string	$name
	 * @param mixed		$config
	 * @return Yod_Model
	 */
	public static function getInstance($name = '', $config = null)
	{
		if (empty($name)) {
			$classname = 'Yod_Model';
		} else {
			$name = ucfirst($name);
			$classname = $name . 'Model';
		}
		if (empty(self::$_model[$name])) {
			$classpath = YOD_RUNPATH . '/models/' . $classname . '.php';
			if (is_file($classpath)) {
				include $classpath;
				self::$_model[$name] = new $classname($name, $config);
			} else {
				if (class_exists($classname, false)) {
					self::$_model[$name] = new $classname($name, $config);
				} else {
					self::$_model[$name] = new self($name, $config);
				}
			}
		}

		return self::$_model[$name];
	}

	/**
	 * find
	 * @access public
	 * @param mixed		$where
	 * @param array		$params
	 * @param mixed		$select
	 * @return mixed
	 */
	public function find($where = null, $params = array(), $select = '*')
	{
		if (is_numeric($where)) {
			$where = 'id = '. $where;
		}
		if ($result = $this->_db->select($select, $this->_table, $where, $params, 'LIMIT 1')) {
			$data = $this->_db->fetch($result);
			$this->_db->free($result);
			return $data;
		}
		return false;
	}

	/**
	 * select
	 * @access public
	 * @param mixed		$where
	 * @param array		$params
	 * @param mixed		$select
	 * @return mixed
	 */
	public function select($where = null, $params = array(), $select = '*')
	{
		if (is_numeric($where)) {
			$where = 'id = '. $where;
		}
		if ($result = $this->_db->select($select, $this->_table, $where, $params)) {
			$data = $this->_db->fetchAll($result);
			$this->_db->free($result);
			return $data;
		}
		return false;
	}

	/**
	 * count
	 * @access public
	 * @param string	$where
	 * @param array		$params
	 * @return integer
	 */
	public function count($where = '', $params = array())
	{
		$count = 0;
		if ($result = $this->_db->select('COUNT(*)', $this->_table, $where, $params, "LIMIT 1")) {
			if ($data = $this->_db->fetch($result)) {
				$count = current($data);
			}
			$this->_db->free($result);
		}
		return $count;
	}

	/**
	 * save
	 * @access public
	 * @param array		$data
	 * @param string	$where
	 * @param array		$params
	 * @return integer
	 */
	public function save($data, $where = '', $params = array())
	{
		if (empty($where)) {
			if ($this->_db->insert($data, $this->_table)) {
				return $this->_db->insertId();
			}
			return false;
		} else {
			return $this->_db->update($data, $this->_table, $where, $params);
		}
	}

	/**
	 * update
	 * @access public
	 * @param array		$data
	 * @param string	$where
	 * @param array		$params
	 * @return integer
	 */
	public function update($data, $where = '', $params = array())
	{
		if (empty($data)) return false;
		foreach ($data as $name => $value) {
			if (is_string($name) && $name[0] == ':') {
				if (empty($where)) {
					$where = substr($name, 1) . ' = ' . $name;
				} else {
					$where .= ' AND ' . substr($name, 1) . ' = ' . $name;
				}
				$params[$name] = $value;
				unset($data[$name]);
			}
		}
		return $this->_db->update($data, $this->_table, $where, $params);
	}

	/**
	 * remove
	 * @access public
	 * @param string	$where
	 * @param array		$params
	 * @return integer
	 */
	public function remove($where, $params = array())
	{
		return $this->_db->delete($this->_table, $where, $params);
	}

	/**
	 * lastQuery
	 * @access public
	 * @return string
	 */
	public function lastQuery()
	{
		return $this->_db->lastQuery();
	}
	
}

/**
 * Yod_DbModel
 * 
 */
class Yod_DbModel extends Yod_Model
{
	protected static $_model = array();

	protected $_query = array();
	protected $_params = array();

	/**
	 * __construct
	 * @access public
	 * @param string $name
	 * @param mixed $config
	 * @return void
	 */
	public function __construct($name = '', $config = null)
	{
		parent::__construct($name, $config);

		$this->initQuery();
	}

	/**
	 * getInstance
	 * @access public
	 * @param string	$name
	 * @param mixed		$config
	 * @return Yod_DbModel
	 */
	public static function getInstance($name = '', $config = null)
	{
		$name = ucfirst($name);
		if (empty(self::$_model[$name])) {
			self::$_model[$name] = new self($name, $config);
		}
		return self::$_model[$name];
	}

	/**
	 * table
	 * @access public
	 * @return Yod_DbModel
	 */
	public function table($table)
	{
		if ($table) {
			$this->_table = $table;
		}
		return $this;
	}

	/**
	 * find
	 * @access public
	 * @param mixed		$where
	 * @param array		$params
	 * @param mixed		$select
	 * @return mixed
	 */
	public function find($where = null, $params = array(), $select = null)
	{
		if (is_numeric($where)) {
			$where = 'id = '. $where;
		}
		$query = $this->field($select)->where($where, $params)->limit('1')->parseQuery();
		if ($result = $this->_db->query($query, $this->_params)) {
			$data = $this->_db->fetch($result);
			$this->_db->free($result);
			$this->initQuery();
			return $data;
		}

		return false;
	}

	/**
	 * select
	 * @access public
	 * @param mixed		$where
	 * @param array		$params
	 * @param mixed		$select
	 * @return mixed
	 */
	public function select($where = null, $params = array(), $select = null)
	{
		if (is_numeric($where)) {
			$where = 'id = '. $where;
		}
		$query = $this->field($select)->where($where, $params)->parseQuery();
		if ($result = $this->_db->query($query, $this->_params)) {
			$data = $this->_db->fetchAll($result);
			$this->_db->free($result);
			$this->initQuery();
			return $data;
		}

		return false;
	}

	/**
	 * count
	 * @access public
	 * @param string	$where
	 * @param array		$params
	 * @return integer
	 */
	public function count($where = '', $params = array())
	{
		$query = $this->field('COUNT(*)')->where($where, $params)->parseQuery();
		if ($result = $this->_db->query($query, $this->_params)) {
			$count = 0;
			if ($data = $this->_db->fetch($result)) {
				$count = current($data);
			}
			$this->_db->free($result);
			$this->initQuery();
			return $count;
		}

		return 0;
	}

	/**
	 * save
	 * @access public
	 * @param array		$data
	 * @param string	$where
	 * @param array		$params
	 * @return integer
	 */
	public function save($data, $where = '', $params = array())
	{
		if (empty($this->_table)) {
			trigger_error('You have an error in your SQL syntax: table name is empty', E_USER_WARNING);
			return false;
		}
		$this->where($where, $params);
		if (empty($this->_query['WHERE'])) {
			if ($this->_db->insert($data, $this->_table)) {
				$result = $this->_db->insertId();
			} else {
				$result = false;
			}
		} else {
			$result = $this->_db->update($data, $this->_table, $this->_query['WHERE'], $this->_params);
		}
		$this->initQuery();
		return $result;
	}

	/**
	 * update
	 * @access public
	 * @param array		$data
	 * @param string	$where
	 * @param array		$params
	 * @return integer
	 */
	public function update($data, $where = '', $params = array())
	{
		if (empty($this->_table)) {
			trigger_error('You have an error in your SQL syntax: table name is empty', E_USER_WARNING);
			return false;
		}
		if (empty($data)) return false;
		foreach ($data as $name => $value) {
			if (is_string($name) && $name[0] == ':') {
				if (empty($where)) {
					$where = substr($name, 1) . ' = ' . $name;
				} else {
					$where .= ' AND ' . substr($name, 1) . ' = ' . $name;
				}
				$params[$name] = $value;
				unset($data[$name]);
			}
		}
		$this->where($where, $params);
		$result = $this->_db->update($data, $this->_table, $this->_query['WHERE'], $this->_params);
		$this->initQuery();
		return $result;
	}

	/**
	 * remove
	 * @access public
	 * @param string	$where
	 * @param array		$params
	 * @return integer
	 */
	public function remove($where, $params = array())
	{
		if (empty($this->_table)) {
			trigger_error('You have an error in your SQL syntax: table name is empty', E_USER_WARNING);
			return false;
		}
		$this->where($where, $params);
		$result = $this->_db->delete($this->_table, $this->_query['WHERE'], $this->_params);
		$this->initQuery();
		return $result;
	}

	/**
	 * field
	 * @access public
	 * @param mixed		$select
	 * @return Yod_DbModel
	 */
	public function field($select)
	{
		if (is_array($select)) {
			foreach ($select as $key => $value) {
				if (is_string($key)) {
					$select[$key] = "{$value} AS {$key}"; 
				}
			}
			$select = implode(', ', $select);
		}
		if ($select) {
			$this->_query['SELECT'] = $select;
		}
		return $this;
	}

	/**
	 * from
	 * @access public
	 * @param string	$table
	 * @return Yod_DbModel
	 */
	public function from($table)
	{
		if ($table) {
			$this->_query['FROM'] = "{$this->_prefix}{$table} AS t1";
		}
		return $this;
	}

	/**
	 * join
	 * @access public
	 * @param string	$table
	 * @param string	$where
	 * @param string	$mode
	 * @return Yod_DbModel
	 */
	public function join($table, $where = '', $mode = 'LEFT')
	{
		$join = count($this->_query['JOIN']) + 2;
		$this->_query['JOIN'][] = "{$mode} JOIN {$this->_prefix}{$table} AS t{$join}". (empty($where) ? '' : " ON {$where}");
		return $this;
	}

	/**
	 * where
	 * @access public
	 * @param string	$where
	 * @param array		$params
	 * @param string	$mode
	 * @return Yod_DbModel
	 */
	public function where($where, $params = array(), $mode = 'AND')
	{
		if ($where) {
			if (is_string($params) && $params) {
				$mode = $params;
			} else {
				$this->params($params);
			}
			if ($this->_query['WHERE']) {
				$where = "({$this->_query['WHERE']}) {$mode} ({$where})";
			}
			$this->_query['WHERE'] = $where;
		}
		return $this;
	}

	/**
	 * group
	 * @access public
	 * @param string	$group
	 * @return Yod_DbModel
	 */
	public function group($group)
	{
		$this->_query['GROUP BY'] = $group;
		return $this;
	}

	/**
	 * having
	 * @access public
	 * @param string	$having
	 * @param array		$params
	 * @return Yod_DbModel
	 */
	public function having($having, $params = array())
	{
		$this->_query['HAVING'] = $having;
		$this->params($params);
		return $this;
	}

	/**
	 * order
	 * @access public
	 * @param string	$order
	 * @return Yod_DbModel
	 */
	public function order($order)
	{
		$this->_query['ORDER BY'] = $order;
		return $this;
	}

	/**
	 * limit
	 * @access public
	 * @param mixed		$limit
	 * @return Yod_DbModel
	 */
	public function limit($limit)
	{
		$this->_query['LIMIT'] = $limit;
		return $this;
	}

	/**
	 * union
	 * @access public
	 * @param mixed		$union
	 * @param array		$params
	 * @param string	$mode
	 * @return Yod_DbModel
	 */
	public function union($union, $params = array(), $mode = '')
	{
		if (is_array($union)) {
			$union = $this->parseQuery($union);
		}
		$this->_query['UNION'][] = 'UNION ' . (empty($mode) ? '' : $mode . ' ') . "({$union})";
		$this->params($params);
		return $this;
	}

	/**
	 * comment
	 * @access public
	 * @param string	$comment
	 * @return Yod_DbModel
	 */
	public function comment($comment)
	{
		$this->_query['COMMENT'] = $comment;
		return $this;
	}

	/**
	 * params
	 * @access public
	 * @param array		$params
	 * @return Yod_DbModel
	 */
	public function params($params)
	{
		if (is_array($params)) {
			if (is_array($this->_params)) {
				$params = array_merge($this->_params, $params);
			}
			$this->_params = $params;
		}
		return $this;
	}

	/**
	 * parseQuery
	 * @access public
	 * @param array		$query
	 * @return mixed
	 */
	public function parseQuery($query = null)
	{
		$query = empty($query) ? $this->_query : $query;
		if (empty($query['FROM'])) {
			if (empty($this->_table)) {
				trigger_error('You have an error in your SQL syntax: table name is empty', E_USER_WARNING);
				return false;
			}
			$query['FROM'] = "{$this->_prefix}{$this->_table} AS t1";
		}

		$result = '';
		foreach ($query as $key => $value) {
			if (empty($value)) {
				continue;
			}
			if (is_array($value)) {
				if ($key == 'UNION') {
					$result = '(' . $result . ') ' . implode(' ', $value);
				} elseif ($key == 'JOIN') {
					$result .= ' ' . implode(' ', $value);
				}
				continue;
			}
			if ($key == 'SELECT') {
				$result = $key . ' ' . $value;
			} elseif ($key == 'COMMENT') {
				$result .= ' /* ' . $value . ' */';
			} else {
				$result .= ' ' . $key . ' ' . $value;
			}
		}

		return $result;
	}

	/**
	 * initQuery
	 * @access protected
	 * @return string
	 */
	protected function initQuery()
	{
		$this->_query = array(
			'SELECT' => '*',
			'FROM' => '',
			'JOIN' => array(),
			'WHERE' => '',
			'GROUP BY' => '',
			'HAVING' => '',
			'ORDER BY' => '',
			'LIMIT' => '',
			'UNION' => array(),
			'COMMENT' => '',
		);
		$this->_params = array();

		return $this;
	}

}

/**
 * Yod_Database
 * 
 */
abstract class Yod_Database
{
	protected static $_db = array();

	protected $_config;
	protected $_driver;
	protected $_prefix;
	protected $_result;
	protected $_linkid;
	protected $_linkids = array();
	protected $_locked = false;
	protected $_lastquery = '';


	/**
	 * __construct
	 * @access public
	 * @param array $config
	 * @return void
	 */
	public function __construct($config)
	{
		$this->_config = $config;
		$this->_prefix = empty($config['prefix']) ? '' : $config['prefix']; 
	}

	/**
	 * __construct
	 * @access public
	 * @param void
	 * @return void
	 */
	public function __destruct()
	{
		$this->close();
	}

	/**
	 * db
	 * @access public
	 * @param mixed $config
	 * @return Yod_Database
	 */
	public static function db($config = 'db_dsn')
	{
		return self::getInstance($config);
	}

	/**
	 * getInstance
	 * @access public
	 * @param mixed $config
	 * @return Yod_Database
	 */
	public static function getInstance($config = 'db_dsn')
	{
		if (is_string($config)) {
			$config = Yod_Application::config($config);
		}

		if (!is_array($config)) {
			return false;
		}

		if (empty($config['type']) || !is_string($config['type'])) {
			$config['type'] = 'pdo';
		}

		$md5key = md5(serialize($config));
		if (empty(self::$_db[$md5key])) {
			if ($config['type'] == 'pdo') {
				$classname = 'Yod_DbPdo';
			} else {
				$classname = 'Yod_Db'.ucwords($config['type']);
			}
			if (!class_exists($classname, false)) {
				include YOD_EXTPATH . '/drivers/' . substr($classname, 4) . '.class.php';
			}
			self::$_db[$md5key] = new $classname($config);
		}

		return self::$_db[$md5key];
	}

	/**
	 * config
	 * @access public
	 * @param string $name
	 * @param mixed $value
	 * @return array
	 */
	public function config($name = '', $value = null)
	{
		$argc = func_num_args();
		switch ($argc) {
			case 0:
				return $this->_config;
				break;
			case 1:
				return isset($this->_config[$name]) ? $this->_config[$name] : null;
				break;
			case 2:
				if (is_null($value)) {
					unset($this->_config[$name]);
				} else {
					$this->_config[$name] = $value;
				}
				break;
			default :
				if ($name) {
					$argv = func_get_args();
					array_shift($argv);
					$this->_config[$name] = $argv;
				}
		}
	}

	/**
	 * create
	 * @access public
	 * @param mixed $fields
	 * @param string $table
	 * @param string $extend
	 * @return mixed
	 */
	public function create($fields, $table, $extend = '')
	{
		if (empty($fields) || empty($table)) return false;
		foreach ($fields as $key => $value) {
			if (is_string($key)) {
				$fields[$key] = $key . ' ' . $value;
			} else {
				$fields[$key] = $value;
			}
		}
		$query = 'CREATE TABLE ' . $this->_prefix . $table . ' (' . implode(', ', $fields) . ')' . $extend;
		return $this->execute($query);
	}

	/**
	 * insert
	 * @access public
	 * @param array $data
	 * @param string $table
	 * @param boolean $replace
	 * @return mixed
	 */
	public function insert($data, $table, $replace = false)
	{
		if (empty($data) || empty($table)) return false;
		$values = $fields = $params = array();
		foreach ($data as $key => $value) {
			if(is_scalar($value) || is_null($value)) {
				$name = ':'. md5($key);
				$fields[] =  $key;
				$values[] = $name;
				$params[$name] = $value;
			}
		}
		$query = ($replace ? 'REPLACE' : 'INSERT') . ' INTO ' . $this->_prefix . $table . ' (' . implode(', ', $fields) . ') VALUES (' . implode(',', $values) . ')';
		return $this->execute($query, $params, true);
	}

	/**
	 * update
	 * @access public
	 * @param array $data
	 * @param string $table
	 * @param string $where
	 * @param array $params
	 * @return integer
	 */
	public function update($data, $table, $where = null, $params = array())
	{
		if (empty($data) || empty($table)) return false;
		$params1 = $update = array();
		foreach ($data as $key => $value) {
			if (is_scalar($value) || is_null($value)) {
				if (is_numeric($key)) {
					$update[] = $value;
				} else {
					$name = ':'. md5($key);
					$update[] = $key.' = '.$name;
					$params1[$name] = $value;
				}
			}
		}
		$params1 = array_merge($params1, $params);
		$query = 'UPDATE ' . $this->_prefix . $table . ' SET ' .implode(', ', $update) . (empty($where) ? '' : ' WHERE ' . $where);
		return $this->execute($query, $params1, true);
	}

	/**
	 * delete
	 * @access public
	 * @param string $table
	 * @param string $where
	 * @param array $params
	 * @return integer
	 */
	public function delete($table, $where = null, $params = array())
	{
		if (empty($table)) return false;
		$query = 'DELETE FROM ' . $this->_prefix . $table . (empty($where) ? '' : ' WHERE ' . $where);
		return $this->execute($query, $params, true);
	}

	/**
	 * select
	 * @access public
	 * @param string $select
	 * @param string $table
	 * @param string $where
	 * @param array $params
	 * @param string $extend
	 * @return mixed
	 */
	public function select($select, $table, $where = null, $params = array(), $extend = null)
	{
		if (empty($table))  return false;
		if (is_array($select)) {
			foreach ($select as $key => $value) {
				if (is_string($key)) {
					$select[$key] = "{$key} AS {$value}"; 
				}
			}
			$select = implode(', ', $select);
		}
		$query = 'SELECT ' . (empty($select) ? '*' : $select) . ' FROM ' . $this->_prefix . $table . (empty($where) ? '' : ' WHERE ' . $where) . (empty($extend) ? '' : ' ') . $extend;
		return $this->query($query, $params);
	}

	/**
	 * lastQuery
	 * @access public
	 * @param void
	 * @return string
	 */
	public function lastQuery()
	{
		return $this->_lastquery;
	}

	/**
	 * dbconfig
	 * @access protected
	 * @param array $config
	 * @param integer $linknum
	 * @return array
	 */
	protected function dbconfig($config, $linknum = 0)
	{
		if (empty($config)) {
			$config = $this->_config;
		}
		if ($this->_locked) {
			$linknum = 0;
		}
		if ($linknum == 1) {
			if (empty($this->_config['slaves'])) {
				$linknum = 0;
			} elseif(is_array($this->_config['slaves'])) {
				if (isset($this->_config['slaves']['dsn'])) {
					$slaves = $this->_config['slaves'];
				} else {
					$k_rand = array_rand($this->_config['slaves'], 1);
					$slaves = $this->_config['slaves'][$k_rand];
				}
				$config = array_merge($config, $slaves);
			}
		}
		if (is_array($config)) {
			$config['linknum'] = $linknum;
		}
		return $config;
	}

	/**
	 * connect
	 * @access public
	 * @param array $config
	 * @param integer $linknum
	 * @return mixed
	 */
	abstract public function connect($config = null, $linknum = 0);

	/**
	* fields
	* @access public
	* @param string $table
	* @return array
	*/
	abstract public function fields($table);

	/**
	 * execute
	 * @access public
	 * @param string $query
	 * @param array $params
	 * @param boolean $affected
	 * @return mixed
	 */
	abstract public function execute($query, $params = array(), $affected = false);

	/**
	 * query
	 * @access public
	 * @param string $query
	 * @param array $params
	 * @return mixed
	 */
	abstract public function query($query, $params = array());
	
	/**
	 * count
	 * @access public
	 * @param mixed $result
	 * @return mixed
	 */
	abstract public function count($result = null);

	/**
	 * fetch
	 * @access public
	 * @param mixed $result
	 * @return mixed
	 */
	abstract public function fetch($result = null);

	/**
	 * fetchAll
	 * @access public
	 * @param mixed $result
	 * @return mixed
	 */
	abstract public function fetchAll($result = null);

	/**
	 * transaction
	 * @access public
	 * @param void
	 * @return boolean 
	 */
	abstract public function transaction();

	/**
	 * commit
	 * @access public
	 * @param void
	 * @return boolean 
	 */
	abstract public function commit();

	/**
	 * rollback
	 * @access public
	 * @param void
	 * @return boolean 
	 */
	abstract public function rollback();

	/**
	 * insertId
	 * @access public
	 * @param void
	 * @return integer
	 */
	abstract public function insertId();

	/**
	 * quote
	 * @access public
	 * @param string $string
	 * @return string
	 */
	abstract public function quote($string);

	/**
	 * free
	 * @access public
	 * @param mixed $result
	 * @return void
	 */
	abstract public function free($result = null);

	/**
	 * close
	 * @access public
	 * @param void
	 * @return void
	 */
	abstract public function close();

}

/**
 * Yod_DbPdo
 * 
 */
class Yod_DbPdo extends Yod_Database
{
	/**
	 * __construct
	 * @access public
	 * @param array $config
	 * @return void
	 */
	public function __construct($config)
	{
		parent::__construct($config);

		$this->_driver = __CLASS__;
		
	}

	/**
	 * connect
	 * @access public
	 * @param array $config
	 * @param integer $linknum
	 * @return mixed
	 */
	public function connect($config = null, $linknum = 0)
	{
		$config = $this->dbconfig($config, $linknum);
		$linknum = isset($config['linknum']) ? $config['linknum'] : 0;
		if (isset($this->_linkids[$linknum])) {
			return $this->_linkid = $this->_linkids[$linknum];
		}
		if (empty($config['dsn']) && empty($config['pdsn'])) {
			trigger_error('PDO DSN configure error', E_USER_ERROR);
			return false;
		}
		$config['pdsn'] = empty($config['pdsn']) ? $config['dsn'] : $config['pdsn'];
		$config['user'] = empty($config['user']) ? null : $config['user'];
		$config['pass'] = empty($config['pass']) ? null : $config['pass'];
		$config['charset'] = empty($config['charset']) ? 'utf8' : $config['charset'];
		$config['options'] = empty($config['options']) ? array() : $config['options'];
		if (isset($config['pconnect']) && $config['pconnect'] == true) {
			$config['options'][PDO::ATTR_PERSISTENT] = true;
		}
		try {
			$this->_linkids[$linknum] = new PDO($config['pdsn'], $config['user'], $config['pass'], $config['options']);
		} catch (PDOException $e) {
			trigger_error($e->getMessage(), E_USER_ERROR);
			return false;
		}

		$this->_linkids[$linknum]->exec('SET NAMES '. $config['charset']);
		$this->_linkids[$linknum]->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_WARNING);
		return $this->_linkid = $this->_linkids[$linknum];
	}

	/**
	 * fields
	 * @access public
	 * @param string $table
	 * @return array
	 */
	public function fields($table)
	{
		$fields = array();
		if ($result = $this->query('SHOW COLUMNS FROM ' . $this->_prefix . $table)) {
			if ($data = $this->fetchAll($result)) {
				foreach ($data as $key => $value) {
					$fields[$value['Field']] = array(
						'name'    => $value['Field'],
						'type'    => $value['Type'],
						'notnull' => (bool) ($value['Null'] === ''), // not null is empty, null is yes
						'default' => $value['Default'],
						'primary' => (strtolower($value['Key']) == 'pri'),
						'autoinc' => (strtolower($value['Extra']) == 'auto_increment'),
					);
				}	
			}

		}
		return $fields;
	}

	/**
	 * execute
	 * @access public
	 * @param string $query
	 * @param array $params
	 * @param boolean $affected
	 * @return boolean
	 */
	public function execute($query, $params = array(), $affected = false)
	{
		$this->connect($this->_config, 0);

		$this->_lastquery = $query;
		if (empty($params) || is_bool($params)) {
			$retval = $this->_linkid->exec($query);
			if ($retval !== false) {
				if (is_bool($params)) {
					$affected = $params;
				}
				return $affected ? $retval : true;
			}
			return false;
		}
		if ($this->_result = $this->_linkid->prepare($query)) {
			$bind_params = array();
			foreach ($params as $key => $value) {
				if (strstr($query, $key)) {
					$bind_params[$key] = $params[$key];
				}
			}
			if ($this->_result->execute($bind_params)) {
				return $affected ? $this->_result->rowCount() : true;
			}
		}
		return false;
	}

	/**
	 * query
	 * @access public
	 * @param string $query
	 * @param array $params
	 * @return mixed
	 */
	public function query($query, $params = array())
	{
		$this->connect($this->_config, 1);

		$this->_lastquery = $query;
		if (empty($params)) {
			return $this->_result = $this->_linkid->query($query);
		} else {
			if ($this->_result = $this->_linkid->prepare($query)) {
				$bind_params = array();
				foreach ($params as $key => $value) {
					if (strstr($query, $key)) {
						$bind_params[$key] = $value;
					}
				}
				$this->_result->execute($bind_params);
				return $this->_result;
			}
		}

		return false;
	}

	/**
	 * count
	 * @access public
	 * @param mixed $result
	 * @return mixed
	 */
	public function count($result = null)
	{
		if (is_null($result)) {
			$result = $this->_result;
		}
		if (is_object($result)) {
			return $result->rowCount();
		}
		return false;
	}

	/**
	 * fetch
	 * @access public
	 * @param mixed $result
	 * @return mixed
	 */
	public function fetch($result = null)
	{
		if (is_null($result)) {
			$result = $this->_result;
		}
		if (is_object($result)) {
			return $result->fetch(PDO::FETCH_ASSOC);
		}
		return false;
	}

	/**
	 * fetchAll
	 * @access public
	 * @param mixed $result
	 * @return mixed
	 */
	public function fetchAll($result = null)
	{
		if (is_null($result)) {
			$result = $this->_result;
		}
		if (is_object($result)) {
			return $result->fetchAll(PDO::FETCH_ASSOC);
		}
		return array();
	}

	/**
	 * transaction
	 * @access public
	 * @param void
	 * @return boolean
	 */
	public function transaction()
	{
		$this->_locked = true;
		$this->connect($this->_config, 0);
		return $this->_linkid->beginTransaction();
	}

	/**
	 * commit
	 * @access public
	 * @param void
	 * @return boolean
	 */
	public function commit()
	{
		$this->_locked = false;
		$this->connect($this->_config, 0);
		return $this->_linkid->commit();
	}

	/**
	 * rollback
	 * @access public
	 * @param void
	 * @return boolean
	 */
	public function rollback()
	{
		$this->_locked = false;
		$this->connect($this->_config, 0);
		return $this->_linkid->rollBack();
	}

	/**
	 * insertId
	 * @access public
	 * @param void
	 * @return integer
	 */
	public function insertId()
	{
		return $this->_linkid->lastInsertId();
	}

	/**
	 * quote
	 * @access public
	 * @param string $result
	 * @return string
	 */
	public function quote($string)
	{
		return $this->_linkid->quote($string);
	}

	/**
	 * free
	 * @access public
	 * @param mixed $result
	 * @return void
	 */
	public function free($result = null)
	{
		if (is_null($result)) {
			$this->_result = null;
		} else {
			$result = null;
		}
	}

	/**
	 * close
	 * @access public
	 * @param void
	 * @return void
	 */
	public function close()
	{
		foreach ($this->_linkids as $key => $linkid) {
			if ($linkid) {
				unset($this->_linkids[$key]);
			}
		}
		if ($this->_linkid) {
			$this->_linkid = null;
		}
	}

	/**
	 * errno
	 * @access public
	 * @param void
	 * @return mixed
	 */
	public function errno()
	{
		if ($this->_result) {
			return $this->_result->errorCode();
		} elseif ($this->_linkid) {
			return $this->_linkid->errorCode();
		} else {
			return false;
		}
	}

	/**
	 * error
	 * @access public
	 * @param void
	 * @return mixed
	 */
	public function error()
	{
		if ($this->_result) {
			if ($error = $this->_result->errorInfo()) {
				return $error[2];
			}
		} elseif ($this->_linkid) {
			if ($error = $this->_linkid->errorInfo()) {
				return $error[2];
			}
		} else {
			return false;
		}
	}
}

/**
 * Yod_Plugin
 * 
 */
abstract class Yod_Plugin
{

}

/**
 * Yod
 *
 */
abstract class Yod
{
	
	/**
	 * app
	 * @access public
	 * @param mixed $config
	 * @return Yod_Application
	 */
	public static function app($config = null)
	{
		return Yod_Application::app($config);
	}

	/**
	 * config
	 * @access public
	 * @param string $name
	 * @return array
	 */
	public static function config($name = null)
	{
		return Yod_Application::config($name);
	}

	/**
	 * import
	 * @access public
	 * @param string $alias
	 * @param string $classext
	 * @return boolean
	 */
	public static function import($alias, $classext = '.class.php')
	{
		return Yod_Application::import($alias, $classext);
	}

	/**
	 * plugin
	 * @access public
	 * @param string $alias
	 * @param string $classext
	 * @return mixed
	 */
	public static function plugin($alias, $classext = '.class.php')
	{
		return Yod_Application::plugin($alias, $classext);
	}

	/**
	 * model
	 * @access public
	 * @param string $name
	 * @param mixed $config
	 * @return Yod_Model
	 */
	public static function model($name = '', $config = '')
	{
		return Yod_Model::getInstance($name, $config);
	}

	/**
	 * dbmodel
	 * @access public
	 * @param string $name
	 * @param mixed $config
	 * @return Yod_DbModel
	 */
	public static function dbmodel($name = '', $config = '')
	{
		return Yod_DbModel::getInstance($name, $config);
	}

	/**
	 * db
	 * @access public
	 * @param mixed $config
	 * @return Yod_Database
	 */
	public static function db($config = 'db_dsn')
	{
		return Yod_Database::getInstance($config);
	}

}
