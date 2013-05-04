

<?php

require("gtcm_gnp_class.inc");

?>

<?php

class GNP_LOCK_MANAGER
{
	#########################
	#	DATA MEMBERS	#
	#########################

	private $m_transnum;		//incremented by every lock or ZA operation for a new lock
	private $m_laflag;			//the most recently used laflag
	private $m_last_canall;		//TRUE if last operation was unconditional unlock	
	private $m_response_cnt;		//number of responses received, one from each server
	private $m_request_cnt;		//maximum number of responses expected
	private $m_granted_cnt;		//count of servers that have granted the lock
	private $m_responded;		//array of names of GTCM_GNP objects that have responded, either granted or blocked
	private $m_granted;			//array of names of GTCM_GNP objects that have granted locks in current lock request operation
	private $m_locked_servers;		//name of GTCM_GNP objects that have ever been LOCKED for this client
	private $m_zallocated_servers;	//name of GTCM_GNP objects that have ever been ZALLOCATED for this client
	private $m_timeout;			//after which m_out_of_time flag will be set	
	private $m_elapsed_time;		//time spent 'sleeping' so far
	private $m_out_of_time;		//boolean
	private $m_interrupted;		//boolean user clicked 'stop', not really implemented yet
	private $m_gld;			//the GNP_DIRECTORY object that can map global names to their corresponding GTCM_GNP object names
	static $instances = array();
	
	#################################
	#	PUBLIC FUNCTIONS 	#
	#################################

public	function __construct($gld = null)
	{
		$this->m_transnum = 0;
		$this->m_granted_cnt = 0;
		$this->m_gld = $gld;
		$this->m_granted = array();
		$this->m_locked_servers = array();
		$this->m_zallocated_servers = array();
		$this->initRequest(FALSE, TRUE);
		GNP_DIRECTORY::$instances[] = $this;
		self::$instances[] = $this;
	}

	#################################################################################
	#	NOTE: There are 3 different public interfaces to most lock functions.	#
	#		Choose one that is most convenient to use for the application	#
	#################################################################################

	/* LOCK_ADD_IMMED
	* action:	remove all previously obtained locks if necessay
	*		call LKREQIMMED functions of the requested database objects
	*		return TRUE if all databases requested grant the lock
	*		return FALSE after sending LKCANCEL all servers that granted locks, if CMMS_M_LKABORT is received
	*		return OPERATION_FAILURE if error occurred
	* usage:	$locked = $lockmgr->lockAddImmed((db_obj_name, array_of_name_array)+)
	*/
public     function lockAddImmed($db_obj_name, $name_array_array /* more pairs if needed */)
	{
		if(!$this->m_last_canall)
		{
			if(catchit($this->unlockAll()))
				return OPERATION_FAILURE;
		}

		$args = func_get_args();
		return $this->immediateLock(CM_LOCKS, $args);
	}

	/* functionally equivalent to the previous one, except that it requires only the global variables
	*  leaving the job of finding the corresponding $db_obj_name to GNP_DIRECTORY m_gld;
	*  e.g. $name_array could be: $a, $b, $c, where $a = array("a",1), $b = array("b"), $c = array("aB")
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
public   function lockAddImmed_2($name_array /* more if needed */)
	{
		$name_array_array = func_get_args();
		return $this->lockAddImmed_3($name_array_array);
	}

	/* functionally equivalent to the previous two, except that it requires only the global variable array
	*  leaving the job of finding the corresponding $db_obj_name to GNP_DIRECTORY m_gld;
	*  e.g. $name_array_array could be: array($a, $b, $c), where $a = array("a",1), $b = array("b"), $c = array("aB")
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
public   function lockAddImmed_3($name_array_array)
	{
		if(!$this->m_last_canall)
		{
			if(catchit($this->unlockAll()))
				return OPERATION_FAILURE;
		}

		$args = $this->addDB($name_array_array);
		return $this->immediateLock(CM_LOCKS, $args);
	}

	/* LOCK_ADD
	* action:	remove all previously obtained locks if necessay
	*		call LKREQUEST functions of the requested database objects, followed by LKACQUIRE/LKSUSPEND/LKRESUME, if needed
	*		return TRUE if all databases requested grant the lock within specified or default time limit
	*		return FALSE after sending LKCANCEL to all if timed out or user interrupted
	*		return OPERATION_FAILURE if error occurred 
	* usage:	$locked = $lockmgr->lockAdd((db_obj_name, array_of_name_array)+, $time_limit)
	*/
public   function lockAdd($db_obj_name, $name_array_array /* more pairs if needed, followed optionally by time_limit */)	
	{
		if(!$this->m_last_canall)
		{
			if(catchit($this->unlockAll()))
				return OPERATION_FAILURE;
		}
		
		$args = func_get_args();
		return $this->timedLock(CM_LOCKS, $args);
	}

	/* functionally equivalent to the previous one, except that it requires only the global variables
	*  leaving the job of finding the corresponding $db_obj_name to GNP_DIRECTORY m_gld;
	*  e.g. $name_array could be: $a, $b, $c, where $a = array("a",1), $b = array("b"), $c = array("aB")
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
public   function lockAdd_2($name_array /* more if needed */)
	{
		$name_array_array = func_get_args();
		return $this->lockAdd_3($name_array_array);
	}

	/* functionally equivalent to the previous two, except that it requires only the global variable array
	*  leaving the job of finding the corresponding $db_obj_name to GNP_DIRECTORY m_gld;
	*  e.g. $name_array_array could be: array($a, $b, $c), where $a = array("a",1), $b = array("b"), $c = array("aB")
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*	Note: the last element is always the time limit
	*/
public   function lockAdd_3($name_array_array)
	{
		if(!$this->m_last_canall)
		{
			if(catchit($this->unlockAll()))
				return OPERATION_FAILURE;
		}
		
		$time_limit = array_pop($name_array_array);
		$args = $this->addDB($name_array_array);
		array_push($args, $time_limit);
		return $this->timedLock(CM_LOCKS, $args);
	}

	/* LOCK_INCR_ADD_IMMED
	* action:	
	*		call LKREQIMMED functions of the requested database objects
	*		return TRUE if all databases requested grant the lock
	*		return FALSE after LKCANCEL all those granted locks, if any returns CMMS_M_LKABORT
	*		return OPERATION_FAILURE if error encountered
	* usage:	$locked = $lockmgr->lockIncrAddImmed((db_obj_name, array_of_name_array)+)
	*/
public   function lockIncrAddImmed($db_obj_name, $name_array_array /* more pairs if needed */)
	{
		$args = func_get_args();
		return $this->immediateLock(CM_LOCKS_INCR, $args);
	}

	/* functionally equivalent to the previous one, except that it requires only the global variables
	*  leaving the job of finding the corresponding $db_obj_name to GNP_DIRECTORY m_gld;
	*  e.g. $name_array could be: $a, $b, $c, where $a = array("a",1), $b = array("b"), $c = array("aB")
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
public   function lockIncrAddImmed_2($name_array /* more if needed */)
	{
		$name_array_array = func_get_args();
		return $this->lockIncrAddImmed_3($name_array_array);
	}

	/* functionally equivalent to the previous two, except that it requires only the global variable array
	*  leaving the job of finding the corresponding $db_obj_name to GNP_DIRECTORY m_gld;
	*  e.g. $name_array_array could be: array($a, $b, $c), where $a = array("a",1), $b = array("b"), $c = array("aB")
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
public   function lockIncrAddImmed_3($name_array_array)
	{
		$args = $this->addDB($name_array_array);
		return $this->immediateLock(CM_LOCKS_INCR, $args);
	}

	/* LOCK_INCR_ADD
	* action:	
	*		call LKREQUEST functions of the requested database objects, followed by LKACQUIRE/LKSUSPEND/LKRESUME, if needed
	*		return TRUE if all databases requested grant the lock within specified or default time limit
	*		return FALSE after sending LKCANCEL to all if timed out or user interrupted
	*		return OPERATION_FAILURE if error encountered 
	* usage:	$locked = $lockmgr->lockIncrAdd((db_obj_name, array_of_name_array)+, $time_limit)
	*/
public   function lockIncrAdd($db_obj_name, $name_array_array /* more pairs if needed, followed optionally by time_limit */)
	{
		$args = func_get_args();
		return $this->timedLock(CM_LOCKS_INCR, $args);
	}

	/* functionally equivalent to the previous one, except that it requires only the global variables
	*  leaving the job of finding the corresponding $db_obj_name to GNP_DIRECTORY m_gld;
	*  e.g. $name_array could be: $a, $b, $c, where $a = array("a",1), $b = array("b"), $c = array("aB")
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
public   function lockIncrAdd_2($name_array /* more if needed */)
	{
		$name_array_array = func_get_args();
		return $this->lockIncrAdd_3($name_array_array);
	}

	/* functionally equivalent to the previous two, except that it requires only the global variable array
	*  leaving the job of finding the corresponding $db_obj_name to GNP_DIRECTORY m_gld;
	*  e.g. $name_array_array could be: array($a, $b, $c), where $a = array("a",1), $b = array("b"), $c = array("aB")
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
public   function lockIncrAdd_3($name_array_array)
	{
		$time_limit = array_pop($name_array_array);
		$args = $this->addDB($name_array_array);
		array_push($args, $time_limit);

		return $this->timedLock(CM_LOCKS_INCR, $args);
	}

	/* UNLOCK
	* action:	call LKDELETE functions of the requested database objects
	*		return TRUE if all databases requested return LKDELETED
	*		return OPERATION_FAILURE otherwise
	* usage;	$lock_deleted = $lockmgr->unlock((db_obj_name, array_of_name_array)+)
	*/
public   function unlock($db_obj_name, $name_array_array /* more pairs if needed */)
	{
		$args = func_get_args();
		return $this->removeLock(CM_LOCKS_INCR, $args);
	}

	/* functionally equivalent to the previous one, except that it requires only the global variables
	*  leaving the job of finding the corresponding $db_obj_name to GNP_DIRECTORY m_gld;
	*  e.g. $name_array could be: $a, $b, $c, where $a = array("a",1), $b = array("b"), $c = array("aB")
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
public   function unlock_2($name_array /* more if needed */)
	{
		$name_array_array = func_get_args();
		return $this->unlock_3($name_array_array);
	}

	/* functionally equivalent to the previous two, except that it requires only the global variable array
	*  leaving the job of finding the corresponding $db_obj_name to GNP_DIRECTORY m_gld;
	*  e.g. $name_array_array could be: array($a, $b, $c), where $a = array("a",1), $b = array("b"), $c = array("aB")
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
public   function unlock_3($name_array_array)
	{
		$args = $this->addDB($name_array_array);
		return $this->removeLock(CM_LOCKS_INCR, $args);
	}

	/* UNLOCK_ALL
	* action:	remove all previously obtained locks 
	*		call LKCANALL functions of the requested database objects
	*		return TRUE if all databases responded with LKDELETED
	*		return OPERATION_FAILURE otherwise
	* usage:	$response = $lockmgr->unlockAll()
	*/
public   function unlockAll()	//return m_exception object if failed
	{
		if(!count($this->m_locked_servers))
			return TRUE;

		$this->initRequest(FALSE, TRUE, CM_LOCKS, count($this->m_locked_servers)); 
		foreach($this->m_locked_servers as $server)
		{
			if(catchit($res = self::$instances[$server]->LKCANALL(CM_LOCKS, $this->m_transnum)))
				return OPERATION_FAILURE;
			$this->m_response_cnt++;
		}
		
		unset($this->m_locked_servers);
		$this->m_locked_servers = array();
		return TRUE;
	}

	/* ZALLOCATE_IMMED
	* action:	
	*		call LKREQIMMED functions of the requested database objects
	*		return TRUE if all databases requested grant the lock
	*		return FALSE after LKCANCEL all those granted locks, if any returns CMMS_M_LKABORT
	*		return OPERATION_FAILURE if error encountered
	* usage:	$locked = $lockmgr->zallocateImmed((db_obj_name, array_of_name_array)+)
	*/
public   function zallocateImmed($db_obj_name, $name_array_array /* more pairs if needed */)
	{
		$args = func_get_args();
		return $this->immediateLock(CM_ZALLOCATES, $args);
	}

	/* functionally equivalent to the previous one, except that it requires only the global variables
	*  leaving the job of finding the corresponding $db_obj_name to GNP_DIRECTORY m_gld;
	*  e.g. $name_array could be: $a, $b, $c, where $a = array("a",1), $b = array("b"), $c = array("aB")
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
public   function zallocateImmed_2($name_array /* more if needed */)
	{
		$name_array_array = func_get_args();
		return $this->zallocateImmed_3($name_array_array);
	}

	/* functionally equivalent to the previous two, except that it requires only the global variable array
	*  leaving the job of finding the corresponding $db_obj_name to GNP_DIRECTORY m_gld;
	*  e.g. $name_array_array could be: array($a, $b, $c), where $a = array("a",1), $b = array("b"), $c = array("aB")
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
public   function zallocateImmed_3($name_array_array)
	{
		$args = $this->addDB($name_array_array);
		return $this->immediateLock(CM_ZALLOCATES, $args);
	}

	/* ZALLOCATE
	* action:	
	*		call LKREQUEST functions of the requested database objects, followed by LKACQUIRE/LKSUSPEND/LKRESUME, if needed
	*		return TRUE if all databases requested grant the lock within specified or default time limit
	*		return FALSE after sending LKCANCEL to all if timed out or user interrupted
 	*		return OPERATION_FAILURE if error encountered
	* usage:	$locked = $lockmgr->zallocate((db_obj_name, array_of_name_array)+, $time_limit)
	*/
public   function zallocate($db_obj_name, $name_array_array /* more pairs if needed, optionally followed by $time_limit */)
	{
		$args = func_get_args();
		return $this->timedLock(CM_ZALLOCATES, $args);
	}

	/* functionally equivalent to the previous one, except that it requires only the global variables
	*  leaving the job of finding the corresponding $db_obj_name to GNP_DIRECTORY m_gld;
	*  e.g. $name_array could be: $a, $b, $c, where $a = array("a",1), $b = array("b"), $c = array("aB")
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
public   function zallocate_2($name_array /* more if needed */)
	{
		$name_array_array = func_get_args();
		return $this->zallocate_3($name_array_array);
	}

	/* functionally equivalent to the previous two, except that it requires only the global variable array
	*  leaving the job of finding the corresponding $db_obj_name to GNP_DIRECTORY m_gld;
	*  e.g. $name_array_array could be: array($a, $b, $c), where $a = array("a",1), $b = array("b"), $c = array("aB")
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
public   function zallocate_3($name_array_array)
	{
		$time_limit = array_pop($name_array_array);
		$args = $this->addDB($name_array_array);
		array_push($args, $time_limit);
		return $this->timedLock(CM_ZALLOCATES, $args);
	}

	/* ZDEALLOCATE
	* action:	call LKDELETE functions of the requested database objects
	*		return TRUE if all databases requested return LKDELETED
	*		return OPERATION_FAILURE otherwise
	* usage;	$lock_deleted = $lockmgr->zdeallocate((db_obj_name, array_of_name_array)+)
	*/
public   function zdeallocate($db_obj_name, $name_array_array /* more pairs if needed */)
	{
		$args = func_get_args();
		return $this->removeLock(CM_ZALLOCATES, $args);
	}

	/* functionally equivalent to the previous one, except that it requires only the global variables
	*  leaving the job of finding the corresponding $db_obj_name to GNP_DIRECTORY m_gld;
	*  e.g. $name_array could be: $a, $b, $c, where $a = array("a",1), $b = array("b"), $c = array("aB")
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
public   function zdeallocate_2($name_array /* more if needed */)
	{
		$name_array_array = func_get_args();
		return $this->zdeallocate_3($name_array_array);
	}

	/* functionally equivalent to the previous two, except that it requires only the global variable array
	*  leaving the job of finding the corresponding $db_obj_name to GNP_DIRECTORY m_gld;
	*  e.g. $name_array_array could be: array($a, $b, $c), where $a = array("a",1), $b = array("b"), $c = array("aB")
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
public   function zdeallocate_3($name_array_array)
	{
		$args = $this->addDB($name_array_array);
		return $this->removeLock(CM_ZALLOCATES, $args);
	}

	/* ZDEALLOCATE_ALL
	* action:	remove all previously obtained locks by way of ZALLOCATE
	*		call LKCANALL functions of the requested database objects
	*		return TRUE if all databases responded with LKDELETED
	*		return OPERATION_FAILURE otherwise
	* usage:	$response = $lockmgr->zdeallocateAll()
	*/
public   function zdeallocateAll()
	{
		if(!count($this->m_zallocated_servers))
			return TRUE;

		$this->initRequest(FALSE, TRUE, CM_ZALLOCATES, count($this->m_zallocated_servers)); 
		foreach($this->m_zallocated_servers as $server)
		{
			if(catchit($res = self::$instances[$server]->LKCANALL(CM_ZALLOCATES, $this->m_transnum)))
				return OPERATION_FAILURE;	
		
			$this->m_response_cnt++;
		}
		
		unset($this->m_zallocated_servers);
		$this->m_zallocated_servers = array();
		return TRUE;
	}


	#################################
	#	PRIVATE FUNCTIONS	#
	#################################

	/* to be called by each lock operation */
private   function initRequest($transnum_incr = TRUE, $last_canall = FALSE, $laflag = CM_LOCKS, $req_cnt = 0, $time_limit = 0)
	{
		if($transnum_incr)
			$this->m_transnum++;
		$this->m_last_canall = $last_canall;
		$this->m_laflag = $laflag;
		$this->m_request_cnt = $req_cnt;
		$this->m_response_cnt = 0;
		$this->m_timeout = $time_limit;
		$this->m_elapsed_time = 0;
		$this->m_interrupted = $this->m_out_of_time = FALSE;
		$this->m_responded = array();
	}

	/* to be called, when timed out, to send one byte urgent message to the server on which a lock is being acquired
	 * returns TRUE if successful, m_exception object if error encountered
	 */
private   function urgentCancel($acq_db)
	{
		if(catchit($response = self::$instances[$acq_db]->LKURGCANCEL($this->m_transnum)))
		{
			$err = throwit(__FILE__, __LINE__, "urgentCancel", $response->getLevel() + 1);
			return $err;
		}
		return TRUE;
	}

	/* call LKCANCEL of objects whose name is in m_granted or m_responded 
	 * returns TRUE if successful, m_exception object if error encountered
	 */
private   function cancel()
	{
		if(count($this->m_responded))
		{
			foreach($this->m_responded as $obj_name)
			{
				if(defined("MSG"))
					echo "Cancelling $obj_name<br>\n";
				if(catchit($response = self::$instances[$obj_name]->LKCANCEL($this->m_laflag, $this->m_transnum)))
				{
					$err = throwit(__FILE__, __LINE__, "cancel", $response->getLevel() + 1);
					return $err;
				}
			}
		}
		if(count($this->m_granted))
		{
			foreach($this->m_granted as $obj_name)
			{
				if(defined("MSG"))
					echo "Cancelling $obj_name<br>\n";
				if(catchit($response = self::$instances[$obj_name]->LKCANCEL($this->m_laflag, $this->m_transnum)))
				{
					$err = throwit(__FILE__, __LINE__, "cancel", $response->getLevel() + 1);
					return $err;
				}
			}
		}
		return TRUE;
	}

	// add newly locked GTCM_GNP object name to m_locked_servers, check for redundancy
private   function addLockedServer($new_server)
	{
		if(!in_array($new_server, $this->m_locked_servers))
			$this->m_locked_servers[] = $new_server;
	}

	// add newly ZALLOCATED GTCM_GNP object name to m_zallocated_servers, check for redundancy
private   function addZallocatedServer($new_server)
	{
		if(!in_array($new_server, $this->m_zallocated_servers))
			$this->m_zallocated_servers[] = $new_server;
	}

	// request lock immediately, shared by lockAddImmed, lockIncrAddImmed, zallocateImmed
	// returns TRUE if successful, FALSE if aborted, or OPERATION_FAILURE if error occurred
private   function immediateLock($laflag, $args)
	{
		$num_args = count($args);
		assert($num_args > 0);
		assert(!($num_args % 2));
		$num_pairs = $num_args / 2;
		$this->m_granted = array();
		$this->initRequest(TRUE, FALSE, $laflag, $num_pairs, 0);

		for($i = 0; $i < $num_pairs; $i++)
		{
			$db_index = $i * 2;
			$db = $args[$db_index];			//$db must be a string
		//	assert(get_class(self::$instances[$db]) == "GTCM_GNP");
			$ref_index = $db_index + 1;
			$ref_array = $args[$ref_index];
			foreach(GTCM_GNP::$instances as $key => $item)
			  if($item->get_name() == $db)
			  break;
			if(catchit($response = $item->LKREQIMMED($laflag, $this->m_transnum, $ref_array)))
				return OPERATION_FAILURE;
			else
			{
				if(defined("lock"))
					echo "immediate lock: response = ", $response, "<br>\n";
				if($response == CMMS_M_LKABORT)
				{
					if(catchit($ret = $this->cancel()))
					{
						catchit(throwit(__FILE__, __LINE__, "immediateLock", $ret->getLevel() + 1));
						return OPERATION_FAILURE;
					}

					if(defined("lock"))
						echo "didn't get it<br>\n";
					return FALSE;
				}
				elseif($response == CMMS_M_LKGRANTED)
				{
					$this->m_response_cnt++;
					$this->m_granted_cnt++;
					$this->m_granted[] = $db;
					if($this->m_laflag != CM_ZALLOCATES)
						$this->addLockedServer($db);
					else $this->addZallocatedServer($db);
				}
				else assert(FALSE);
			}
		}
	
		if(defined("lock")) echo "got it<br>\n";
		return TRUE;
	}

	// request lock within t_limit, shared by lockAdd, lockIncrAdd, zallocate
	// returns TRUE if successful, FALSE if aborted, or OPERATION_FAILURE if error occurred
private   function timedLock($laflag, $args)
	{
		$num_args = count($args);
		assert($num_args > 0);
		if($num_args % 2)
		{
			$t_limit = $args[($num_args - 1)];
			assert($t_limit > 1);
			$num_pairs = ($num_args - 1) / 2;
		}
		else 
		{
			$t_limit = CLIENT_WAIT_TIMEOUT;
			$num_pairs = $num_args / 2;
		}
		$this->initRequest(TRUE, FALSE, $laflag, $num_pairs, $t_limit);
		$first_to_block = -1;		//$first_to_block holds the index of m_responded array which blocks first
		$this->m_granted_cnt = 0;
		$this->m_granted = array();

		// make a first attemp anyway
		for($i = 0; $i < $this->m_request_cnt; $i++)
		{
			$db_index = $i * 2;
			$db = $args[$db_index];
			$ref_index = $db_index + 1;
			$ref_array = $args[$ref_index];
			foreach(GTCM_GNP::$instances as $key => $item)
			  if($item->get_name() == $db)
			  break;
			  if(catchit($response = $item->LKREQUEST($laflag, $this->m_transnum, $ref_array)))
			    {
				if(catchit($ret = $this->urgentCancel($db)))
				{
					catchit(throwit(__FILE__, __LINE__, "timedLock", $ret->getLevel() + 1));
					return OPERATION_FAILURE;
				}

				if(catchit($ret = $this->cancel()))
				{
					catchit(throwit(__FILE__, __LINE__, "timedLock", $ret->getLevel() + 1));
					return OPERATION_FAILURE;
				}
				catchit(throwit(__FILE__, __LINE__, "timedLock", $response->getLevel() + 1));
				return OPERATION_FAILURE;
			    }
			   else
			    {
				$this->m_response_cnt++;
				$this->m_responded[] = $db;
				if($response == CMMS_M_LKBLOCKED)
				{
					if($first_to_block == -1)
						$first_to_block = count($this->m_responded) - 1;
				}
				elseif($response == CMMS_M_LKGRANTED)
				{
					$this->m_granted_cnt++;
				}
				else assert(FALSE); 
				
			}
		    
		}

		// if first attemp succeeded, we are all set
		if($this->m_granted_cnt == $this->m_request_cnt)
		{
			$this->m_granted = $this->m_responded;
			foreach($this->m_responded as $server)
			{
				if($this->m_laflag != CM_ZALLOCATES)
					$this->addLockedServer($db);
				else $this->addZallocatedServer($db);
			}
			return TRUE;
		}

		if(defined("MSG"))
			echo "First attemp failed<br>\n";

		// otherwise we need more time to do it
		$this->m_granted_cnt = 0;
		$this->m_granted = array();
		$follow_up = array();
		for($j = 0; $j < count($this->m_responded); $j++)
		{
			if($j != $first_to_block)
				$follow_up[] = $this->m_responded[$j];
		}
		$acq_db = $this->m_responded[$first_to_block];
		$this->initRequest(FALSE, FALSE, $this->m_laflag, $num_pairs, $this->m_timeout);
		do
		{
			//send LKACQUIRE to first_to_block and LKSUSPEND to the rest
			foreach($follow_up as $sus_server)
			{
				if(catchit($sus_res = self::$instances[$sus_server]->LKSUSPEND($this->m_laflag)))
				{
					$this->m_responded = $follow_up;
					if(catchit($ret = $this->urgentCancel($acq_db)))
					{
						catchit(throwit(__FILE__, __LINE__, "timedLock", $ret->getLevel() + 1));
						return OPERATION_FAILURE;
					}

					if(catchit($ret = $this->cancel()))
					{
						catchit(throwit(__FILE__, __LINE__, "timedLock", $ret->getLevel() + 1));
						return OPERATION_FAILURE;
					}

					catchit(throwit(__FILE__, __LINE__, "timedLock", $sus_res->getLevel() + 1));
					return OPERATION_FAILURE;
				}
				else
				{
					$this->m_response_cnt++;
					$this->m_responded[] = $sus_server;
				}
			}
	
			if(defined("MSG"))
				echo "$acq_db is about to LKACQUIRE<br>\n";

			if(catchit($acq_res = self::$instances[$acq_db]->LKACQUIRE($this->m_laflag)))
			{
				if(catchit($ret = $this->urgentCancel($acq_db)))
				{
					catchit(throwit(__FILE__, __LINE__, "timedLock", $ret->getLevel() + 1));
					return OPERATION_FAILURE;
				}

				if(catchit($ret = $this->cancel()))
				{
					catchit(throwit(__FILE__, __LINE__, "timedLock", $ret->getLevel() + 1));
					return OPERATION_FAILURE;
				}

				catchit(throwit(__FILE__, __LINE__, "timedLock", $acq_res->getLevel() + 1));
				return OPERATION_FAILURE;
			}
	
			$this->m_elapsed_time = LOCK_INTERVAL;
			//if previous acquire resulted in FALSE, we keep trying receiving
			if($acq_res == FALSE)
			{
				if($this->m_timeout <= $this->m_elapsed_time)
				{
					$this->m_out_of_time = TRUE;
					if(catchit($ret = $this->urgentCancel($acq_db)))
					{
						catchit(throwit(__FILE__, __LINE__, "timedLock", $ret->getLevel() + 1));
						return OPERATION_FAILURE;
					}

					if(catchit($ret = $this->cancel()))
					{
						catchit(throwit(__FILE__, __LINE__, "timedLock", $ret->getLevel() + 1));
						return OPERATION_FAILURE;
					}
				
					break;
				}
					 
				do
				{
					if(!($acq_recv = self::$instances[$acq_db]->m_buffer->lockReceive()))
					{
						if(catchit($ret = $this->urgentCancel($acq_db)))
						{
							catchit(throwit(__FILE__, __LINE__, "timedLock", $ret->getLevel() + 1));
							return OPERATION_FAILURE;
						}

						if(catchit($ret = $this->cancel()))
						{
							catchit(throwit(__FILE__, __LINE__, "timedLock", $ret->getLevel() + 1));
							return OPERATION_FAILURE;
						}
	
						catchit(throwit(__FILE__, __LINE__, "timedLock", 1));
						return OPERATION_FAILURE;
					}
					$this->m_elapsed_time += LOCK_INTERVAL;
					$this->m_out_of_time = ($this->m_timeout <= $this->m_elapsed_time);
				}while(($acq_recv == TRUE) && (is_null(self::$instances[$acq_db]->m_buffer->m_data)) 
					&& (!$this->m_out_of_time));	
				if($this->m_out_of_time)
				{
					if(catchit($ret = $this->urgentCancel($acq_db)))
					{
						catchit(throwit(__FILE__, __LINE__, "timedLock", $ret->getLevel() + 1));
						return OPERATION_FAILURE;
					}

					if(catchit($ret = $this->cancel()))
					{
						catchit(throwit(__FILE__, __LINE__, "timedLock", $ret->getLevel() + 1));
						return OPERATION_FAILURE;
					}

					break;
				}
			}
			$this->m_granted_cnt++;
			$this->m_granted[] = $acq_db;
			if($this->m_granted_cnt == $this->m_request_cnt) break;
			$first_to_block = -1;
			$temp_granted = 0;
			$suspended = $this->m_responded;
			$this->initRequest(FALSE, FALSE, $this->m_laflag, 
				$num_pairs, ($this->m_timeout - $this->m_elapsed_time));
			foreach($suspended as $server)
			{
				if(catchit($response = self::$instances[$server]->LKRESUME($this->m_laflag)))
				{
					$this->m_responded = $suspended;
					if(catchit($ret = $this->urgentCancel($acq_db)))
					{
						catchit(throwit(__FILE__, __LINE__, "timedLock", $ret->getLevel() + 1));
						return OPERATION_FAILURE;
					}
					if(catchit($ret = $this->cancel()))
					{
						catchit(throwit(__FILE__, __LINE__, "timedLock", $ret->getLevel() + 1));
						return OPERATION_FAILURE;
					}
	
					catchit(throwit(__FILE__, __LINE__, "timedLock", $response->getLevel() + 1));
					return OPERATION_FAILURE;
				}
				else
				{
					$this->m_response_cnt++;
					$this->m_responded[] = $server;
					if($response == CMMS_M_LKBLOCKED)
					{
						if($first_to_block == -1)
							$first_to_block = count($this->m_responded) - 1;
					}
					elseif($response == CMMS_M_LKGRANTED)
					{
						$temp_granted++;
					}
					else assert(FALSE);
				}
			}

			if($first_to_block == -1)
			{
				$this->m_granted_cnt += $temp_granted;
				foreach($suspended as $granted)
					$this->m_granted[] = $granted;
				break;
			}
			else
			{
				$follow_up = array();
				for($j = 0; $j < count($this->m_responded); $j++)
				{
					if($j != $first_to_block)
						$follow_up[] = $this->m_responded[$j];
				}
				$acq_db = $this->m_responded[$first_to_block];
				$this->initRequest(FALSE, FALSE, $this->m_laflag, 
					$num_pairs, $this->m_timeout);
			}

		} while(($this->m_granted_cnt < $this->m_request_cnt) && (!$this->m_out_of_time));
		

		if($this->m_granted_cnt == $this->m_request_cnt)
		{
			foreach($this->m_granted as $server)
			{
				if($this->m_laflag != CM_ZALLOCATES)
					$this->addLockedServer($server);
				else $this->addZallocatedServer($server);
			}
			return TRUE;
		}
		else return FALSE;
	}

	// delete locks, shared by unlock and zdeallocate
	// returns TRUE on success, OPERATION_FAILURE on failure
private	function removeLock($laflag, $args) 
	{
		$num_args = count($args);
		assert($num_args > 0);
		assert(!($num_args % 2));
		$num_pairs = $num_args / 2;
		$this->initRequest(FALSE, FALSE, $laflag, $num_pairs, 0);

		for($i = 0; $i < $num_pairs; $i++)
		{
			$db_index = $i * 2;
			$db = $args[$db_index];			//$db must be a string
			//assert(get_class(self::$instances[$db]) == "GTCM_GNP");
			$ref_index = $db_index + 1;
			$ref_array = $args[$ref_index];
			foreach(GTCM_GNP::$instances as $key => $item)
			  if($item->get_name() == $db)
			  break;
			if(catchit($response = $item->LKDELETE($laflag, $this->m_transnum, $ref_array)))
			{
				catchit(throwit(__FILE__, __LINE__, "removeLock", $response->getLevel() + 1));
				return OPERATION_FAILURE;
			}
			elseif($response == CMMS_M_LKDELETED)
			{
				$this->m_response_cnt++;
			}
			else assert(FALSE);
		}
		return TRUE;
	}
	
	// transforms array($global1, $global2,...) into array("db1", array($global1, $global2), "db2", ...)
private  function addDB($name_array_array)
	{
		assert(!is_null($this->m_gld));

		$arr = array();
		foreach($name_array_array as $name_array)
		{
			//find out which GTCM_GNP object each belongs to

			$db_name = $this->m_gld->lookUp($name_array[0]);

			$keys = array_keys($arr);
			if(in_array($db_name, $keys))
				$arr[$db_name][] = $name_array;
			else
			{
				$arr[$db_name] = array($name_array);
			}
		}
		$ret_arr = array();
		foreach ($arr as $db => $gvns)
		{
			$gvns = (array)$gvns;
			$ret_arr[] = $db;
			$ret_arr[] = $gvns;
		}
		return $ret_arr;
	}
}
?>
