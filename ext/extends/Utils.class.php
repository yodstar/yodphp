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

abstract class Yod_Utils
{

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
