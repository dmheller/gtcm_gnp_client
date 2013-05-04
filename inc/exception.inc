

<?php

require_once("constants.inc");

?>

<?php

/* Usage: $this->m_info = new m_exception(...); if FATAL, exit() 
 * Note:	Only errors identified as GTM-F- are considered fatal, which causes the script to terminate 
 */
class m_exception
{
	private $m_id;		//unix time stamp
	private $m_file;		//FILE
	private $m_line;		//LINE
	private $m_func;		//function where error occurred
	private$m_desc;		//error description, human readable
	private $m_is_fatal;	
	private $m_is_server;
	private $m_level;		//new errors have level 0, enclosing function has level 1, etc.

	#################################
	#	PUBLIC FUNCTIONS	#
	#################################

public	function __construct($file, $line, $func, $msg, $level, $is_server = FALSE)
	{
		$this->m_id = $this->makeID();
		$this->m_file = $file;
		$this->m_line = $line;
		$this->m_func = $func;
		$this->m_level = $level;
		$this->m_is_fatal = $this->fatal($msg);
		$this->m_is_server = $is_server;
		if($level == 0)
		$this->m_desc = "<br>\nNew exception: ";
		$this->m_desc .= ($this->m_is_fatal?"<br>\nFatal ":"<br>\nNon-fatal ");
		$this->m_desc .= ($is_server?"server-side error":"client-side error ");
		$this->m_desc .= "in function: $func() on line: $line in file: $file. <br>\n";
		$this->m_desc .= "Details: " . $msg . "<br>\n";
	}

	/* make sure error.log has write perssion set for normal users: chmod a+w error.log */
public	function log()		
	{
		/* dump the error context into the log file first */
		if ($fp = fopen(LOGFILE, 'a'))
		{
			fputs($fp, "\n\n");
			fputs($fp, str_repeat("=", 35));
			fputs($fp, $this->m_id);
			fputs($fp, str_repeat("=", 35));
			fputs($fp, "\n");
			fwrite($fp, $this->m_desc, strlen($this->m_desc));
	
			$msg = ($this->m_is_fatal?"Fatal error, script aborted.\n":"Error, script may still run.\n");
			fwrite($fp, $msg, strlen($msg));
			fputs($fp, str_repeat("=", 96));
			fclose($fp);
		}	
		else
		{
			$this->m_desc = $this->m_id . "--" . $this->m_desc;
			if($this->m_is_fatal)
			{
				echo $this->m_desc . "<br>\nIrrecoverable error occurred, but " . 
						"LOGFILE " . LOGFILE . " cannot be opened.";
			}
			else
			{
				echo $this->m_desc . "<br>\nLOGFILE " . LOGFILE . "cannot be opened."; 
			}
		}
	}

public	function toString()
	{
		return $this->m_desc;
	}

public	function isFatal()
	{
		return $this->m_is_fatal;
	}

public	function getLevel()
	{
		return $this->m_level;
	}
	#################################
	#	PRIVATE FUNCTIONS	#
	#################################

private	function makeID()
	{
		$time = getdate(time());
		$id = $time['hours'] . ":" . $time['minutes'] . ":" . $time['seconds'] . " " . $time['weekday']
				. " " . $time['month'] . " " . $time['mday'] . ", " . $time['year']; 
		return $id;
	}

	/* used to identify messages that indicate fatal error */
private	function fatal($message)
	{
		if(preg_match("/GTM-F-/i", $message) || preg_match("/PHP-F-/i", $message))
			return TRUE; 	
		/* all others are not fatal */
		return FALSE;
	}

}



?>
