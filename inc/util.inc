

<?php

register_shutdown_function("before_exit");
require_once("constants.inc");
require_once("gtcm_gnp_class.inc");
require_once("exception.inc");
require_once("gde.inc");

//these consants control how errors are logged and reported, if not defined already, they should be defined here
if(!defined("SILENT"))	
	define("SILENT", FALSE);		// by default, show error message to the stdout, or browser
if(!defined("LOG_FATAL_ONLY"))
	define("LOG_FATAL_ONLY", FALSE);	// by default, all errors are logged

?>

<?php
/* recommended approach: use throwit to generate or pass an m_exception, use catchit in combinatioin with switches
*  to process errors
*/
function &throwit($file, $line, $func, $level, $msg = "see previous", $server = FALSE)
{	
	$err = new m_exception($file, $line, $func, $msg, $level, $server);
	return $err;
}

/* action: if !LOG_FATAL_ONLY & !SILENT, all m_exception are logged and reported
 *	   elseif LOG_FATAL_ONLY & SILENT, only fatal m_exceptions are logged, nothing is reported to the screen
 *	   elseif LOG_FATAL_ONLY & !SILENT, all m_exceptions are reported to the screen, but only fatal ones are logged
 *	   elseif !LOG_FATAL_ONLY & SILENT, all m_exceptions are logged, but none reported to the screen
 *	   All fatal errors result in termination of the script. The registered shutdown function makes sure the database is sent
 * 	   a final TERMINATE message. 
 */
function catchit($m_exception = null)
{ 
	if (is_null($m_exception)) return FALSE;	
	if ((!is_object($m_exception)) ||
		 (get_class($m_exception) != "m_exception")) return FALSE;		//ignore non-exception arguments
	if(!LOG_FATAL_ONLY)
		$m_exception->log();
	else
	{
		if($m_exception->isFatal())
			$m_exception->log();
	}

	if(!SILENT)
	{
		if ($m_exception->isFatal()) 
			exit("Fatal error " . $m_exception->toString() . "<br>\nAlso see " . LOGFILE . "<br>\n");
		else 
		{
			echo "Error: " . $m_exception->toString() . "<br>\n";
			if (!LOG_FATAL_ONLY)
				echo "Also see " . LOGFILE . "<br>\n";
		}
	}
	else
	{
		if ($m_exception->isFatal())
			exit();
	}

	return TRUE;
}


function before_exit()
{
	foreach (GNP_DIRECTORY::$instances as $key => $val)
	{
		if (get_parent_class(GNP_DIRECTORY::$instances[$key]) == "GTCM_GNP")
			@$val->destroy(); 
	}
}

require_once("gnp_lock_manager_class.inc");
?>
