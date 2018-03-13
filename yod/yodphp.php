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
defined('YOD_RUNTIME') or define('YOD_RUNTIME', microtime(true) * 1000);
defined('YOD_VERSION') or define('YOD_VERSION', '1.4.2');
defined('YOD_FORWARD') or define('YOD_FORWARD', 5);
defined('YOD_RUNMODE') or define('YOD_RUNMODE', 3);
defined('YOD_CHARSET') or define('YOD_CHARSET', 'utf-8');
defined('YOD_MODULES') or define('YOD_MODULES', 'Home');
defined('YOD_VIEWEXT') or define('YOD_VIEWEXT', '.php');
defined('YOD_PATHVAR') or define('YOD_PATHVAR', '');

// init_startup
Yod_Base::init_startup();

/**
 * Yod_Application
 * 
 */
final class Yod_Application
{
	protected static $_app;
	protected static $_running = false;

	protected $_request = null;

	/**
	 * __construct
	 * @access public
	 * @return void
	 */
	public function __construct()
	{
		if (is_object(self::$_app)) {
			trigger_error('Only one application can be initialized', E_USER_ERROR);
			return;
		}

		defined('YOD_RUNPATH') or define('YOD_RUNPATH', dirname(__FILE__));
		defined('YOD_LIBPATH') or define('YOD_LIBPATH', dirname(__FILE__));

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
	 * @param void
	 * @return Yod_Application
	 */
	public static function app()
	{
		if (self::$_app) {
			return self::$_app;
		}
		return new self();
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
		if ((YOD_RUNMODE & 8) && defined('YOD_DATADIR')) {
			$logfile = YOD_DATADIR . '/debug.log';
			is_dir(YOD_DATADIR) or mkdir(YOD_DATADIR);
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
	public $module;
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
		($rules = Yod::config('urlrules')) or ($rules = Yod::config('url_rules'));
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
				$this->module = 'Home';
			}
		}

		if (empty($this->module)) {
			$modules = explode(',', strtolower(YOD_MODULES));
			if (in_array(strtolower($route[0]), $modules)) {
				$this->module = ucfirst(strtolower(array_shift($route)));
			} else {
				$this->module = 'Home';
			}
		}
		Yod_Base::set_modname($this->module);

		$this->controller = empty($route[0]) ? 'Index' : implode('', array_map('ucfirst', explode('-', strtolower(str_replace('--', '_', $route[0])))));
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
			$classpath = YOD_RUNPATH . '/' . $this->module . '/Controller/' . $controller . 'Controller.php';
			if (is_file($classpath)) {
				require $classpath;
				new $classname($this);
			} else {
				$action = empty($this->action) ? 'Index' : ucfirst($this->action);
				$classpath = YOD_RUNPATH . '/' . $this->module . '/Action/' . strtolower($controller) . '/' . $action . 'Action.php';
				if (is_file($classpath)) {
					require $classpath;
					$classname = $action . 'Action';
					new $classname($this);
				} else {
					$classpath = YOD_RUNPATH . '/' . $this->module . '/Controller/ErrorController.php';
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
		$classpath = YOD_RUNPATH . '/' . $this->module . '/Action/' . $controller . '/ErrorAction.php';
		if (is_file($classpath)) {
			require $classpath;
			new ErrorAction($this);
		} else {
			$this->controller = 'Error';
			$classpath = YOD_RUNPATH . '/' . $this->module . '/Action/ErrorAction.php';
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
	public function __construct(Yod_Request $request, $action = null)
	{
		$this->_name = strtolower($request->controller);
		$this->_action = empty($action) ? $request->action : strtolower($action);
		$this->_request = $request;
		$this->_view['tpl_path'] = YOD_RUNPATH . '/' . $request->module . '/View';
		($tpl_data = Yod::config('tpldata')) or ($tpl_data = Yod::config('tpl_data'));
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
			$classpath = YOD_RUNPATH . '/' . $this->_request->module . '/Action/' . $cname . '/' . $classname . '.php';
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
	protected function render($view = null, array $data = array())
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
	protected function display($view = null, array $data = array())
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
	protected function widget($route, array $params = array())
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
			$classpath = YOD_RUNPATH . '/' . $this->_request->module . '/Widget/' . $widget . 'Widget.php';
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
	public function __construct(Yod_Request $request, $action = null, array $params = null)
	{
		$name = get_class($this);
		$this->_name = strtolower(substr($name, 0, -6));
		$this->_action = empty($action) ? 'index' : strtolower($action);
		$this->_request = $request;
		$this->_view['tpl_path'] = YOD_RUNPATH . '/' . $request->module . '/Widget';
		($tpl_data = Yod::config('tpldata')) or ($tpl_data = Yod::config('tpl_data'));
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
 * Yod_Server
 * 
 */
class Yod_Server
{
	protected $_handle = null;

	/**
	 * __construct
	 * @access public
	 * @param mixed $handle
	 * @return void
	 */
	public function __construct($handle = null)
	{
		$this->_handle = $handle;
	}

	/**
	 * handle
	 * @access public
	 * @return void
	 */
	public function handle()
	{

		$input = file_get_contents('php://input');
		$input = $this->decrypt($input);
		$request = json_decode($input, true);

		if (empty($request)) {
			return;
		}

		$result = array('server' => 'Yod_Server', 'status' => 0, 'data' => null, 'extra' => null);
		if (!empty($request['handle'])) {
			$handle = trim(str_replace('//', '/', str_replace('.', '/', $request['handle'])), '/');
			$handle = implode('/', array_map('ucfirst', explode('/', strtolower($handle))));
			if (strncasecmp(substr($handle, -5), 'Model', 5) == 0) {
				$classname = basename(substr($handle, 0, -5)) . 'Model';
			} else {
				$classname = basename($handle) . 'Service';
			}
			if (!class_exists($classname, false)) {
				if (strncasecmp(substr($handle, -5), 'Model', 5) == 0) {
					$classfile = YOD_LIBPATH . '/Model/' . substr($handle, 0, -5) . 'Model.php';
				} else {
					$classfile = YOD_LIBPATH . '/Service/' . $handle . 'Service.php';
				}
				if (is_file($classfile)) {
					require $classfile;
				}
			}
			if (class_exists($classname, false)) {
				$this->_handle = new $classname();
			} else {
				$result['data'] = sprintf('Class \'%s\' not found', $classname);
			}
		}

		if (empty($result['data'])) {
			if (is_object($this->_handle)) {
				if (empty($request['method'])) {
					$result['data'] = 'Call to undefined method';
				} else {
					$extra = isset($request['extra']) ? $request['extra'] : null;
					$method = isset($request['method']) ? $request['method'] : null;
					$params = isset($request['params']) ? $request['params'] : null;
					if (method_exists($this->_handle, $method)) {
						if (is_array($extra)) {
							foreach ($extra as $name => $value) {
								$this->_handle->$name = $value;
							}
						}
						$result['status'] = 1;
						$result['data'] = call_user_func_array(array($this->_handle, $method), $params);
						$result['extra'] = get_object_vars($this->_handle);
					} else {
						$result['data'] = sprintf('Call to undefined method %s::%s()', get_class($this->_handle), $method);
					}
				}
			} else {
				$result['data'] = 'Call to undefined handle';
			}
		}
		
		$output = @json_encode($result);
		$output = $this->encrypt($output);
		file_put_contents('php://output', $output);
	}

	/**
	 * encrypt
	 * @access public
	 * @param string 		$data
	 * @return string
	 */
	protected function encrypt($data)
	{
		return $data;
	}

	/**
	 * decrypt
	 * @access public
	 * @param string 		$data
	 * @return string
	 */
	protected function decrypt($data)
	{
		return $data;
	}
}

/**
 * Yod_Client
 * 
 */
class Yod_Client
{
	protected $_url;
	protected $_data = null;
	protected $_params = null;
	protected $_extra = array();
	protected $_handle = null;
	protected $_timeout = 5;

	protected $_debug = false;

	/**
	 * __construct
	 * @access public
	 * @param string $url
	 * @param string $handle
	 * @param integer $timeout
	 * @return void
	 */
	public function __construct($url, $handle = null, $timeout = 5)
	{
		$this->_url = $url;
		$this->_handle = $handle;
		$this->_timeout = $timeout;
	}

	/**
	 * __get
	 * @access public
	 * @param string $name
	 * @return mixed
	 */
	public function __get($name)
	{
		return isset($this->_extra[$name]) ? $this->_extra[$name] : null;
	}

	/**
	 * __get
	 * @access public
	 * @param string $name
	 * @param mixed $value
	 * @return void
	 */
	public function __set($name, $value)
	{
		$this->_extra[$name] = $value;
	}

	/**
	 * __isset
	 * @access public
	 * @param string $name
	 * @return boolean
	 */
	public function __isset($name)
	{
		return isset($this->_extra[$name]);
	}

	/**
	 * __unset
	 * @access public
	 * @param string $name
	 * @return void
	 */
	public function __unset($name)
	{
		unset($this->_extra[$name]);
	}

	/**
	 * __call
	 * @access public
	 * @param string 		$method
	 * @param array 		$params
	 * @return mixed
	 */
	public function __call($method, array $params)
	{
		$post = array(
			'client' => 'Yod_Client',
			'handle' => $this->_handle,
			'method' => $method,
			'params' => $params,
			'extra' => $this->_extra,
		);
		$data = $this->curl_post($post);
		if (empty($data['status'])) {
			$error = empty($data['data']) ? 'Unknown error' : $data['data'];
			throw new Exception($error);
		} else {
			if (isset($data['extra']) && is_array($data['extra'])) {
				foreach ($data['extra'] as $name => $value) {
					$this->_extra[$name] = $value;
				}
			}
			return isset($data['data']) ? $data['data'] : null;
		}
	}

	/**
	 * debug
	 * @access public
	 * @param boolean 		$debug
	 * @return void
	 */
	public function debug($debug = true)
	{
		$this->_debug = (bool)$debug;
	}

	/**
	 * timeout
	 * @access public
	 * @param integer 		$timeout
	 * @return void
	 */
	public function timeout($timeout)
	{
		$this->_timeout = $timeout;
	}

	/**
	 * encrypt
	 * @access public
	 * @param string 		$data
	 * @return string
	 */
	protected function encrypt($data)
	{
		return $data;
	}

	/**
	 * decrypt
	 * @access public
	 * @param string 		$data
	 * @return string
	 */
	protected function decrypt($data)
	{
		return $data;
	}

	/**
	 * curl_post
	 * @access public
	 * @param array 		$post
	 * @return string
	 */
	private function curl_post($post)
	{
		$this->_params = json_encode($post);
		$ch = curl_init();
		curl_setopt($ch, CURLOPT_URL, $this->_url);
		curl_setopt($ch, CURLOPT_TIMEOUT, $this->_timeout);
		curl_setopt($ch, CURLOPT_POST, 1); 
		curl_setopt($ch, CURLOPT_POSTFIELDS, $this->encrypt($this->_params));
		curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
		curl_setopt($ch, CURLOPT_HEADER, 0);
		$resp = curl_exec($ch);
		$data = $this->decrypt($resp);
		if (empty($data)) {
			if ($this->_debug) {
				echo '<fieldset style="width:75%"><legend><b>[ERROR]</b></legend>'. $resp .'</fieldset><br>';
			}
			return null;
		}
		if ($this->_debug) {
			echo '<fieldset style="width:75%"><legend><b>[DEBUG]</b></legend>'. $data .'</fieldset><br>';
		}
		if ($error = curl_error($ch)) {
			if ($this->_debug) {
				echo '<fieldset style="width:75%"><legend><b>[ERROR]</b></legend>'. $resp .'</fieldset><br>';
			}
			throw new Exception($error);
		}
		curl_close($ch);
		return json_decode($data, true);
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
	protected $_data = array();

	protected $_name;
	protected $_table;
	protected $_fields;
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
			$config = Yod::config($this->_dsn);
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
			if (class_exists($classname, false)) {
				self::$_model[$name] = new $classname($name, $config);
			} else {
				$classpath = YOD_LIBPATH . '/Model/' . $classname . '.php';
				if (is_file($classpath)) {
					include $classpath;
					self::$_model[$name] = new $classname($name, $config);
				} else {
					$modname = Yod_Base::get_modname();
					$classpath = YOD_RUNPATH . '/' . $modname . '/Model/' . $classname . '.php';
					if (is_file($classpath)) {
						include $classpath;
						self::$_model[$name] = new $classname($name, $config);
					} else {
						self::$_model[$name] = new self($name, $config);
					}
				}
			}
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
	public function find($where = null, array $params = array(), $select = null)
	{
		if (is_numeric($where)) {
			$where = 'id = '. $where;
		}
		if (is_null($select)) {
			$select = empty($this->_fields) ? '*' : $this->_fields;
		}
		if ($result = $this->_db->select($select, $this->_table, $where, $params, 'LIMIT 1')) {
			$this->_data = $this->_db->fetch($result);
			$this->_db->free($result);
			return $this->_data;
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
	public function select($where = null, array $params = array(), $select = null)
	{
		if (is_numeric($where)) {
			$where = 'id = '. $where;
		}
		if (is_null($select)) {
			$select = empty($this->_fields) ? '*' : $this->_fields;
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
	public function count($where = '', array $params = array())
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
	public function save(array $data)
	{
		if ($this->_db->insert($data, $this->_table)) {
			$id = $this->_db->insertId();
			return ($id == '0') ? true : $id;
		}
		return false;
	}

	/**
	 * update
	 * @access public
	 * @param array		$data
	 * @param string	$where
	 * @param array		$params
	 * @return integer
	 */
	public function update(array $data, $where = '', array $params = array())
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
	public function remove($where, array $params = array())
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
	 * __get
	 */
	public function __get($name)
	{
		return isset($this->_data[$name]) ? $this->_data[$name] : null;
	}

	/**
	 * __set
	 */
	public function __set($name, $value)
	{
		$this->_data[$name] = $value;
	}

	/**
	 * __isset
	 */
	public function __isset($name)
	{
		return isset($this->_data[$name]);
	}

	/**
	 * __unset
	 */
	public function __unset($name)
	{
		unset($this->_data[$name]);
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
	public function __construct(array $config)
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
			$config = Yod::config($config);
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
				include YOD_LIBPATH . '/Driver/' . substr($classname, 4) . '.php';
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
	public function config($name = null, $value = null)
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
	public function insert(array $data, $table, $replace = false)
	{
		if (empty($data) || empty($table)) return false;
		$values = $fields = $params = array();
		foreach ($data as $key => $value) {
			if(is_scalar($value) || is_null($value)) {
				$fields[] =  $key;
				$values[] = '?';
				$params[] = $value;
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
	public function update(array $data, $table, $where = null, array $params = array())
	{
		if (empty($data) || empty($table)) return false;
		$params1 = $update = array();
		foreach ($data as $key => $value) {
			if (is_scalar($value) || is_null($value)) {
				if (is_numeric($key)) {
					$update[] = $value;
				} else {
					$update[] = $key.' = ?';
					$params1[] = $value;
				}
			}
		}
		foreach ($params as $key => $value) {
			$params1[] = $value;
			if (is_numeric($key)) {
				continue;
			}
			$where = str_replace($key, '?', $where);
		}
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
	public function delete($table, $where = null, array $params = array())
	{
		if (empty($table)) return false;
		$params1 = array();
		foreach ($params as $key => $value) {
			$params1[] = $value;
			if (is_numeric($key)) {
				continue;
			}
			$where = str_replace($key, '?', $where);
		}
		$query = 'DELETE FROM ' . $this->_prefix . $table . (empty($where) ? '' : ' WHERE ' . $where);
		return $this->execute($query, $params1, true);
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
	public function select($select, $table, $where = null, array $params = array(), $extend = null)
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
		$params1 = array();
		foreach ($params as $key => $value) {
			$params1[] = $value;
			if (is_numeric($key)) {
				continue;
			}
			$where = str_replace($key, '?', $where);
		}
		$query = 'SELECT ' . (empty($select) ? '*' : $select) . ' FROM ' . $this->_prefix . $table . (empty($where) ? '' : ' WHERE ' . $where) . (empty($extend) ? '' : ' ') . $extend;
		return $this->query($query, $params1);
	}

	/**
	 * queryData
	 * @access public
	 * @param string $query
	 * @param array $params
	 * @return mixed
	 */
	public function queryData($query, array $params = array())
	{
		$result = $this->query($query, $params);
		$data = $this->fetch($result);
		$this->free($result);
		return $data;
	}

	/**
	 * queryRows
	 * @access public
	 * @param string $query
	 * @param array $params
	 * @return mixed
	 */
	public function queryRows($query, array $params = array())
	{
		$result = $this->query($query, $params);
		$rows = $this->fetchAll($result);
		$this->free($result);
		return $rows;
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
	protected function dbconfig(array $config, $linknum = 0)
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
	abstract public function connect(array $config = null, $linknum = 0);

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
	abstract public function execute($query, array $params = array(), $affected = false);

	/**
	 * query
	 * @access public
	 * @param string $query
	 * @param array $params
	 * @return mixed
	 */
	abstract public function query($query, array $params = array());
	
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
	public function __construct(array $config)
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
	public function connect(array $config = null, $linknum = 0)
	{
		$config = $this->dbconfig($config, $linknum);
		$linknum = isset($config['linknum']) ? $config['linknum'] : 0;
		if (isset($this->_linkids[$linknum])) {
			$this->_linkid = $this->_linkids[$linknum];
			$errinfo = $this->_linkid->errorInfo();
			if ($errinfo[1] != 2013 && $errinfo[1] != 2006) {
				return $this->_linkid;
			}
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
			$this->_linkids[$linknum]->setAttribute(PDO::ATTR_EMULATE_PREPARES, false);
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
	public function execute($query, array $params = array(), $affected = false)
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
			if ($this->_result->execute($params)) {
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
	public function query($query, array $params = array())
	{
		$this->connect($this->_config, 1);

		$this->_lastquery = $query;
		if (empty($params)) {
			return $this->_result = $this->_linkid->query($query);
		} else {
			if ($this->_result = $this->_linkid->prepare($query)) {
				$this->_result->execute($params);
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
 * Yod_Base
 *
 */
abstract class Yod_Base
{
	protected static $_config = array();
	protected static $_imports = array();
	protected static $_plugins = array();
	protected static $_modname = 'Home';
	protected static $_startup = false;

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
	 * @return mixed
	 */
	public static function config($name = null)
	{
		if (empty(self::$_startup)) {
			self::init_startup();
		}
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
	 * session
	 * @access public
	 * @param string $name
	 * @param mixed $value
	 * @return mixed
	 */
	public static function session($name = null, $value = null)
	{
		isset($_SESSION) or session_start();

		$num_args = func_num_args();
		if ($num_args == 0) {
			return $_SESSION;
		}
		if ($num_args > 1) {
			if (is_null($value)) {
				if (is_null($name)) {
					return $_SESSION;
				}
				unset($_SESSION[$name]);
			} else {
				$_SESSION[$name] = $value;
			}
			return;
		}
		if (is_null($name)) {
			$_SESSION = array();
			return;
		}
		if (strpos($name, '.')) {
			list($name1, $name2) = explode('.', $name, 2);
			return isset($_SESSION[$name1][$name2]) ? $_SESSION[$name1][$name2] : null;
		} else {
			return isset($_SESSION[$name]) ? $_SESSION[$name] : null;
		}
	}

	/**
	 * import
	 * @access public
	 * @param string $alias
	 * @param string $classext
	 * @return boolean
	 */
	public static function import($alias, $classext = '.php')
	{
		$classfile = trim(str_replace('\\', '/', str_replace('.', '/', $alias)), '/');
		$classname = basename($classfile);

		if (empty(self::$_imports[$alias])) {
			if (class_exists($classname, false) || interface_exists($classname, false)) {
				return self::$_imports[$alias] = true;
			}

			if (strncasecmp($classfile, 'yod/', 4) == 0) {
				$classpath = YOD_LIBPATH . '/Extend/Yod/' . substr($classfile, 4) . $classext;
			} else {
				$classpath = YOD_LIBPATH . '/Extend/' . $classfile . $classext;
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
	public static function plugin($alias, $classext = '.php')
	{
		$classfile = trim(str_replace('\\', '/', str_replace('.', '/', $alias)), '/');
		$classname = basename($classfile);

		if (empty(self::$_plugins[$alias])) {
			if (!class_exists($classname, false) && !interface_exists($classname, false)) {
				if (strncasecmp($classfile, 'yod/', 4) == 0) {
					if (strncasecmp($classname, 'Yod_', 4)) {
						$classname = 'Yod_' . $classname;
					}
					$classpath = YOD_LIBPATH . '/Plugin/Yod/' . substr($classfile, 4) . $classext;
				} elseif (strncasecmp($classname, 'Yod_', 4) == 0) {
					$classpath = YOD_LIBPATH . '/Plugin/Yod/' . substr($classfile, 0, -strlen($classname)) . substr($classname, 4) . $classext;
				} else {
					$classpath = YOD_LIBPATH . '/Plugin/' . $classfile . $classext;
				}
				if (is_file($classpath)) include $classpath;
			}
			
			$config = self::config('plugin.' . strtolower($alias));
			if (is_null($config)) {
				self::$_plugins[$alias] = new $classname();
			} else {
				self::$_plugins[$alias] = new $classname($config);
			}
		}

		return self::$_plugins[$alias];
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
	 * db
	 * @access public
	 * @param mixed $config
	 * @return Yod_Database
	 */
	public static function db($config = 'db_dsn')
	{
		return Yod_Database::getInstance($config);
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
			$classfile = str_replace('\\', '/', $classname);
		}

		if (strncasecmp($classfile, 'Yod_', 4) == 0) { // yodphp extends class
			if (strncasecmp($classfile, 'Yod_Db', 6) == 0) {
				$directory = '/Driver/';
			} else {
				$directory = '/Extend/Yod/';
			}
			$classpath = YOD_LIBPATH . $directory . substr($classfile, 4) . '.php';
		} else {
			$modname = Yod_Base::get_modname();
			if (strncasecmp(substr($classname, -10), 'Controller', 10) == 0) {
				$classpath = YOD_RUNPATH . '/' . $modname . '/Controller/' . $classfile . '.php';
				if (is_file($classpath)) include $classpath;
			} elseif (strncasecmp(substr($classname, -5), 'Model', 5) == 0) {
				$classpath = YOD_RUNPATH . '/' . $modname . '/Model/' . $classfile . '.php';
				if (is_file($classpath)) {
					include $classpath;
				} else {
					$classpath = YOD_LIBPATH . '/Model/' . $classfile . '.php';
					if (is_file($classpath)) include $classpath;
				}
			} elseif (strncasecmp(substr($classname, -7), 'Service', 7) == 0) {
				$classpath = YOD_RUNPATH . '/' . $modname . '/Service/' . $classfile . '.php';
				if (is_file($classpath)) {
					include $classpath;
				} else {
					$classpath = YOD_LIBPATH . '/Service/' . $classfile . '.php';
					if (is_file($classpath)) include $classpath;
				}
			} else {
				$classpath = YOD_LIBPATH . '/Extend/' . $classfile . '.php';
				if (is_file($classpath)) include $classpath;
			}
		}

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
		$logfile = YOD_DATADIR . '/error.log';
		is_dir(YOD_DATADIR) or mkdir(YOD_DATADIR);
		file_put_contents($logfile, $logdata, FILE_APPEND);

		return false;
	}

	/**
	 * init_config
	 * @access public
	 * @param string $config
	 * @return mixed
	 */
	public static function init_config()
	{
		// config
		$config = YOD_RUNPATH . '/Config/config.php';
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
	}

	/**
	 * init_register
	 * @access public
	 * @param void
	 * @return void
	 */
	public static function init_register()
	{
		// errorlog
		if ((YOD_RUNMODE & 2) && defined('YOD_DATADIR')) {
			set_error_handler(array('Yod_Base', 'errorlog'));
		}

		// autoload
		spl_autoload_register(array('Yod_Base', 'autoload'));

	}

	/**
	 * init_autorun
	 * @access public
	 * @param void
	 * @return void
	 */
	public static function init_autorun()
	{
		if (YOD_RUNMODE & 1) {
			if (defined('YOD_RUNPATH')) {
				Yod_Application::app()->run();
				exit;
			} else {
				define('YOD_RUNPATH', dirname(__FILE__));
			}
		}
	}

	/**
	 * init_startup
	 * @access public
	 * @param void
	 * @return void
	 */
	public static function init_startup()
	{
		if (self::$_startup) {
			return;
		}
		self::$_startup = true;
		self::init_config();
		self::init_register();
		self::init_autorun();
	}

	/**
	 * set_modname
	 * @access public
	 * @param string 	$modname
	 * @return void
	 */
	public static function set_modname($modname)
	{
		self::$_modname = $modname;
	}

	/**
	 * get_modname
	 * @access public
	 * @param void
	 * @return void
	 */
	public static function get_modname()
	{
		return self::$_modname;
	}


}

/**
 * Yod_Base
 *
 */
abstract class Yod extends Yod_Base
{

}
