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

class Yod_Fcache
{
	protected $_path;

	/**
	 * __construct
	 * @access public
	 * @param mixed $config
	 * @return void
	 */
	public function __construct($config = null)
	{
		$this->_path = empty($config['path']) ? YOD_RUNPATH . '/caches' : $config['path'];
		is_dir($this->_path) or mkdir($this->_path);
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
		$file = $this->file($key);
		$path = dirname($file);
		is_dir($path) or mkdir($path, 0700, true);
		$data = var_export(array('var' => $var, 'exp' => time() + $expire), true);
		return file_put_contents($file, '<?php return '. $data .';');
	}
	
	/**
	 * get
	 * @access public
	 * @param string $key
	 * @return mixed
	 */
	public function get($key)
	{
		$file = $this->file($key);
		if (is_file($file)) {
			$data = include $file;
			if (empty($data['exp']) || ($data['exp'] > time())) {
				return isset($data['var']) ? $data['var'] : null;
			}
		}
		return false;
	}

	/**
	 * del
	 * @access public
	 * @param string $key
	 * @return boolean
	 */
	public function del($key)
	{
		$file = $this->file($key);
		if (is_file($file)) {
			return unlink($file);
		}
		return false;
	}

	/**
	 * file
	 * @access protected
	 * @param string $key
	 * @return string
	 */
	protected function file($key)
	{
		$md5key = md5($key);
		return $this->_path .'/'. $md5key[0] .'/'. $md5key[1] .'/'. $md5key[2] .'/'. $md5key .'.php';
	}

}
