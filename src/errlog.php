<?php
// errlog
register_shutdown_function(array('Errlog', 'handle'));

/**
 * Errlog
 */
abstract class Errlog
{
	/**
	 * Mail Config
	 */
	protected static $subject = '[Develop]YOD发生错误';
	protected static $send_to = array(
			'zmrnet@qq.com',
		);

	/**
	 * SMTP Config
	 */
	protected static $smtp_config = array(
			'host' => 'smtp.126.com',
			'port' => '25',
			'user' => 'phpdev@126.com',
			'pass' => '******',
			'from' => 'phpdev@126.com',
			'name' => 'phpdev',
		);

	/**
	 * HTTP Filter
	 */
	protected static $http_filter = array(
			'HTTP_USER_AGENT' => array(
				'Baidu-YunGuanCe',
				'Alibaba.Security.Heimdall',
				'Baiduspider',
				'YisouSpider',
				'Googlebot',
				'360Spider',
			),
			'HTTP_X_SCANNER' => array(
				'Netsparker',
			),
			'REQUEST_URI' => array(
				'browserconfig',
				'undefined',
			),
		);
	
	/**
	 * Error Type
	 */
	protected static $errtype = array(
			/* ERROR */
			E_ERROR => 'E_ERROR',
			E_USER_ERROR => 'E_USER_ERROR',
			E_CORE_ERROR => 'E_CORE_ERROR',
			E_COMPILE_ERROR => 'E_COMPILE_ERROR',
			E_RECOVERABLE_ERROR => 'E_RECOVERABLE_ERROR',
			/* WARNING */
			E_WARNING => 'E_WARNING',
			E_USER_WARNING => 'E_USER_WARNING',
			E_CORE_WARNING => 'E_CORE_WARNING',
			E_COMPILE_WARNING => 'E_COMPILE_WARNING',
			/* PARSE */
			E_PARSE => 'E_PARSE',
			/* NOTICE */
			E_NOTICE => 'E_NOTICE',
			E_USER_NOTICE => 'E_USER_NOTICE',
			/* STRICT */
			E_STRICT => 'E_STRICT',
			/* DEPRECATED */
			E_DEPRECATED => 'E_DEPRECATED',
			E_USER_DEPRECATED => 'E_USER_DEPRECATED',
		);

	/**
	 * handle
	 */
	public static function handle()
	{
		if ($error = error_get_last()) {
			$md5key = md5(json_encode($error));
			if (FCache::S($md5key)) {
				return;
			}
			FCache::S($md5key, $error, 1800);
			if (isset(self::$errtype[$error['type']])) {
				$error['type'] = self::$errtype[$error['type']];
			}
			// http_filter
			foreach (Errlog::$http_filter as $key => $rules) {
			 	if (empty($_SERVER[$key])) {
					continue;
				}
				foreach ($rules as $value) {
				 	if (strripos($_SERVER[$key], $value) !== false) {
				 		return;
				 	}
				}
			}
			Errlog::sendlog($error);
		}
	}

	/**
	 * errmsg
	 *
	 * @access public
	 * @param string  $content
	 * @param string  $title
	 * @return mixed
	 */
	public static function errmsg($content, $title = null)
	{
		if (is_scalar($content)) {
			$md5key = md5($content . $title);
		} else {
			$md5key = md5(json_encode($content) . $title);
			if (!is_array($content)) {
				$content = '<pre>' . print_r($content, true) . '</pre>';
			}
		}
		if (FCache::S($md5key)) {
			return;
		}
		FCache::S($md5key, $content, 1800);
		Errlog::sendlog($content, $title);
	}

	/**
	 * sendlog
	 *
	 * @access public
	 * @param string  $content
	 * @param string  $title
	 * @return mixed
	 */
	public static function sendlog($content, $title = null)
	{
		// content
		if (!is_scalar($content)) {
			$content = '<pre>' . print_r($content, true) . '</pre>';
		}
		// title
		if (empty($title)) {
			if (empty($_SERVER['HTTP_HOST'])) {
				$_SERVER['HTTP_HOST'] = isset($_SERVER['HOSTNAME']) ? $_SERVER['HOSTNAME'] : '';
			}
			$title = $_SERVER['HTTP_HOST'] . $_SERVER['PHP_SELF'];
		}
		$envdata = (empty($_POST) ? '' : json_encode($_POST) . "\n\n") . print_r($_SERVER, true);
		$logdata = '<p>' . $title . '</p>' . $content . '<hr><pre>' . $envdata . '</pre>';
		return Errlog::sendmail(Errlog::$send_to, Errlog::$subject, $logdata);
	}

	/**
	 * sendmail
	 *
	 * @access public
	 * @param array 	$email
	 * @param string 	$subject
	 * @param string 	$content
	 * @param string 	$charset
	 * @return mixed
	 */
	public static function sendmail($email, $subject, $content, $charset = 'UTF-8')
	{
		if (empty($email) || empty($subject) || empty($content)) {
			return false;
		}

		$E_LEVEL = error_reporting();
		error_reporting(0);

		$content = '<html><head><meta http-equiv="Content-Type" content="text/html;charset='. $charset .'"></head><body>' . $content . '</body></html>';

		$mailer = new SmtpMailer(self::$smtp_config);
#		$mailer->debug(true);
		$result = $mailer->sendmail($email, $subject, $content, $charset);
#		echo '<pre>'; print_r($mailer->loginfo()); echo '</pre>';

		error_reporting($E_LEVEL);
		return $result;
	}

}

/**
 * SmtpMailer
 */
class SmtpMailer
{
	protected $smtp;
#	protected $sendmail  = '/usr/sbin/sendmail';

	protected $smtp_host = 'localhost';
	protected $smtp_port = '25';
	protected $smtp_auth = true;
	protected $smtp_user = '';
	protected $smtp_pass = '';
	protected $smtp_from = '';
	protected $from_name = '';

	protected $helo    = 'localhost';
	protected $c_type  = 'text/html';
	protected $mail_to = array();
	protected $mail_cc = array();
	protected $timeout = 5;
	protected $errno   = 0;
	protected $errmsg  = '';
	protected $debug   = false;
	protected $loginfo = array();

	public $charset  = 'UTF-8';
	public $subject  = '';
	public $content  = '';

	/**
	 * __construct
	 * @param void
	 * @return void
	 */
	public function __construct($config = null)
	{
		$this->smtp_host = empty($config['host']) ? 'localhost' : $config['host'];
		$this->smtp_port = empty($config['port']) ? '25' : $config['port'];
		$this->smtp_auth = isset($config['auth']) ? $config['auth'] : true;
		$this->smtp_user = empty($config['user']) ? '' : $config['user'];
		$this->smtp_pass = empty($config['pass']) ? '' : $config['pass'];
		$this->smtp_from = empty($config['from']) ? 'root@localhost' : $config['from'];
		$this->from_name = empty($config['name']) ? 'root' : $config['name'];
	}

	/**
	 * 发送邮件
	 *
	 * @access public
	 * @param array 	$email
	 * @param string 	$subject
	 * @param string 	$content
	 * @param string 	$charset
	 * @return mixed
	 */
	public function sendmail($email, $subject, $content, $charset = 'UTF-8')
	{
		if (empty($email)) {
			return false;
		}
		$this->ClearEmail();
		if (is_array($email)) {
			foreach ($email as $value) {
				$this->AddEmail($value);
			}
		} else {
			$this->AddEmail($email);
		}
		$this->charset = $charset;
		$this->subject = $subject;
		$this->content = $content;
		return $this->Send();
	}

	/**
	 * Sets message type to HTML.
	 * @param bool $ishtml
	 * @return void
	 */
	public function IsHTML($ishtml = true)
	{
		if ($ishtml) {
			$this->c_type = 'text/html';
		} else {
			$this->c_type = 'text/plain';
		}
	}

	/**
	 * Add Email Address.
	 * @param string 	$email
	 * @param string 	$uname
	 * @return void
	 */
	public function AddEmail($email, $uname = null)
	{
		$this->mail_to[] = array(
				'email' => $email,
				'uname' => empty($uname) ? strstr($email, '@', true) : $uname,
			);
	}

	/**
	 * Clear Email Address.
	 * @param void
	 * @return void
	 */
	public function ClearEmail()
	{
		$this->mail_to = array();
	}

	/**
	 * Add Cc Email Address.
	 * @param string 	$email
	 * @param string 	$uname
	 * @return void
	 */
	public function AddCc($email, $uname = null)
	{
		$this->mail_cc[] = array(
				'email' => $email,
				'uname' => empty($uname) ? strstr($email, '@', true) : $uname,
			);
	}

	/**
	 * Clear Cc Email Address.
	 * @param void
	 * @return void
	 */
	public function ClearCc()
	{
		$this->mail_cc = array();
	}

	/**
	 * Send Mail.
	 * @param void
	 * @return bool
	 */
	public function Send()
	{
		if (!empty($this->sendmail)) {
			return $this->mail_send();
		}
		if (!$this->smtp_sockopen()) {
			return false;
		}
		$send = $this->smtp_send();
		$this->smtp_close();
		return $send ? true : false;
	}

	/**
	 * Set Timeout.
	 * @param integer 	$timeout
	 * @return void
	 */
	public function timeout($timeout)
	{
		$this->timeout = $timeout;
	}

	/**
	 * Set Debug.
	 * @param boolean 	$debug
	 * @return void
	 */
	public function debug($debug = true)
	{
		$this->debug = $debug;
	}

	/**
	 * Get SMTP Loginfo.
	 * @param void
	 * @return integer
	 */
	public function loginfo()
	{
		return implode('', $this->loginfo);
	}

	/**
	 * Get Error NO.
	 * @param void
	 * @return integer
	 */
	public function errno()
	{
		return $this->errno;
	}

	/**
	 * Get Error Message.
	 * @param void
	 * @return integer
	 */
	public function errmsg()
	{
		return $this->errmsg;
	}

	/**
	 * SMTP头信息
	 */
	protected function smtp_header()
	{
		$header = array();
		$header[] = 'MIME-Version: 1.0';
		$header[] = 'Content-Type: '. $this->c_type;
		$header[] = 'Content-Transfer-Encoding: base64';

		$mail_to = array();
		foreach ($this->mail_to as $rcpt_to) {
			$mail_to[] = $rcpt_to['uname'] .'<'. $rcpt_to['email'] .'>';
		}
		$header[] = 'To: '. implode(';', $mail_to);

		if (!empty($this->mail_cc)) {
			$mail_cc = array();
			foreach ($this->mail_cc as $rcpt_to) {
				$mail_cc[] = $rcpt_to['uname'] .'<'. $rcpt_to['email'] .'>';
			}
			$header[] = 'Cc: '. implode(';', $mail_cc);
		}

		$header[] = 'From: ' . $this->from_name .'<'. $this->smtp_from .'>';
		$header[] = 'Subject: =?'. $this->charset .'?B?' . base64_encode($this->subject) . '?=';
		$header[] = 'Date: ' . date('r');
		$header[] = 'X-Mailer: By SmtpMailer (PHP/' . phpversion() . ')';
		
		list($msec, $sec) = explode(' ', microtime());
		$header[] = 'Message-ID: <' . date('YmdHis', $sec) . '.' . ($msec*1000000) . '.' . $this->smtp_from;
		
		return implode("\r\n", $header) . "\r\n";
	}

	/**
	 * 发送邮件
	 */
	protected function smtp_send()
	{
		// HELO
		if (!$this->smtp_putcmd('HELO', $this->helo)) {
			$this->errmsg = 'Error: Error occurred while sending HELO command';
			$this->errno = 101;
			return false;
		}

		// AUTH LOGIN
		if ($this->smtp_auth) {
			if (!$this->smtp_putcmd('AUTH LOGIN', base64_encode($this->smtp_user))) {
				$this->errmsg = 'Error: Error occurred while sending AUTH LOGIN command';
				$this->errno = 102;
				return false;
			}
			if (!$this->smtp_putcmd('', base64_encode($this->smtp_pass))) {
				$this->errmsg = 'Error: Error occurred while sending AUTH LOGIN command';
				$this->errno = 103;
				return false;
			}
		}

		// MAIL FROM
		if (!$this->smtp_putcmd('MAIL', 'FROM:<' . $this->smtp_from . '>')) {
			$this->errmsg = 'Error: Error occurred while sending MAIL FROM command';
			$this->errno = 104;
			return false;
		}

		// RCPT TO
		foreach ($this->mail_to as $rcpt_to) {
			if (!$this->smtp_putcmd('RCPT', 'TO:<'. $rcpt_to['email'] .'>')) {
				$this->errmsg = 'Error: Error occurred while sending RCPT TO command';
				$this->errno = 105;
				return false;
			}
		}

		// DATA
		if (!$this->smtp_putcmd('DATA')) {
			$this->errmsg = 'Error: Error occurred while sending DATA command';
			$this->errno = 106;
			return false;
		}

		// message
		if (!$this->smtp_message()) {
			$this->errmsg = 'Error: Error occurred while sending message';
			$this->errno = 107;
			return false;
		}

		// [EOM]
		if (!$this->smtp_eom()) {
			$this->errmsg = 'Error: Error occurred while sending <CR><LF>.<CR><LF> [EOM]';
			$this->errno = 108;
			return false;
		}

		// QUIT
		if (!$this->smtp_putcmd('QUIT')) {
			$this->errmsg = 'Error: Error occurred while sending QUIT command';
			$this->errno = 109;
			return false;
		}

		return true;
	}

	/**
	 * 连接SMTP
	 */
	protected function smtp_sockopen()
	{
		$this->smtp = @fsockopen(
				$this->smtp_host,
				$this->smtp_port,
				$this->errno,
				$this->errmsg,
				$this->timeout
			);
		if ($this->smtp && $this->smtp_ok()) {
			return true;
		}
		$this->smtp = null;
		return false;
	}

	/**
	 * 关闭SMTP连接
	 */
	protected function smtp_close()
	{
		if ($this->smtp) {
			fclose($this->smtp);
		}
	}

	/**
	 * 发送消息内容
	 */
	protected function smtp_message()
	{
		$message = $this->smtp_header() . "\r\n" . base64_encode($this->content);
		fputs($this->smtp, $message);
		if ($this->debug) {
			$this->loginfo[] = $message;
		}
		return TRUE;
	}

	/**
	 * 发送SMTP命令
	 */
	protected function smtp_putcmd($cmd, $arg = '')
	{
		if ($arg != '') {
			if ($cmd == '') {
				$cmd = $arg;
			} else {
				$cmd = $cmd . ' ' . $arg;
			}
		}
		fputs($this->smtp, $cmd . "\r\n");
		if ($this->debug) {
			$this->loginfo[] = $cmd . "\r\n";
		}
		return $this->smtp_ok();
	}

	/**
	 * 消息结束命令
	 */
	protected function smtp_eom()
	{
		fputs($this->smtp, "\r\n.\r\n");
		if ($this->debug) {
			$this->loginfo[] = "\r\n.\r\n";
		}
		return $this->smtp_ok();
	}

	/**
	 * 监测SMTP连接
	 */
	protected function smtp_ok()
	{
		$data = fgets($this->smtp, 512);
		if ($this->debug) {
			$this->loginfo[] = $data;
		}
		$data = str_replace("\r\n", '', $data);
		if (!preg_match('/^[23]/', $data)) {
			fputs($this->smtp, "QUIT\r\n");
			fgets($this->smtp, 512);
			return false;
		}
		return true;
	}

	/**
	 * 使用sendmail发送
	 */
	protected function mail_send()
	{
		$sendmail = sprintf('%s -oi -t', escapeshellcmd($this->sendmail));
		if($handle = @popen($sendmail, 'w')) {
			$message = $this->smtp_header() . "\r\n" . base64_encode($this->content);

			fputs($handle, $message);
			$this->loginfo[] = $message;

			return (pclose($handle) == 0);
		}
		$this->loginfo[] = 'Failure';
		return false;
	}

} 

/**
 * FCache
 */
class FCache
{
	protected static $_cache;

	protected $_path;

	/**
	 * __construct
	 * @access public
	 * @param mixed $config
	 * @return void
	 */
	public function __construct($config = null)
	{
		$this->_path = empty($config['path']) ? dirname(__DIR__) . '/data/fcache/errlog' : $config['path'];
		is_dir($this->_path) or mkdir($this->_path, 0755, true);
	}

	/**
	 * S
	 * @access public
	 * @param string $key
	 * @param mixed $var
	 * @param integer $expire
	 * @return mixed
	 */
	public static function S($key, $var = null, $expire = 3600)
	{
		if (empty($key)) {
			return null;
		}
		if (!is_object(self::$_cache)) {
			self::$_cache = new static();
		}
		$num = func_num_args();
		if ($num == 1) {
			return self::$_cache->get($key);
		} elseif ($num > 1) {
			return self::$_cache->set($key, $var, $expire);
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
	public function set($key, $var, $expire = 3600)
	{
		$file = $this->file($key);
		$path = dirname($file);
		is_dir($path) or mkdir($path, 0755, true);
		$expire = $expire ? $expire + time() : 0;
		$data = var_export(array('var' => $var, 'exp' => $expire), true);
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
	 * delete
	 * @access public
	 * @param string $key
	 * @return boolean
	 */
	public function delete($key)
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
		return $this->_path . '/' .
				$md5key[0] . $md5key[1] . '/' .
				$md5key[2] . $md5key[3] . '/' .
				$md5key[4] . $md5key[5] . '/' .
				$md5key .'.php';
	}

}
