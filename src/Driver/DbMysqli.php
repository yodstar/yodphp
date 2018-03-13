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

class Yod_DbMysqli extends Yod_Database
{
	protected $_errno;
	protected $_error;

	/**
	 * __construct
	 * @access public
	 * @return void
	 */
	public function __construct($dbconfig)
	{
		parent::__construct($dbconfig);

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
			return $this->_linkid = $this->_linkids[$linknum];
		}
		if (empty($config['dbname'])) {
			trigger_error('Mysqli DSN configure error', E_USER_ERROR);
			return false;
		}
		$config['host'] = empty($config['host']) ? 'localhost' : $config['host'];
		$config['user'] = empty($config['user']) ? '' : $config['user'];
		$config['pass'] = empty($config['pass']) ? '' : $config['pass'];
		$config['port'] = empty($config['port']) ? 3306 : intval($config['port']);
		$config['charset'] = empty($config['charset']) ? 'utf8' : $config['charset'];
		$this->_linkids[$linknum] = new mysqli($config['host'], $config['user'], $config['pass'], $config['dbname'], $config['port']);
		if (mysqli_connect_errno()) {
			trigger_error(mysqli_connect_error(), E_USER_ERROR);
			return false;
		}
		$dbversion = $this->_linkids[$linknum]->server_version;
		$this->_linkids[$linknum]->query("SET NAMES '{$config['charset']}'");
		if($dbversion >'5.0.1'){
			$this->_linkids[$linknum]->query("SET sql_mode=''");
		}
		mysqli_report(MYSQLI_REPORT_ERROR);
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
	public function execute($query, array $params = array(), $affected = false)
	{
		$this->connect($this->_config, 0);

		$this->_lastquery = $query;
		if (empty($params) || is_bool($params)) {
			if ($this->_linkid->query($query)) {
				if (is_bool($params)) {
					$affected = $params;
				}
				return $affected ? $this->_linkid->affected_rows : true;
			}
		} else {
			if ($mysqli_stmt = $this->_linkid->prepare($query)) {
				if (count($params) > 1) {
					array_unshift($params, 's');
					call_user_func_array(array($mysqli_stmt, 'bind_param'), $params);
				}
				if ($mysqli_stmt->execute()) {
					if ($affected) {
						$retval = $mysqli_stmt->affected_rows;
					} else {
						$retval = true;
					}
					$mysqli_stmt->close();
					return $retval;

				}
				$this->_errno = $mysqli_stmt->errno;
				$this->_error = $mysqli_stmt->error;
				$mysqli_stmt->close();
			}
		}
		
		return false;
	}

	/**
	 * query
	 * @access public
	 * @return mixed
	 */
	public function query($query, array $params = array())
	{
		$this->connect($this->_config, 1);

		$this->_lastquery = $query;
		if (empty($params)){
			return $this->_result = $this->_linkid->query($query);
		} else {
			if ($mysqli_stmt = $this->_linkid->prepare($query)) {
				if (count($params) > 1) {
					array_unshift($params, 's');
					call_user_func_array(array($mysqli_stmt, 'bind_param'), $params);
				}
				if ($mysqli_stmt->execute()) {
					if ($result = $mysqli_stmt->get_result()) {
						$this->_result = $result;
					} else {
						$result = $mysqli_stmt->affected_rows;
					}
					$mysqli_stmt->close();
					return $result;
				}
				$this->_errno = $mysqli_stmt->errno();
				$this->_error = $mysqli_stmt->error();
				$mysqli_stmt->close();
			}
		}

		return false;
	}

	/**
	 * count
	 * @access public
	 * @return mixed
	 */
	public function count($result = null)
	{
		if (is_null($result)) {
			$result = $this->_result;
		}
		if ($result instanceof mysqli_result) {
			return $result->num_rows;
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
		if ($result instanceof mysqli_result) {
			return $result->fetch_assoc();
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
		if ($result instanceof mysqli_result) {
			if (method_exists('mysqli_result', 'fetch_all')) {
				return $result->fetch_all(MYSQLI_ASSOC);
			}
			$data = array();
			while ($fetch = $result->fetch_assoc()){
				$data[] = $fetch;
			}
			return $data;		
		}
		return false;
	}

	/**
	 * free
	 * @access public
	 * @return mixed
	 */
	public function free($result = null)
	{
		if (is_null($result)) {
			$result = $this->_result;
		}
		if ($result instanceof mysqli_result) {
			$result->free();
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
				$linkid->close();
				unset($this->_linkids[$key]);
			}
		}
		if ($this->_linkid) {
			$this->_linkid = null;
		}
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
		if (method_exists('mysqli', 'begin_transaction')) {
			return $this->_linkid->begin_transaction();
		} else {
			return $this->_linkid->query('START TRANSACTION');
		}
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
		if (method_exists('mysqli', 'commit')) {
			return $this->_linkid->commit();
		} else {
			return $this->_linkid->query('COMMIT');
		}
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
		if (method_exists('mysqli', 'rollback')) {
			return $this->_linkid->rollback();
		} else {
			return $this->_linkid->query('ROLLBACK');
		}
	}

	/**
	 * insertId
	 * @access public
	 * @return integer
	 */
	public function insertId()
	{
		return $this->_linkid->insert_id;
	}

	/**
	 * quote
	 * @access public
	 * @return string
	 */
	public function quote($string)
	{
		$quote = $this->_linkid->real_escape_string($string);
		return "'{$quote}'";
	}

	/**
	 * errno
	 * @access public
	 * @return mixed
	 */
	public function errno()
	{
		if ($this->_errno) {
			return $this->_errno;
		} elseif ($this->_linkid) {
			return $this->_linkid->errno;
		} else {
				return false;
		}
	}

	/**
	 * errer
	 * @access public
	 * @return mixed
	 */
	public function error()
	{
		if ($this->_error) {
			return $this->_error;
		} elseif ($this->_linkid) {
			return $this->_linkid->error;
		} else {
				return false;
		}
	}

}
