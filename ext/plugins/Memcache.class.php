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

class Yod_Memcache extends Memcache
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
		if (is_array($config)) {
			$host = empty($config['host']) ? 'localhost' : $config['host'];
			$port = empty($config['port']) ? '11211' : $config['port'];
			try {
				$this->connect($host, $port);
				$this->_connected = true;
			} catch (Exception $e) {
				trigger_error($e->getMessage(), E_USER_WARNING);
				$this->_connected = false;
			}
		}
	}

	/**
	 * add
	 * @access public
	 * @param string $key
	 * @param mixed $var
	 * @param integer $expire
	 * @param integer $flag
	 * @return boolean
	 */
	public function add($key, $var, $expire = 0, $flag = false)
	{
		if ($this->_connected) {
			return parent::add($key, $var, $flag, $expire);
		}
		return false;
	}
	
	/**
	 * set
	 * @access public
	 * @param string $key
	 * @param mixed $var
	 * @param integer $expire
	 * @param integer $flag
	 * @return boolean
	 */
	public function set($key, $var, $expire = 0, $flag = false)
	{
		if ($this->_connected) {
			return parent::set($key, $var, $flag, $expire);
		}
		return false;
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
