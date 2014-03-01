<?php
// +----------------------------------------------------------------------
// | yodphp [ Yod PHP Framework ]
// +----------------------------------------------------------------------
// | Copyright (c) 2013 http://yodphp.duapp.com All rights reserved.
// +----------------------------------------------------------------------
// | Licensed ( http://www.apache.org/licenses/LICENSE-2.0 )
// +----------------------------------------------------------------------
// | Author: Baoqiang Su <zmrnet@qq.com>
// +----------------------------------------------------------------------

class Yod_DbMysql extends Yod_Database
{
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
	public function connect($config = null, $linknum = 0)
	{
		$config = $this->dbconfig($config, $linknum);
		$linknum = isset($config['linknum']) ? $config['linknum'] : 0;
		if (isset($this->_linkids[$linknum])) {
			return $this->_linkid = $this->_linkids[$linknum];
		}
		if (empty($config['dbname'])) {
			trigger_error('Database DSN configure error', E_USER_ERROR);
			return false;
		}
		$config['host'] = empty($config['host']) ? 'localhost' : $config['host'];
		$config['user'] = empty($config['user']) ? '' : $config['user'];
		$config['pass'] = empty($config['pass']) ? '' : $config['pass'];
		$config['port'] = empty($config['port']) ? 3306 : intval($config['port']);
		$config['charset'] = empty($config['charset']) ? 'utf8' : $config['charset'];

		$server = $config['host'] . (empty($config['port']) ? '' : ":{$config['port']}");
		if (empty($config['pconnect'])) {
			$this->_linkids[$linknum] = mysql_connect($server, $config['user'], $config['pass']);
		} else {
			$this->_linkids[$linknum] = mysql_pconnect($server, $config['user'], $config['pass']);
		}
		if (!$this->_linkids[$linknum] || (!mysql_select_db($config['dbname'], $this->_linkids[$linknum])) ) {
			trigger_error(mysql_error(), E_USER_ERROR);
			return false;
		}
		$dbversion = mysql_get_server_info($this->_linkids[$linknum]);
		mysql_query("SET NAMES '{$config['charset']}'", $this->_linkids[$linknum]);
		if($dbversion >'5.0.1'){
			mysql_query("SET sql_mode=''", $this->_linkids[$linknum]);
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
		if (!empty($params) && is_array($params)) {
			foreach ($params as $key => $value) {
				if (strstr($query, $key)) {
					$value = mysql_real_escape_string($value);
					$query = str_replace($key, "'{$value}'", $query);
				}
			}
		}
		if (mysql_query($query, $this->_linkid)) {
			if (is_bool($params)) {
				$affected = $params;
			}
			return $affected ? mysql_affected_rows($this->_linkid) : true;
		}

		if ($error = mysql_error($this->_linkid)) {
			trigger_error($error, E_USER_WARNING);
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
		if (!empty($params)) {
			foreach ($params as $key => $value) {
				if (strstr($query, $key)) {
					$value = mysql_real_escape_string($value);
					$query = str_replace($key, "'{$value}'", $query);
				}
			}
		}
		if ($result = mysql_query($query, $this->_linkid)) {
			if (is_resource($result)) {
				$this->_result = $result;
			} else {
				$result = mysql_affected_rows($this->_linkid);
			}
			return $result;
		}
		if ($error = mysql_error($this->_linkid)) {
			trigger_error($error, E_USER_WARNING);
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
		return $result ? mysql_fetch_assoc($result) : false;
	}

	/**
	 * fetchAll
	 * @access public
	 * @return mixed
	 */
	public function fetchAll($result = null)
	{
		$data = array();
		while ($fetch = $this->fetch($result)){
			$data[] = $fetch;
		}
		return $data;
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
		return $result ? mysql_free_result($result) : false;
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
				mysql_close($linkid);
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
		if (mysql_query('START TRANSACTION', $this->_linkid)) {
			return true;
		}
		if ($error = mysql_error($this->_linkid)) {
			trigger_error($error, E_USER_WARNING);
		}
		return false;
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
		if (mysql_query('COMMIT', $this->_linkid)) {
			return true;
		}
		if ($error = mysql_error($this->_linkid)) {
			trigger_error($error, E_USER_WARNING);
		}
		return false;
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
		if (mysql_query('ROLLBACK', $this->_linkid)) {
			return true;
		}
		if ($error = mysql_error($this->_linkid)) {
			trigger_error($error, E_USER_WARNING);
		}
		return false;
	}

	/**
	 * insertid
	 * @access public
	 * @return integer
	 */
	public function insertid()
	{
		return mysql_insert_id($this->_linkid);
	}

	/**
	 * quote
	 * @access public
	 * @return string
	 */
	public function quote($string)
	{
		$quote = mysql_real_escape_string($string, $this->_linkid);
		return "'{$quote}'";
	}

	/**
	 * errno
	 * @access public
	 * @return mixed
	 */
	public function errno()
	{
		if ($this->_linkid) {
			return mysql_errno($this->_linkid);
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
		if ($this->_linkid) {
			return mysql_error($this->_linkid);
		} else {
			return false;
		}
	}
}