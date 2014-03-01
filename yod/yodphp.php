<?php
// +----------------------------------------------------------------------
// | yodphp [ Yod PHP Framework ]
// +----------------------------------------------------------------------
// | Copyright (c) 2013 http://yodphp.com All rights reserved.
// +----------------------------------------------------------------------
// | Licensed ( http://www.apache.org/licenses/LICENSE-2.0 )
// +----------------------------------------------------------------------
// | Author: Baoqiang Su <zmrnet@qq.com>
// +----------------------------------------------------------------------

// yodphp constant
defined('YOD_RUNTIME') or define('YOD_RUNTIME', microtime(true));
defined('YOD_VERSION') or define('YOD_VERSION', '1.2.0');
defined('YOD_FORWARD') or define('YOD_FORWARD', 5);
defined('YOD_CHARSET') or define('YOD_CHARSET', 'utf-8');
defined('YOD_PATHVAR') or define('YOD_PATHVAR', '');
defined('YOD_EXTPATH') or define('YOD_EXTPATH', dirname(__FILE__));

// yodphp register
register_shutdown_function(array('Yod_Application', 'autorun'));

/**
 * Yod_Application
 * 
 */
final class Yod_Application
{
	protected static $_app;

	protected $_config = array();
	protected $_request = null;
	protected $_imports = array();
	protected $_running = false;

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

		// autoload
		spl_autoload_register(array('Yod_Application', 'autoload'));

		// runmode
		if (defined('YOD_RUNMODE') && (YOD_RUNMODE & 1) == 0) {
			error_reporting(0);
		}

		// config
		if (is_array($config)) {
			$this->_config = $config;
		} else {
			if (!is_string($config)) {
				$config = YOD_RUNPATH . '/configs/config.php';
			}
			if (is_file($config)) {
				$this->_config = include($config);
			} else {
				$this->_config = array();
				$scandir = dirname($config);
				if (is_dir($scandir) && ($handle = opendir($scandir))) {
					while (($file = readdir($handle)) != false) {
						if (substr($file, -11) == '.config.php') {
							$key = substr($file, 0, -11);
							$value = include($scandir .'/'. $file);
							if (is_array($value)) {
								if ($key == 'base') {
									$this->_config = array_merge($this->_config, $value);
								} elseif (isset($this->_config[$key])){
									if (is_array($this->_config[$key])) {
										$value = array_merge($this->_config[$key], $value);
									}
								}
								$this->_config[$key] = $value;
							}
						}
					}
					closedir($handle);
				}
			}
		}
		if (isset($GLOBALS['config']) && is_array($GLOBALS['config'])) {
			foreach ($GLOBALS['config'] as $key => $value) {
				if (isset($this->_config[$key]) && is_array($this->_config[$key]) && is_array($value)) {
					$this->_config[$key] = array_merge($this->_config[$key], $value);
				} else {
					$this->_config[$key] = $GLOBALS['config'][$key];
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
		if ($this->_running) {
			trigger_error('An application instance already running', E_USER_WARNING);
			return;
		}
		$this->_running = true;

		// dispatch
		$this->_request->dispatch();
	}

	/**
	 * config
	 * @access public
	 * @param mixed
	 * @return array
	 */
	public function config($name = null)
	{
		if (is_null($name)) {
			return $this->_config;
		} elseif(isset($this->_config[$name])) {
			return $this->_config[$name];
		} elseif(strstr($name, '.')) {
			$name = explode('.', $name);
			$value = $this->_config;
			foreach ($name as $skey) {
				if (isset($value[$skey])) {
					$value = $value[$skey];
				} else {
					return null;
				}
			}
			return $value;
		} else {
			return null;
		}
	}

	/**
	 * import
	 * @access public
	 * @param string $alias
	 * @param string $classext
	 * @return boolean
	 */
	public function import($alias, $classext = '.class.php')
	{
		$classfile = trim(str_replace('\\', '/', str_replace('.', '/', $alias)), '/');
		$classname = basename($classfile);

		if (empty($this->_imports[$alias])) {
			$classpath = YOD_RUNPATH . '/extends/' . $classfile . $classext;
			if (is_file($classpath)) include $classpath;
			$this->_imports[$alias] = $classpath;
		}

		return class_exists($classname, false) || interface_exists($classname, false);
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
	 * autorun
	 * @access public
	 * @param void
	 * @return void
	 */
	public static function autorun()
	{
		if (defined('YOD_RUNPATH')) {
			Yod_Application::app();
		} else {
			define('YOD_RUNPATH', dirname(__FILE__));
		}
	}

	/**
	 * autoload
	 * 
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

		if (substr($classfile, 0, 4) == 'Yod_') { // yodphp extends class
			if (strncmp($classfile, 'Yod_Db', 6) == 0) {
				$directory = '/drivers/';
			} else {
				$directory = '/extends/';
			}
			$classpath = YOD_EXTPATH . $directory . substr($classfile, 4) . '.class.php';
		} else {
			if (strncmp(substr($classname, -10), 'Controller', 10) == 0) {
				$directory = '/controllers/';
			} elseif (strncmp(substr($classname, -5), 'Model', 5) == 0) {
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
	 * __destruct
	 * @access public
	 * @param void
	 */
	public function __destruct()
	{
		if (!$this->_running) {
			$this->run();
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
				$this->method = 'Cli';
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
		$route = str_replace('\\', '/', $route);
		$route = str_replace('//', '/', $route);
		$route = explode('/', trim($route, '/'));

		if (isset($_SERVER['SCRIPT_FILENAME'])) {
			$controller = basename($_SERVER['SCRIPT_FILENAME'], '.php');
			$classname = $controller . 'Controller';
			if (class_exists($classname, false)) {
				array_unshift($route, $controller);
			}
		}

		$this->controller = empty($route[0]) ? 'Index' : ucfirst(strtolower($route[0]));
		$this->action = empty($route[1]) ? 'index' : strtolower($route[1]);
		$this->params = array();
		$count = count($route);
		for ($i=2; $i<$count; $i+=2) {
			$_GET[$route[$i]] = empty($route[$i+1]) ? '' : $route[$i+1];
			$this->params[$route[$i]] = $_GET[$route[$i]];
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
	 * @param mixed $html
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
	 * @param mixed $action
	 * @param mixed $params
	 * @return void
	 */
	public function __construct($request, $action = null)
	{
		$this->_name = strtolower($request->controller);
		$this->_action = empty($action) ? $request->action : strtolower($action);
		$this->_request = $request;
		$this->_view['tpl_path'] = YOD_RUNPATH . '/views';
		if ($tpl_data = $this->config('tpl_data')) {
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
			$this->_name = empty($this->_name) ? 'index' : strtolower($this->_name);
			$classname = ucfirst($this->_action) . 'Action';
			$classpath = YOD_RUNPATH . '/actions/' . $this->_name . '/' . $classname . '.php';
			if (is_file($classpath)) {
				require $classpath;
				$action = new $classname($this->_request);
				$action->run($this->_request->params);
			} elseif (method_exists($this, 'errorAction')) {
				$this->errorAction($this->_request->params);
			} else {
				$this->_request->errorAction();
			}
		}
	}

	/**
	 * config
	 * @access protected
	 * @param string $name
	 * @return array
	 */
	protected function config($name = null)
	{
		return Yod_Application::app()->config($name);
	}

	/**
	 * import
	 * @access protected
	 * @param string $alias
	 * @param string $classext
	 * @return boolean
	 */
	protected function import($alias, $classext = '.class.php')
	{
		return Yod_Application::app()->import($alias, $classext);
	}

	/**
	 * model
	 * @access protected
	 * @param mixed $name
	 * @param mixed $config
	 * @param boolean $dbmod
	 * @return array
	 */
	protected function model($name = '', $config = '', $dbmod = false)
	{
		if (is_bool($name)) {
			$dbmod = $name;
			$name = '';
		} elseif(is_bool($config)) {
			$dbmod = $config;
			$config = '';
		}
		if (empty($name)) {
			$name = ucfirst($this->_name);
		}
		if ($dbmod) {
			//return Yod_DbModel::getInstance($name, $config);
		}
		return Yod_Model::getInstance($name, $config);
	}

	/**
	 * assign
	 * @access protected
	 * @param mixed $name
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
	 * @param mixed $view
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
			$view = '/' . $this->_name . '/' . $view;
		}
		if (empty($this->_view['tpl_path']) || !is_string($this->_view['tpl_path'])) {
			trigger_error('Unavailable property ' . __CLASS__ . '::$_view', E_USER_WARNING);
			return null;
		}
		$this->_view['tpl_file'] = $this->_view['tpl_path'] . strtolower($view) . '.php';
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
	 * @param mixed $view
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
	 * @return void
	 */
	protected function widget($route)
	{
		$route = str_replace('\\', '/', $route);
		$route = str_replace('//', '/', $route);
		$route = explode('/', trim($route, '/'));

		$widget = empty($route[0]) ? 'Index' : ucfirst(strtolower($route[0]));
		$action = empty($route[1]) ? 'index' : strtolower($route[1]);
		$params = array();
		$count = count($route);
		for ($i=2; $i<$count; $i+=2) {
			$params[$route[$i]] = empty($route[$i+1]) ? '' : $route[$i+1];
		}

		$classname = $widget . 'Widget';
		if (class_exists($classname, false)) {
			new $classname($this->_request, $action, $params);
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
	 * @param mixed $action
	 * @param mixed $params
	 * @return void
	 */
	public function __construct($request, $action = null, $params = null)
	{
		$name = get_class($this);
		$this->_name = strtolower(substr($name, 0, -6));
		$this->_action = empty($action) ? 'index' : strtolower($action);
		$this->_request = $request;
		$this->_view['tpl_path'] = YOD_RUNPATH . '/widgets';

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
	 * @return void
	 */
	public function __construct($name = '', $config = '')
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
	public static function getInstance($name, $config = '')
	{
		if (empty($name)) {
			$classname = 'Yod_Model';
		} else {
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
	 * @param string	$where
	 * @param array		$params
	 * @param mixed		$select
	 * @return mixed
	 */
	public function find($where = '', $params = array(), $select = '*')
	{
		if ($result = $this->_db->select($select, $this->_table, $where, $params, "LIMIT 1")) {
			$data = $this->_db->fetch($result);
			$this->_db->free($result);
			return $data;
		}
		return false;
	}

	/**
	 * findAll
	 * @access public
	 * @param string	$where
	 * @param array		$params
	 * @param mixed		$select
	 * @return mixed
	 */
	public function findAll($where = '', $params = array(), $select = '*')
	{
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
	 * @return boolean
	 */
	public function save($data, $where = '', $params = array())
	{
		if (empty($data)) return false;

		if (empty($where)) {
			return $this->_db->insert($data, $this->_table);
		} else {
			return $this->_db->update($data, $this->_table, $where, $params);
		}
	}

	/**
	 * remove
	 * @access public
	 * @return boolean
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

	/**
	 * config
	 * @access protected
	 * @param void
	 * @return array
	 */
	protected function config($name = null)
	{
		return Yod_Application::app()->config($name);
	}
	
	/**
	 * import
	 * @access protected
	 * @param string $alias
	 * @param string $classext
	 * @return boolean
	 */
	protected function import($alias, $classext = '.class.php')
	{
		return Yod_Application::app()->import($alias, $classext);
	}

	/**
	 * model
	 * @access protected
	 * @param string $name
	 * @return array
	 */
	protected function model($name = '', $config = '', $dbmod = false)
	{
		if (empty($name)) {
			return $this;
		}
		if(is_bool($config)) {
			$dbmod = $config;
			$config = '';
		}
		if ($dbmod) {
			//return Yod_DbModel::getInstance($name, $config);
		}
		return Yod_Model::getInstance($name, $config);
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
			$config = Yod_Application::app()->config($config);
		}

		if (!is_array($config)) {
			return false;
		}

		if (empty($config['type']) || !is_string($config['type'])) {
			$config['type'] = 'pdo';
		}

		$md5hash = md5(serialize($config));
		if (empty(self::$_db[$md5hash])) {
			if ($config['type'] == 'pdo') {
				$classname = 'Yod_DbPdo';
			} else {
				$classname = 'Yod_Db'.ucwords($config['type']);
			}
			if (!class_exists($classname, false)) {
				include YOD_EXTPATH . '/drivers/' . substr($classname, 4) . '.class.php';
			}
			self::$_db[$md5hash] = new $classname($config);
		}

		return self::$_db[$md5hash];
	}

	/**
	 * config
	 * @access public
	 * @param void
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
	 * @return mixed
	 */
	public function create($fields, $table, $extend = '')
	{
		if (empty($fields) || empty($table)) return false;
		foreach ($fields as $key => $value) {
			$fields[$key] = $key . ' ' . $value;
		}
		$query = 'CREATE TABLE ' . $this->_prefix . $table . ' (' . implode(', ', $fields) . ')' . $extend;
		return $this->execute($query);
	}

	/**
	 * insert
	 * @access public
	 * @return mixed
	 */
	public function insert($data, $table, $replace=false)
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
	 * @return integer
	 */
	public function update($data, $table, $where = null, $params = array())
	{
		if (empty($data) || empty($table)) return false;
		$params1 = $update = array();
		foreach ($data as $key => $value) {
			if(is_scalar($value) || is_null(($value))) {
				$name = ':'. md5($key);
				$update[] = $key.' = '.$name;
				$params1[$name] = $value;
			}
		}
		$params1 = array_merge($params1, $params);
		$query = 'UPDATE ' . $this->_prefix . $table . ' SET ' .implode(', ', $update) . (empty($where) ? '' : ' WHERE ' . $where);
		return $this->execute($query, $params1, true);
	}

	/**
	 * delete
	 * @access public
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
	 * @return string
	 */
	public function lastQuery()
	{
		return $this->_lastquery;
	}

	/**
	 * dbconfig
	 * @access protected
	 * @return void
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
	* @return array
	*/
	abstract public function fields($table);

	/**
	 * execute
	 * @access public
	 * @return mixed
	 */
	abstract public function execute($query, $params = array(), $affected = false);

	/**
	 * query
	 * @access public
	 * @return mixed
	 */
	abstract public function query($query, $params = array());

	/**
	 * fetch
	 * @access public
	 * @return mixed
	 */
	abstract public function fetch($result = null);

	/**
	 * fetchAll
	 * @access public
	 * @return mixed
	 */
	abstract public function fetchAll($result = null);

	/**
	 * transaction
	 * @access public
	 * @return boolean 
	 */
	abstract public function transaction();

	/**
	 * commit
	 * @access public
	 * @return boolean 
	 */
	abstract public function commit();

	/**
	 * rollback
	 * @access public
	 * @return boolean 
	 */
	abstract public function rollback();

	/**
	 * insertid
	 * @access public
	 * @return integer
	 */
	abstract public function insertid();

	/**
	 * quote
	 * @access public
	 * @return string
	 */
	abstract public function quote($string);

	/**
	 * free
	 * @access public
	 * @return mixed
	 */
	abstract public function free($result = null);

	/**
	 * close
	 * @access public
	 * @return mixed
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
	 * @param mixed $config
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
		if (empty($config['dsn'])) {
			trigger_error('Database DSN configure error', E_USER_ERROR);
			return false;
		}
		$config['user'] = empty($config['user']) ? null : $config['user'];
		$config['pass'] = empty($config['pass']) ? null : $config['pass'];
		$config['charset'] = empty($config['charset']) ? 'utf8' : $config['charset'];
		$config['options'] = empty($config['options']) ? array() : $config['options'];
		if (isset($config['pconnect']) && $config['pconnect'] == true) {
			$config['options'][PDO::ATTR_PERSISTENT] = true;
		}
		try {
			$this->_linkids[$linknum] = new PDO($config['dsn'], $config['user'], $config['pass'], $config['options']);
		} catch (PDOException $e) {
			trigger_error($e->getMessage(), E_USER_ERROR);
			return false;
		}

		$this->_linkids[$linknum]->exec('SET NAMES '. $config['charset']);
		if (error_reporting()) {
			$this->_linkids[$linknum]->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_WARNING);
		}
		return $this->_linkid = $this->_linkids[$linknum];
	}

	/**
	* fields
	* @access public
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
	 * fetch
	 * @access public
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
	 * @return boolean 
	 */
	public function rollback()
	{
		$this->_locked = false;
		$this->connect($this->_config, 0);
		return $this->_linkid->rollBack();
	}

	/**
	 * insertid
	 * @access public
	 * @return integer
	 */
	public function insertid()
	{
		return $this->_linkid->lastInsertId();
	}

	/**
	 * quote
	 * @access public
	 * @return string
	 */
	public function quote($string)
	{
		return $this->_linkid->quote($string);
	}

	/**
	 * free
	 * @access public
	 * @return mixed
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
	 * @return mixed
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
