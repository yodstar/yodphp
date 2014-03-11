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

abstract class Yod_ViewController extends Yod_Controller
{
	protected $_view;

	/**
	 * __construct
	 * @access public
	 * @param Yod_Request $request
	 * @param string $action
	 * @return void
	 */
	public function init()
	{
		($tpl_view = $this->config('tplview')) or ($tpl_view = $this->config('tpl_view'));

		if (empty($tpl_view['class'])) {
			return;
		}

		$this->import($tpl_view['class']);

		$classname = str_replace('..', '.', str_replace('\\', '.', str_replace('/', '.', $tpl_view['class'])));
		if (strstr($classname, '.')) {
			$classname = substr($classname, strrpos($classname, '.') + 1);
		}

		$this->_view = new $classname();

		if (is_array($tpl_view['config'])) {
			foreach ($tpl_view['config'] as $property => $value) {
				$this->_view->$property = $value;
			}
		}

		if ($tpl_data = $this->config('tpl_data')) {
			$this->_view->assign($tpl_data);
		}

		parent::init();
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
		$this->_view->assign($name, $value);

		return $this;
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
		empty($data) or $this->_view->assign($data);
		$this->_view->display($view);
	}

}
