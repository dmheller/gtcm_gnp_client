<?php

require("tcp_socket.inc");
require_once("util.inc");
require_once("gnp_misc.inc");
require("gnp_buffer.inc");

?>

<?php

class GTCM_GNP
{
	#################################
	#	DATA MEMBERS		#
	#################################

	/* User provided */
	private $m_server; 		//must always be a string, provided by user
	private $m_port;   		//must always be a number, provided by user
	private $m_dat_file;	//database file name, may be on a remote machine, format???, provided by user
	private $m_name;
	/* GTCM_GNP provided */
	private $m_socket;		//the socket over which communication is taking place
	private $m_buffer;		//deals with message formatting and send/receive issues
	private $m_proc_num;	//process number returned from INITPROC call
	private $m_open_region;	//REGION INFO of the current region
	private $m_cur_region_no;	//number of region currently in use, member of REGION_INFO type, returned from INITREG call
	static $instances = array();

	#################################
	#	PUBLIC FUNCTIONS	#
	#################################

	/* Constructor
	*  action:	initialize class data members
	*  usage: 	$obj = new GTCM_GNP()
	*  notes:	requires $dat_file as a string correctly formatted to suit the platform
	*/
public	function __construct($name, $dat_file, $server = DEFAULT_SERVER, $port = DEFAULT_PORT)
	{ 
		if(empty($dat_file))
		{
			return FALSE;
		}
		
		$socket = new TCP_SOCKET($server, $port); 
		if(!$socket) 
			return FALSE;
		$buffer = new GNP_BUFFER($socket);
		if(!$buffer)
			return FALSE;
		if(!$proc_num = $buffer->doop(CMMS_S_INITPROC))
			return FALSE;
		if(!$new_region = $buffer->doop(CMMS_S_INITREG, $dat_file))
			return FALSE;
		$this->m_socket = $socket;
		$this->m_buffer = $buffer;
		$this->m_dat_file = $dat_file;
		$this->m_server = $server;
		if(defined("SHOW_SERVER"))
			echo "<br>\nServer: ", $this->m_server, "<br>\n";
		$this->m_port = $port;
		$this->m_proc_num = $proc_num;
		$this->m_cur_region_no = $new_region->m_region_no;
		$this->m_open_region = $new_region;
		$this->m_name = $name;
		GNP_DIRECTORY::$instances[] = $this;
		self::$instances[] = $this;
	}
public   function get_name()
	{
	return $this->m_name;
	}
	
	
	/* GET
	*  action:	parse input, instantiate a SUBSC object and call buffer doop function
	*  		return the value if successful
			return OPERATION_FAILURE if other non-fatal error occurred.
	*  usage:	$value = $this->GET(array of name and subscripts)
	*  note:	make sure the correct region is being used
	*/
public	function GET($name_array)
	{
		$num_args = func_num_args();
		if($num_args < 1)
			return $this->argError(1, "GET", __LINE__);
	if(catchit($value = $this->m_buffer->doop(CMMS_Q_GET, $this->makeSUBSC($name_array))))
	  return OPERATION_FAILURE;
		return $value;		
	}

	/* ORDER
	*  action:	parse input, instantiate a SUBSC object and call buffer doop function
	*  		return the value if successful
			return OPERATION_FAILURE if other non-fatal error occurred.
	*  usage:	$value = $this->ORDER(array of name and subscripts)
	*/
public	function ORDER($name_array)
	{
	      $num_args = func_num_args();
		if($num_args < 1)
			return $this->argError(1, "ORDER", __LINE__);	
		    if(catchit($value = $this->m_buffer->doop(CMMS_Q_ORDER, $this->makeSUBSC($name_array))))
			return OPERATION_FAILURE;
			return $value;	
		
	}

	/* PREV
	*  action:	parse input, instantiate a SUBSC object and call buffer doop function
	*  		return the value if successful
			return OPERATION_FAILURE if other non-fatal error occurred.
	*  usage:	$value = $this->PREV(array of name ans subscripts)
	*/
public	function PREV($name_array)
	{
		$num_args = func_num_args();
		if($num_args < 1)
			return $this->argError(1, "PREV", __LINE__);
		if(catchit($value = $this->m_buffer->doop(CMMS_Q_PREV, $this->makeSUBSC($name_array))))
			return OPERATION_FAILURE;
			return $value;	
	}

	/* DATA
	*  action:	parse input, instantiate a SUBSC object and call buffer doop function
	*  		return the value if successful
			return OPERATION_FAILURE if other non-fatal error occurred.
	*  usage:	$value = $this->DATA(array of name and subscripts)
	*  note:	make sure the correct region is being used
	*/
public	function DATA($name_array)
	{
		$num_args = func_num_args();
		if($num_args < 1)
			return $this->argError(1, "DATA", __LINE__);
		if(catchit($value = $this->m_buffer->doop(CMMS_Q_DATA, $this->makeSUBSC($name_array))))
			return OPERATION_FAILURE;
			return $value;			
	}

	/* KILL
	*  action:	parse input, instantiate a SUBSC object and call buffer doop function
	*  		return TRUE if successful
			return OPERATION_FAILURE if other non-fatal error occurred.
	*  usage:	$value = $this->KILL(array of name and subscripts)
	*  note:	make sure the correct region is being used
	*/
public	function KILL($name_array)
	{
		$num_args = func_num_args();
		if($num_args < 1)
			return $this->argError(1, "KILL", __LINE__);		
	
		if(catchit($value = $this->m_buffer->doop(CMMS_Q_KILL, $this->makeSUBSC($name_array))))
			return OPERATION_FAILURE;
			return $value;			
	}

	/* PUT
	*  action:	parse input, instantiate a SUBSC object and call buffer doop function
	*  		return TRUE if successful
			return OPERATION_FAILURE if other non-fatal error occurred.
	*  usage:	$value = $this->PUT($val, array of name and subscripts)
	*  note:	make sure the correct region is being used
	*/
public	function PUT($name_array, $val)
	{
		$num_args = func_num_args();
		if($num_args < 2)
			return $this->argError(2, "PUT", __LINE__);
	
		if(catchit($value = $this->m_buffer->doop(CMMS_Q_PUT, $this->makeSUBSC($name_array), $val)))
			return OPERATION_FAILURE;
		return $value;			
	}

	/* QUERY
	*  action:	parse input, instantiate a SUBSC object and call buffer doop function
	*  		return the value if successful
			return OPERATION_FAILURE if other non-fatal error occurred.
	*  usage:	$value = $this->QUERY(array of name and subscripts)
	*/
public	function QUERY($name_array)
	{
		$num_args = func_num_args();
		if($num_args < 1)
			return $this->argError(1, "QUERY", __LINE__);
          
		if(catchit($value = $this->m_buffer->doop(CMMS_Q_QUERY, $this->makeSUBSC($name_array))))
			return OPERATION_FAILURE;
			return $value;			
	}

	/* WITHDRAW
	*  action:	parse input, instantiate a SUBSC object and call buffer doop function
	*  		return TRUE if successful
			return OPERATION_FAILURE if other non-fatal error occurred.
	*  usage:	$value = $this->WITHDRAW(array of name and subscripts)
	*  note:	make sure the correct region is being used
	*/
public	function WITHDRAW($name_array)
	{
		$num_args = func_num_args();
		if($num_args < 1)
			return $this->argError(1, "WITHDRAW", __LINE__);
		if(catchit($value = $this->m_buffer->doop(CMMS_Q_ZWITHDRAW, $this->makeSUBSC($name_array))))
			return OPERATION_FAILURE;
		return $value;			
	}
	
	/* TERMINATE
	*  action:	call buffer doop function
	*  		return TRUE if successful
	*  usage:	$value = $this->TERMINATE()
	*  note:	no response from the server is expected
	*/
public	function TERMINATE()
	{
		
		if(catchit($value = $this->m_buffer->doop(CMMS_S_TERMINATE)))
			return OPERATION_FAILURE;
		return TRUE;
	}

	/* DESTROY 	(as destructor, but only to be called, if before_exit is not registered shutdown function)
	*  action:	send TERMINATE message,	close the TCP socket, unset all data members
	*  usage:	$this->destroy()
	*/
public	function destroy()
	{
		$this->TERMINATE();
		$this->cleanUp();
	}

	#################################################################################
	#	LOCK FUNCTIONS	-- THESE SHOULD NORMALLY ONLY BE USED BY LOCK MANAGER 	#
	#################################################################################

	/* LKREQUEST
	*  action:	parse input, instantiate an array of SUBSC object and call buffer doop function
	*  		return CMMS_M_LKGRANTED or CMMS_M_LKBLOCKED message if successful
	*  usage:	$value = $this->LKREQUEST($laflag, $transnum, aray of array of name and subscripts)
	*  note:	make sure the correct region is being used
	*/
public	function LKREQUEST($laflag, $transnum, $name_array_array)
	{
		$num_args = func_num_args();
		if($num_args < 3)
			return $this->argError(3, "LKREQUEST", __LINE__);
		
		if(catchit($state = $this->m_buffer->doop(CMMS_L_LKREQUEST, $laflag, $transnum, $this->makeSUBLIST($name_array_array)))) 
	    return OPERATION_FAILURE;
	          return $state;			
	}

	/* LKREQNODE
	*  action:	parse input, instantiate an array of SUBSC object and call buffer doop function
	*  		return TRUE if successful
	*  usage:	$value = $this->LKREQNODE($laflag, $transnum, aray of array of name and subscripts)
	*  note:	make sure the correct region is being used
	*/
public	function LKREQNODE($laflag, $transnum, $name_array_array)
	{
		$num_args = func_num_args();
		if($num_args < 3)
			return $this->argError(3, "LKREQNODE", __LINE__);
		if(catchit($state = $this->m_buffer->doop(CMMS_L_LKREQNODE, $laflag, $transnum, $this->makeSUBLIST($name_array_array))))
		        return OPERATION_FAILURE;
		        return TRUE;
	}

	/* LKREQIMMED
	*  action:	parse input, instantiate an array of SUBSC object and call buffer doop function
	*  		return CMMS_M_LKGRANTED or CMMS_M_LKABORT message if successful
	*  usage:	$value = $this->LKREQIMMED($laflag, $transnum, aray of array of name and subscripts)
	*  note:	make sure the correct region is being used
	*/
public	function LKREQIMMED($laflag, $transnum, $name_array_array)
	{
		$num_args = func_num_args();
		if($num_args < 3)
			return $this->argError(3, "LKREQIMMED", __LINE__);
		if(catchit($state = $this->m_buffer->doop(CMMS_L_LKREQIMMED, $laflag, $transnum, $this->makeSUBLIST($name_array_array))))
		        return OPERATION_FAILURE;
		        return $state;			
	}

	/* LKDELETE
	*  action:	parse input, instantiate an array of SUBSC object and call buffer doop function
	*  		return CMMS_M_LKDELETED message if successful
	*  usage:	$value = $this->LKDELETE($laflag, $transnum, aray of array of name and subscripts)
	*  note:	make sure the correct region is being used
	*/
public	function LKDELETE($laflag, $transnum, $name_array_array)
	{
		$num_args = func_num_args();
		if($num_args < 3)
			return $this->argError(3, "LKDELETE", __LINE__);
		 if(catchit($state = $this->m_buffer->doop(CMMS_L_LKDELETE, $laflag, $transnum, $this->makeSUBLIST($name_array_array)))) 
		        return OPERATION_FAILURE;
		        return $state;			
	}

	/* LKSUSPEND
	*  action:	
	*  		return CMMS_M_LKSUSPENDED if successful
	*  usage:	$value = $this->LKSUSPEND($laflag)
	*  note:	make sure the correct region is being used
	*/
public	function LKSUSPEND($laflag)
	{
		$num_args = func_num_args();
		if($num_args < 1)
			return $this->argError(1, "LKSUSPEND", __LINE__);
		if(catchit($state = $this->m_buffer->doop(CMMS_L_LKSUSPEND, $laflag)))
		        return OPERATION_FAILURE;
		 return $state;			
	}

	/* LKRESUME
	*  action:		
	*  		return CMMS_M_LKGRANTED or CMMS_M_LKBLOCKED if successful
	*  usage:	$value = $this->LKRESUME($laflag)
	*  note:	make sure the correct region is being used
	*/
public	function LKRESUME($laflag)
	{
		$num_args = func_num_args();
		if($num_args < 1)
			return $this->argError(1, "LKRESUME", __LINE__);
		if(catchit($state = $this->m_buffer->doop(CMMS_L_LKRESUME, $laflag)))
		        return OPERATION_FAILURE;
		 return $state;			
	}

	/* LKACQUIRE
	*  action:		
	*  		return CMMS_M_LKGRANTED if successful
	*		return FALSE if no data was read from the socket (need more time)
	*  usage:	$value = $this->LKACQUIRE($laflag)
	*  note:	make sure the correct region is being used
	*/
public	function LKACQUIRE($laflag)
	{
		$num_args = func_num_args();
		if($num_args < 1)
			return $this->argError(1, "LKACQUIRE", __LINE__);
		if(catchit($state = $this->m_buffer->doop(CMMS_L_LKACQUIRE, $laflag)))
		        return OPERATION_FAILURE;
		        return $state;			//this could be FALSE when no data is received
	}

	/* LKCANCEL
	*  action:	send LKCANCEL message via normal channel, c.f. LKURGCANCEL	
	*  		return CMMS_M_LKDELETED if successful
	*  usage:	$value = $this->LKCANCEL($laflag, $transnum)
	*  note:	make sure the correct region is being used
	*/
public	function LKCANCEL($laflag, $transnum)
	{
		$num_args = func_num_args();
		if($num_args < 2)
			return $this->argError(2, "LKCANCEL", __LINE__);
		if(catchit($state = $this->m_buffer->doop(CMMS_L_LKCANCEL, $laflag, $transnum)))
			return OPERATION_FAILURE;
			return $state;			
	}

	/* LKURGCANCEL 	(not part of GNP protocol, but crucial for the GTCM implementation)
	*  action:	send LKINTERRUPT message via urgent channel, c.f. LKCANCEL	
	*  		return CMMS_M_DELETED if successful
	*  usage:	$value = $this->LKURGCANCEL($transnum)
	*  note:	make sure the correct region is being used
	*/
public	function LKURGCANCEL($transnum)
	{
		$num_args = func_num_args();
		if($num_args < 1)
			return $this->argError(1, "LKURGCANCEL", __LINE__);
		if(catchit($state = $this->m_buffer->doop(CMMS_S_INTERRUPT, $transnum)))
		        return OPERATION_FAILURE;
		       return $state;			
	}
		
	/* LKCANALL
	*  action:	sent when unconditional lock cancel is needed 	
	*  		return CMMS_M_LKDELETED if successful
	*  usage:	$value = $this->LKCANALL($laflag, $transnum)
	*  note:	make sure the correct region is being used
	*/
public	function LKCANALL($laflag, $transnum)
	{
		$num_args = func_num_args();
		if($num_args < 2)
			return $this->argError(2, "LKCANALL", __LINE__);
		 if(catchit($state = $this->m_buffer->doop(CMMS_L_LKCANALL, $laflag, $transnum)))
		        return OPERATION_FAILURE;
		     return $state;			
	}

	#################################
	#	PRIVATE FUNCTIONS	#
	#################################

	/* Create a GDS object from an array of global references
	*  action:	transform $array into a GDS object, only if $array is not already a SUBSC
	*  usage:	$sub = $this->makeSUBSC($name_array)
	*/
private	function makeSUBSC($name_array)
	{
		if((is_object($name_array)) && (get_class($name_array) == "SUBSC"))
		{
			return $name_array;
		}
		$GDS_key_seed = array();
		assert(is_array($name_array));
		$GDS_key_seed = $name_array;
		$GDS_key = new GDS($GDS_key_seed);
		$subsc = new SUBSC($this->m_cur_region_no, $GDS_key);
		return $subsc;
	}

	/* Create an array of GDS object from an array of global references arrays
	*  action:	transform $array into a GDS object, assemble an array of GDS objects
	*  usage:	$sub = $this->makeSUBLIST($name_array_array)
	*  note:	the list is in reverse order for lock messages
	*/
private	function makeSUBLIST($name_array_array)
	{
		$subList = array();
		$name_array_array = (array) $name_array_array;
		foreach($name_array_array as $n)
			$subList[] = $this->makeSUBSC($n);
		$subList_rev = array_reverse($subList, FALSE);
		return $subList_rev;
	}

	/* argument error handler
	*  action:	check for number of arguments and call ERROR_HANDLER if necessary
	*  usage:  	return $this->argError($num_args, $FUNCTION, __LINE__)
	*/
private	function argError($num_args, $func_name, $line)
	{
		$err_msg = "Invalid Argument Error: " . $func_name . "() requires at least " . 
			    $num_args . " argument(s).<br>\n";
		print_r($err_msg);
		return OPERATION_FAILURE;
	}

	/* CLEANUP
	*  action:	close the TCP socket, unset all data members
	*  usage:	$this->cleanUp()
	*/
private	function cleanUp()
	{
		$this->m_socket->closeSocket();
		unset($this->m_server);
		unset($this->m_port);
		unset($this->m_dat_file);
		unset($this->m_timeOut);
		unset($this->m_socket);
		unset($this->m_buffer);
		unset($this->m_proc_num);
		unset($this->m_open_region);
		unset($this->m_cur_region_no);
	}


}


?>
