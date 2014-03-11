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

class Yod_Redis extends Redis
{
	protected $_connected = false;

	/**
	 * __construct
	 * @access public
	 * @param mixed $config
	 * @return void
	 */
	public function __construct($config = null)
	{
		parent::__construct();

		if (is_array($config)) {
			$host = empty($config['host']) ? 'localhost' : $config['host'];
			$port = empty($config['port']) ? '6379' : $config['port'];
			try {
				$this->connect($host, $port);
				empty($config['auth']) or $this->auth($config['auth']);
				$this->_connected = true;
			} catch (Exception $e) {
				trigger_error($e->getMessage(), E_USER_WARNING);
				$this->_connected = false;
			}
		}
	}

	/**
	 * set
	 * @access public
	 * @param string $key
	 * @param mixed $var
	 * @param integer $expire
	 * @return boolean
	 */
	public function set($key, $var, $expire = 0)
	{
		if ($this->_connected) {
			try {
				return parent::set($key, $var, $expire);
			} catch (Exception $e) {
				$this->_connected = false;
			}
		} else {
			return false;
		}
	}
	
	/**
	 * get
	 * @access public
	 * @param string $key
	 * @return mixed
	 */
	public function get($key)
	{
		if ($this->_connected) {
			try {
				return parent::get($key);
			} catch (Exception $e) {
				$this->_connected = false;
			}
		} else {
			return false;
		}
	}

	/**
	 * __destruct
	 * @access public
	 * @param void
	 */
	public function __destruct()
	{
		if ($this->_connected) {
			try {
				$this->close();
			} catch (Exception $e) {

			}
		}
	}
	
}
