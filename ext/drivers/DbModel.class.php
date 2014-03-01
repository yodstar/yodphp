<?php

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
	 * @return void
	 */
	public function __construct($name = '', $config = '')
	{
		parent::__construct($name, $config);

		$this->initQuery();

		self::$_model[$name] = $this;
	}

	/**
	 * getInstance
	 * @access public
	 * @param string	$name
	 * @param mixed		$config
	 * @return Yod_DbModel
	 */
	public static function getInstance($name, $config = '')
	{
		if (empty($name)) {
			$classname = 'Yod_DbModel';
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
	 * @param string	$where
	 * @param array		$params
	 * @param mixed		$select
	 * @return mixed
	 */
	public function find($where = '', $params = array(), $select = '*')
	{
		$query = $this->select($select)->where($where, $params)->limit('1')->parseQuery();
		if ($result = $this->_db->query($query, $this->_params)) {
			$data = $this->_db->fetch($result);
			$this->_db->free($result);
			$this->initQuery();
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
		$query = $this->select($select)->where($where, $params)->parseQuery();
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
		$query = $this->select('COUNT(*)')->where($where, $params)->parseQuery();
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
	 * @return boolean
	 */
	public function save($data, $where = '', $params = array())
	{
		if (empty($data)) return false;
		if (empty($this->_table)) {
			trigger_error('You have an error in your SQL syntax: table name is empty', E_USER_WARNING);
			return false;
		}
		$this->where($where, $params);
		if (empty($this->_query['WHERE'])) {
			$result = $this->_db->insert($data, $this->_table);
		} else {
			$result = $this->_db->update($data, $this->_table, $this->_query['WHERE'], $this->_params);
		}
		$this->initQuery();
		return $result;
	}

	/**
	 * remove
	 * @access public
	 * @return boolean
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
	 * select
	 * @access public
	 * @return Yod_DbModel
	 */
	public function select($select)
	{
		if (is_array($select)) {
			foreach ($select as $key => $value) {
				if (is_string($key)) {
					$select[$key] = "{$key} AS {$value}"; 
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
	 * @return Yod_DbModel
	 */
	public function join($table, $where = '', $mode = 'LEFT')
	{
		$join = count($this->_query['JOIN']) + 2;
		$this->_query['JOIN'][] = "{$mode} JOIN {$this->_prefix}{$table} AS t{$join}". (empty($while) ? '' : " ON {$where}");
		return $this;
	}

	/**
	 * where
	 * @access public
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
	 * @return Yod_DbModel
	 */
	public function union($union)
	{
		$this->_query['UNION'] = $union;
		return $this;
	}

	/**
	 * comment
	 * @access public
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
	 * @access protected
	 * @return mixed
	 */
	protected function parseQuery($query = null)
	{
		$parse = array();
		$query = empty($query) ? $this->_query : $query;
		if (empty($query['FROM'])) {
			if (empty($this->_table)) {
				trigger_error('You have an error in your SQL syntax: table name is empty', E_USER_WARNING);
				return false;
			}
			$query['FROM'] = "{$this->_prefix}{$this->_table} AS t1";
		}
		foreach ($query as $key => $value) {
			if (empty($value)) {
				continue;
			}
			if (is_array($value)) {
				if ($key == 'UNION') {
					$value = $this->parseQuery($value);
				} elseif ($key == 'JOIN') {
					$value = implode(' ', $value);
					$key = '';
				}
			}
			if ($key == 'COMMENT') {
				$value = '/* ' . $value . ' */';
				$key = '';
			}
			$parse[] = $key . ' ' . $value;
		}

		return implode(' ', $parse);
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
			'UNION' => '',
			'COMMENT' => '',
		);
		$this->_params = array();

		return $this;
	}

}
