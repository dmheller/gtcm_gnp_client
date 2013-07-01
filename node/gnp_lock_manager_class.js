

var gtcm_gnp_class = require('gtcm_gnp_class');
var gnp_buffer = require('gnp_buffer');
var m = require('constants');
var defineClass = require("./defineClass.js").defineClass;
var _ = require('underscore')._;


var m_transnum;		//incremented by every lock or ZA operation for a new lock
var m_laflag;			//the most recently used laflag
var m_last_canall;		//true if last operation was unconditional unlock	
var m_response_cnt;		//number of responses received, one from each server
var m_request_cnt;		//maximum number of responses expected
var m_granted_cnt;		//util.count of servers that have granted the lock
var m_responded;		//array of names of gtcm_gnp objects that have responded, either granted or blocked
var m_granted;			//array of names of gtcm_gnp objects that have granted locks in current lock request operation
var m_locked_servers;		//name of gtcm_gnp objects that have ever been LOCKED for this client
var m_zallocated_servers;	//name of gtcm_gnp objects that have ever been ZALLOCATED for this client
var m_timeout;			//after which m_out_of_time flag will be set	
var m_elapsed_time;		//time spent 'sleeping' so far
var m_out_of_time;		//boolean
var m_interrupted;		//boolean user clicked 'stop', not really implemented yet
var m_gld;			//the gnp_directory object that can map global names to their corresponding gtcm_gnp object names


var gnp_lock_manager = defineClass({
constructor: function(gld){
       if (typeof gld === 'undefined') {
		    gld = null; }
		this.m_transnum = 0;
		this.m_granted_cnt = 0;
		this.m_gld = gld;
		this.m_granted = {};
		this.m_locked_servers = {};
		this.m_zallocated_servers = {};
		this.initRequest(false, true);
},
	/* LOCK_ADD_IMMED
	* action:	remove all previously obtained locks if necessay
	*		call LKREQIMMED functions of the requested database objects
	*		return true if all databases requested grant the lock
	*		return false after sending LKCANCEL all servers that granted locks, if CMMS_M_LKABORT is received
	*		return OPERATION_FAILURE if error occurred
	* usage:	$locked = $lockmgr->lockAddImmed((db_obj_name, array_of_name_array)+)
	*/
	lockAddImmed: function(db_obj_name, name_array_array){
		if(!this.m_last_canall)
		{
			if(this.unlockAll())
				return OPERATION_FAILURE;
		}

		args = func_get_args();
		return this.immediateLock(CM_LOCKS, args);
	},

	/* functionally equivalent to the previous one, except that it requires only the global variables
	*  leaving the job of finding the corresponding $db_obj_name to gnp_directory m_gld;
	*  e.g. $name_array could be: $a, $b, $c, where $a = array('a',1), $b = array('b'), $c = array('aB')
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
	lockAddImmed_2: function(name_array){
		name_array_array = func_get_args();
		return this.lockAddImmed_3(name_array_array);
	},

	/* functionally equivalent to the previous two, except that it requires only the global variable array
	*  leaving the job of finding the corresponding $db_obj_name to gnp_directory m_gld;
	*  e.g. $name_array_array could be: array($a, $b, $c), where $a = array('a',1), $b = array('b'), $c = array('aB')
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
	lockAddImmed_3: function(name_array_array){
		if(!this.m_last_canall)
		{
			if(this.unlockAll())
				return OPERATION_FAILURE;
		}

		args = this.addDB(name_array_array);
		return this.immediateLock(CM_LOCKS, args);
	},

	/* LOCK_ADD
	* action:	remove all previously obtained locks if necessay
	*		call LKREQUEST functions of the requested database objects, followed by LKACQUIRE/LKSUSPEND/LKRESUME, if needed
	*		return true if all databases requested grant the lock within specified or default time limit
	*		return false after sending LKCANCEL to all if timed out or user interrupted
	*		return OPERATION_FAILURE if error occurred 
	* usage:	$locked = $lockmgr->lockAdd((db_obj_name, array_of_name_array)+, $time_limit)
	*/
	lockAdd: function(db_obj_name, name_array_array){
		if(!this.m_last_canall){
			if(this.unlockAll())
				return OPERATION_FAILURE;
		}
		
		args = func_get_args();
		return this.timedLock(CM_LOCKS, args);
	},

	/* functionally equivalent to the previous one, except that it requires only the global variables
	*  leaving the job of finding the corresponding $db_obj_name to gnp_directory m_gld;
	*  e.g. $name_array could be: $a, $b, $c, where $a = array('a',1), $b = array('b'), $c = array('aB')
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
	lockAdd_2: function(name_array){
		name_array_array = func_get_args();
		return this.lockAdd_3(name_array_array);
	},

	/* functionally equivalent to the previous two, except that it requires only the global variable array
	*  leaving the job of finding the corresponding $db_obj_name to gnp_directory m_gld;
	*  e.g. $name_array_array could be: array($a, $b, $c), where $a = array('a',1), $b = array('b'), $c = array('aB')
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*	Note: the last element is always the time limit
	*/
	lockAdd_3: function(name_array_array){
		if(!this.m_last_canall)
		{
			if(this.unlockAll())
				return OPERATION_FAILURE;
		}
		
		time_limit = array_pop(name_array_array);
		args = this.addDB(name_array_array);
		array_push(args, time_limit);
		return this.timedLock(CM_LOCKS, args);
	},

	/* LOCK_INCR_ADD_IMMED
	* action:	
	*		call LKREQIMMED functions of the requested database objects
	*		return true if all databases requested grant the lock
	*		return false after LKCANCEL all those granted locks, if any returns CMMS_M_LKABORT
	*		return OPERATION_FAILURE if error enutil.countered
	* usage:	$locked = $lockmgr->lockIncrAddImmed((db_obj_name, array_of_name_array)+)
	*/
	lockIncrAddImmed: function(db_obj_name, name_array_array){
		args = func_get_args();
		return this.immediateLock(CM_LOCKS_INCR, args);
	},

	/* functionally equivalent to the previous one, except that it requires only the global variables
	*  leaving the job of finding the corresponding $db_obj_name to gnp_directory m_gld;
	*  e.g. $name_array could be: $a, $b, $c, where $a = array('a',1), $b = array('b'), $c = array('aB')
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
	lockIncrAddImmed_2: function(name_array){
		name_array_array = func_get_args();
		return this.lockIncrAddImmed_3(name_array_array);
	},

	/* functionally equivalent to the previous two, except that it requires only the global variable array
	*  leaving the job of finding the corresponding $db_obj_name to gnp_directory m_gld;
	*  e.g. $name_array_array could be: array($a, $b, $c), where $a = array('a',1), $b = array('b'), $c = array('aB')
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
	lockIncrAddImmed_3: function(name_array_array){
		args = this.addDB(name_array_array);
		return this.immediateLock(CM_LOCKS_INCR, args);
	},

	/* LOCK_INCR_ADD
	* action:	
	*		call LKREQUEST functions of the requested database objects, followed by LKACQUIRE/LKSUSPEND/LKRESUME, if needed
	*		return true if all databases requested grant the lock within specified or default time limit
	*		return false after sending LKCANCEL to all if timed out or user interrupted
	*		return OPERATION_FAILURE if error enutil.countered 
	* usage:	$locked = $lockmgr->lockIncrAdd((db_obj_name, array_of_name_array)+, $time_limit)
	*/
	lockIncrAdd: function(db_obj_name, name_array_array){
		args = func_get_args();
		return this.timedLock(CM_LOCKS_INCR, args);
	},

	/* functionally equivalent to the previous one, except that it requires only the global variables
	*  leaving the job of finding the corresponding $db_obj_name to gnp_directory m_gld;
	*  e.g. $name_array could be: $a, $b, $c, where $a = array('a',1), $b = array('b'), $c = array('aB')
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
	lockIncrAdd_2: function(name_array){
		name_array_array = func_get_args();
		return this.lockIncrAdd_3(name_array_array);
	},

	/* functionally equivalent to the previous two, except that it requires only the global variable array
	*  leaving the job of finding the corresponding $db_obj_name to gnp_directory m_gld;
	*  e.g. $name_array_array could be: array($a, $b, $c), where $a = array('a',1), $b = array('b'), $c = array('aB')
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
	lockIncrAdd_3: function(name_array_array){
		time_limit = array_pop(name_array_array);
		args = this.addDB(name_array_array);
		array_push(args, time_limit);

		return this.timedLock(CM_LOCKS_INCR, args);
	},

	/* UNLOCK
	* action:	call LKDELETE functions of the requested database objects
	*		return true if all databases requested return LKDELETED
	*		return OPERATION_FAILURE otherwise
	* usage;	$lock_deleted = $lockmgr->unlock((db_obj_name, array_of_name_array)+)
	*/
	unlock: function(db_obj_name, name_array_array){
		args = func_get_args();
		return this.removeLock(CM_LOCKS_INCR, args);
	},

	/* functionally equivalent to the previous one, except that it requires only the global variables
	*  leaving the job of finding the corresponding $db_obj_name to gnp_directory m_gld;
	*  e.g. $name_array could be: $a, $b, $c, where $a = array('a',1), $b = array('b'), $c = array('aB')
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
	unlock_2: function(name_array){
		name_array_array = func_get_args();
		return this.unlock_3(name_array_array);
	},

	/* functionally equivalent to the previous two, except that it requires only the global variable array
	*  leaving the job of finding the corresponding $db_obj_name to gnp_directory m_gld;
	*  e.g. $name_array_array could be: array($a, $b, $c), where $a = array('a',1), $b = array('b'), $c = array('aB')
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
	unlock_3: function(name_array_array){
		args = this.addDB(name_array_array);
		return this.removeLock(CM_LOCKS_INCR, args);
	},

	/* UNLOCK_ALL
	* action:	remove all previously obtained locks 
	*		call LKCANALL functions of the requested database objects
	*		return true if all databases responded with LKDELETED
	*		return OPERATION_FAILURE otherwise
	* usage:	$response = $lockmgr->unlockAll()
	*/
	unlockAll: function(){
		if(util.count(this.m_locked_servers))
			return true;

		this.initRequest(false, true, CM_LOCKS, util.count(this.m_locked_servers)); 
		for (var serverVal in this) {
                        server = this[serverVal];
			if(res = GLOBALS[server].LKCANALL(CM_LOCKS, this.m_transnum))
				return OPERATION_FAILURE;
			this.m_response_cnt++;
		}
		
		(this.m_locked_servers);
		this.m_locked_servers = {};
		return true;
	},

	/* ZALLOCATE_IMMED
	* action:	
	*		call LKREQIMMED functions of the requested database objects
	*		return true if all databases requested grant the lock
	*		return false after LKCANCEL all those granted locks, if any returns CMMS_M_LKABORT
	*		return OPERATION_FAILURE if error enutil.countered
	* usage:	$locked = $lockmgr->zallocateImmed((db_obj_name, array_of_name_array)+)
	*/
	zallocateImmed: function(db_obj_name, name_array_array){
		args = func_get_args();
		return this.immediateLock(CM_ZALLOCATES, args);
	},

	/* functionally equivalent to the previous one, except that it requires only the global variables
	*  leaving the job of finding the corresponding $db_obj_name to gnp_directory m_gld;
	*  e.g. $name_array could be: $a, $b, $c, where $a = array('a',1), $b = array('b'), $c = array('aB')
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
	zallocateImmed_2: function(name_array){
		name_array_array = func_get_args();
		return this.zallocateImmed_3(name_array_array);
	},

	/* functionally equivalent to the previous two, except that it requires only the global variable array
	*  leaving the job of finding the corresponding $db_obj_name to gnp_directory m_gld;
	*  e.g. $name_array_array could be: array($a, $b, $c), where $a = array('a',1), $b = array('b'), $c = array('aB')
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
	zallocateImmed_3: function(name_array_array){
		args = this.addDB(name_array_array);
		return this.immediateLock(CM_ZALLOCATES, args);
	},

	/* ZALLOCATE
	* action:	
	*		call LKREQUEST functions of the requested database objects, followed by LKACQUIRE/LKSUSPEND/LKRESUME, if needed
	*		return true if all databases requested grant the lock within specified or default time limit
	*		return false after sending LKCANCEL to all if timed out or user interrupted
 	*		return OPERATION_FAILURE if error enutil.countered
	* usage:	$locked = $lockmgr->zallocate((db_obj_name, array_of_name_array)+, $time_limit)
	*/
	zallocate: function(db_obj_name, name_array_array){
		args = func_get_args();
		return this.timedLock(CM_ZALLOCATES, args);
	},

	/* functionally equivalent to the previous one, except that it requires only the global variables
	*  leaving the job of finding the corresponding $db_obj_name to gnp_directory m_gld;
	*  e.g. $name_array could be: $a, $b, $c, where $a = array('a',1), $b = array('b'), $c = array('aB')
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
	zallocate_2: function(name_array){
		name_array_array = func_get_args();
		return this.zallocate_3(name_array_array);
	},

	/* functionally equivalent to the previous two, except that it requires only the global variable array
	*  leaving the job of finding the corresponding $db_obj_name to gnp_directory m_gld;
	*  e.g. $name_array_array could be: array($a, $b, $c), where $a = array('a',1), $b = array('b'), $c = array('aB')
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
	zallocate_3: function(name_array_array){
		time_limit = array_pop(name_array_array);
		args = this.addDB(name_array_array);
		array_push(args, time_limit);
		return this.timedLock(CM_ZALLOCATES, args);
	},

	/* ZDEALLOCATE
	* action:	call LKDELETE functions of the requested database objects
	*		return true if all databases requested return LKDELETED
	*		return OPERATION_FAILURE otherwise
	* usage;	$lock_deleted = $lockmgr->zdeallocate((db_obj_name, array_of_name_array)+)
	*/
	zdeallocate: function(db_obj_name, name_array_array){
		args = func_get_args();
		return this.removeLock(CM_ZALLOCATES, args);
	},

	/* functionally equivalent to the previous one, except that it requires only the global variables
	*  leaving the job of finding the corresponding $db_obj_name to gnp_directory m_gld;
	*  e.g. $name_array could be: $a, $b, $c, where $a = array('a',1), $b = array('b'), $c = array('aB')
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
	zdeallocate_2: function(name_array){
		name_array_array = func_get_args();
		return this.zdeallocate_3(name_array_array);
	},

	/* functionally equivalent to the previous two, except that it requires only the global variable array
	*  leaving the job of finding the corresponding $db_obj_name to gnp_directory m_gld;
	*  e.g. $name_array_array could be: array($a, $b, $c), where $a = array('a',1), $b = array('b'), $c = array('aB')
	*	in which case, $a and $c are associated with a.dat ($db1), and $b with mumps.dat ($db2)
	*/
	zdeallocate_3: function(name_array_array){
		args = this.addDB(name_array_array);
		return this.removeLock(m.CM_ZALLOCATES, args);
	},

	/* ZDEALLOCATE_ALL
	* action:	remove all previously obtained locks by way of ZALLOCATE
	*		call LKCANALL functions of the requested database objects
	*		return true if all databases responded with LKDELETED
	*		return OPERATION_FAILURE otherwise
	* usage:	$response = $lockmgr->zdeallocateAll()
	*/
	zdeallocateAll: function(){
		if(util.count(this.m_zallocated_servers))
			return true;

		this.initRequest(false, true, CM_ZALLOCATES, util.count(this.m_zallocated_servers)); 
		for (var serverVal in this) {
                        server = this[serverVal];
			if(res = GLOBALS[server].LKCANALL(m.CM_ZALLOCATES, this.m_transnum))
				return OPERATION_FAILURE;	
		
			this.m_response_cnt++;
		}
		
		(this.m_zallocated_servers);
		this.m_zallocated_servers = {};
		return true;
	},



	/* to be called by each lock operation */
	initRequest: function(transnum_incr, last_canall, laflag, req_cnt, time_limit){
		if (typeof transnum === 'undefined') {
		    transnum = true; }
		if (typeof last_canall === 'undefined') {
		    canall = false; }
		if (typeof laflag === 'undefined') {
		    laflag = m.CM_LOCKS; }
		if (typeof req_cnt === 'undefined') {
		    req_cnt = m_config.F; }
		if (typeof time_limit === 'undefined') {
		    time_limit = 0; }
		if(transnum_incr)
			this.m_transnum++;
		this.m_last_canall = last_canall;
		this.m_laflag = laflag;
		this.m_request_cnt = req_cnt;
		this.m_response_cnt = 0;
		this.m_timeout = time_limit;
		this.m_elapsed_time = 0;
		this.m_interrupted = this.m_out_of_time = false;
		this.m_responded = {};
	},

	
	urgentCancel: function(acq_db){
		if(response = GLOBALS[acq_db].LKURGCANCEL(this.m_transnum))
		{
			return err;
		}
		return true;
	},

	cancel: function(){
		if(util.count(this.m_responded))
		{
			for (var obj_nameVal in this) {
                        obj_name = this[obj_nameVal];
				if(response = GLOBALS[obj_name].LKCANCEL(this.m_laflag, this.m_transnum))
				{
					return err;
				};
			};
		}
		if(util.count(this.m_granted))
		{
			for (var obj_nameVal in this) {
                        obj_name = this[obj_nameVal];
				if(response = GLOBALS[obj_name].LKCANCEL(this.m_laflag, this.m_transnum))
				{
					return err;
				};
			};
		}
		return true;
	},

	// add newly locked gtcm_gnp object name to m_locked_servers, check for redundancy
	addLockedServer: function(new_server){
		if(util.in_array(new_server, this.m_locked_servers))
		{
			this.m_locked_servers.push(new_server);
		}
	},

	// add newly ZALLOCATED gtcm_gnp object name to m_zallocated_servers, check for redundancy
	addZallocatedServer: function(new_server)
	{
		if(util.in_array(new_server, this.m_zallocated_servers))
			{
			this.m_zallocated_servers.push(new_server);
			}
	},

	// request lock immediately, shared by lockAddImmed, lockIncrAddImmed, zallocateImmed
	// returns true if successful, false if aborted, or OPERATION_FAILURE if error occurred
	immediateLock: function(laflag, args){
		var i;
		num_args = util.count(args);
		assert(num_args > 0);
		assert((num_args = 2));
		num_pairs = num_args / 2;
		this.m_granted = {};
		this.initRequest(true, false, laflag, num_pairs, 0);

		for (i = 0; i < num_pairs; i++)
		{
			db_index = i * 2;
			db = args[db_index];			//$db must be a string
			assert(util.get_class(GLOBALS[db]) == 'gtcm_gnp');

			ref_index = db_index + 1;
			ref_array = args[ref_index];	
			if(response = GLOBALS[db].LKREQIMMED(laflag, this.m_transnum, ref_array))
				return OPERATION_FAILURE;
			else
			{
				if(response == m.CMMS_M_LKABORT)
				{
					if(ret = this.cancel())
					{
						return OPERATION_FAILURE;
					}

					return false;
				}else if(response == m.CMMS_M_LKGRANTED)
				{
					this.m_response_cnt++;
					this.m_granted_cnt++;
					this.m_granted.push(db);
					if(this.m_laflag != CM_ZALLOCATES)
						this.addLockedServer(db);
					else this.addZallocatedServer(db);
				}else assert(false);
			};
		}
	
		return true;
	},

	// request lock within t_limit, shared by lockAdd, lockIncrAdd, zallocate
	// returns true if successful, false if aborted, or OPERATION_FAILURE if error occurred
	timedLock:  function(laflag, args){
		num_args = util.count(args);
		assert(num_args, 0);
		if(num_args = 2)
		{
			t_limit = args[(num_args - 1)];
			assert(t_limit, '>1');
			num_pairs = (num_args - 1) / 2;
		}
		else 
		{
			t_limit = CLIENT_WAIT_TIMEOUT;
			num_pairs = num_args / 2;
		}
		this.initRequest(true, false, laflag, num_pairs, t_limit);
		first_to_block = -1;		//$first_to_block holds the index of m_responded array which blocks first
		this.m_granted_cnt = 0;
		this.m_granted = {};

		// make a first attemp anyway
	 for  (var i = 0; i < this.m_request_cnt; i++)
		{
			db_index = i * 2;
			db = args[db_index];
			assert(util.get_class(GLOBALS[db]) == 'gtcm_gnp');

			ref_index = db_index + 1;
			ref_array = args[ref_index];
			if(response = GLOBALS[db].LKREQUEST(laflag, this.m_transnum, ref_array))
			{
				if(ret = this.urgentCancel(db))
				{
		
					return OPERATION_FAILURE;
				}

				if(ret = this.cancel())
				{
		
					return OPERATION_FAILURE;
				}
			
				return OPERATION_FAILURE;
			}else
			{
				this.m_response_cnt++;
				this.m_responded.push(db);
				if(response == m.CMMS_M_LKBLOCKED)
				{
					if(first_to_block == -1)
						first_to_block = util.count(this.m_responded) - 1;
				}else if(response == m.CMMS_M_LKGRANTED)
				{
					this.m_granted_cnt++;
				}else assert(false);
			};
		}

		// if first attemp succeeded, we are all set
		if(this.m_granted_cnt == this.m_request_cnt)
		{
			this.m_granted = this.m_responded;
			for (var serverVal in this) {
                        server = this[serverVal];
				if(this.m_laflag != CM_ZALLOCATES)
					this.addLockedServer(db);
				else this.addZallocatedServer(db);
			}
			return true;
		}

		// otherwise we need more time to do it
		this.m_granted_cnt = 0;
		this.m_granted = {};
		follow_up = {};
	for	(var j = 0; j < util.count(this.m_responded); j++)
		{
			if(j != first_to_block)
				{
				follow_up.push(this.m_responded[j]);
				};
		}
		acq_db = this.m_responded[first_to_block];
		this.initRequest(false, false, this.m_laflag, 
				num_pairs, this.m_timeout);
		
		{
			//send LKACQUIRE to first_to_block and LKSUSPEND to the rest
			for (var sus_serverVal in follow_up) {
                        sus_server = follow_up[sus_serverVal];
				if(sus_res = GLOBALS[sus_server].LKSUSPEND(this.m_laflag))
				{
					this.m_responded = follow_up;
					if(ret = this.urgentCancel(acq_db))
					{
						return OPERATION_FAILURE;
					}

					if(ret = this.cancel())
					{
						return OPERATION_FAILURE;
					}

					return OPERATION_FAILURE;
				}else
				{
					this.m_response_cnt++;
					this.m_responded.push(sus_server);
				}
				if(acq_res = GLOBALS[acq_db].LKACQUIRE(this.m_laflag))
			    {
				if(ret = this.urgentCancel(acq_db))
				{
					return OPERATION_FAILURE;
				}

				if(ret = this.cancel())
				{
					return OPERATION_FAILURE;
				}
				return OPERATION_FAILURE;
			}
	
			this.m_elapsed_time = LOCK_INTERVAL;
			//if previous acquire resulted in false, we keep trying receiving
			if(acq_res == false)
			{
				if(this.m_timeout <= this.m_elapsed_time)
				{
					this.m_out_of_time = true;
					if(ret = this.urgentCancel(acq_db))
					{
						return OPERATION_FAILURE;
					}

					if(ret = this.cancel())
					{
						return OPERATION_FAILURE;
					}
				
					break;
				}
					
				
				{
					if((acq_recv = GLOBALS[acq_db].m_buffer.lockReceive()))
					{
						if(ret = this.urgentCancel(acq_db))
						{
							return OPERATION_FAILURE;
						}

						if(ret = this.cancel())
						{
							return OPERATION_FAILURE;
						}
	
						return OPERATION_FAILURE;
					}
					this.m_elapsed_time += LOCK_INTERVAL;
					this.m_out_of_time = (this.m_timeout <= this.m_elapsed_time);
				}while((acq_recv == true) && (is_null(GLOBALS[acq_db].m_buffer.m_data)) 
					&& (this.m_out_of_time));	
				if(this.m_out_of_time)
				{
					if(mcatch(ret = this.urgentCancel(acq_db)))
					{
				//		mcatch(throwit(, , 'timedLock', ret.getLevel() + 1));
						return OPERATION_FAILURE;
					}

					if(mcatch(ret = this.cancel()))
					{
					//	mcatch(throwit(, , 'timedLock', ret.getLevel() + 1));
						return OPERATION_FAILURE;
					}

					break;
				};
			};
			this.m_granted_cnt++;
			this.m_granted.push(acq_db);
			if(this.m_granted_cnt == this.m_request_cnt) break;
			first_to_block = -1;
			temp_granted = 0;
			suspended = this.m_responded;
			this.initRequest(false, false, this.m_laflag, 
				num_pairs, (this.m_timeout - this.m_elapsed_time));
			for (var serverVal in suspended) {
                        server = suspended[serverVal];
				if(mcatch(response = GLOBALS[server].LKRESUME(this.m_laflag)))
				{
					this.m_responded = suspended;
					if(mcatch(ret = this.urgentCancel(acq_db)))
					{
					//	mcatch(throwit(, , 'timedLock', ret.getLevel() + 1));
						return OPERATION_FAILURE;
					}
					if(mcatch(ret = this.cancel()))
					{
					//	mcatch(throwit(, , 'timedLock', ret.getLevel() + 1));
						return OPERATION_FAILURE;
					}
	
			//		mcatch(throwit(, , 'timedLock', response.getLevel() + 1));
					return OPERATION_FAILURE;
				}else
				{
					this.m_response_cnt++;
					this.m_responded.push(server);
					if(response == m.CMMS_M_LKBLOCKED)
					{
						if(first_to_block == -1)
							first_to_block = util.count(this.m_responded) - 1;
					}else if(response == m.CMMS_M_LKGRANTED)
					{
						temp_granted++;
					}else assert(false);
				};
			}

			if(first_to_block == -1)
			{
				this.m_granted_cnt += temp_granted;
				
				follow_up = {};
			for	(j = 0; j < util.count(this.m_responded); j++)
				{
					if(j != first_to_block)
						follow_up.push(this.m_responded[j]);
				}
				acq_db = this.m_responded[first_to_block];
				this.initRequest(false, false, this.m_laflag, 
					num_pairs, this.m_timeout);
			};

		} while((this.m_granted_cnt < this.m_request_cnt) && (this.m_out_of_time));
		

		if(this.m_granted_cnt == this.m_request_cnt)
		{
			for (var serverVal in this) {
                        server = this[serverVal];
				if(this.m_laflag != CM_ZALLOCATES)
					this.addLockedServer(server);
				else this.addZallocatedServer(server);
			}
			return true;
		}else
	return false;
	}
},

	// delete locks, shared by unlock and zdeallocate
	// returns true on success, OPERATION_FAILURE on failure
	removeLock: function(laflag, args){
		num_args = util.count(args);
		assert(num_args > 0);
		assert(num_args = 2);
		num_pairs = num_args / 2;
		this.initRequest(false, false, laflag, num_pairs, 0);

	for(var i = 0; i < num_pairs; i++)
		{
			db_index = i * 2;
			db = args[db_index];			//$db must be a string
			assert(util.get_class(GLOBALS[db]) == 'gtcm_gnp');

			ref_index = db_index + 1;
			ref_array = args[ref_index];	
			if(response = GLOBALS[db].LKDELETE(laflag, this.m_transnum, ref_array))
			{
				return OPERATION_FAILURE;
			}
			else if(response == m.CMMS_M_LKDELETED)
			{
				this.m_response_cnt++;
			}
			else assert(false);
		}
		return true;
	},
	
	// transforms array($global1, $global2,...) into array('db1', array($global1, $global2), 'db2', ...)
	addDB: function(name_array_array){
		assert(is_null(this.m_gld));

		for (var name_arrayVal in name_array_array) {
                        name_array = name_array_array[name_arrayVal];
			//find out which gtcm_gnp object each belongs to

			db_name = this.m_gld.lookUp(name_array[0]);

			keys = array_keys(arr);
			if(util.in_array(db_name, keys))
				arr[db_name].push(name_array);
			else
			{
				arr[db_name] = array(name_array);
			}
		}
		ret_arr = {};
		for (var db in arr) {
                        gvns = arr[db];
			gvns = array(gvns);
			ret_arr.push(db);
			ret_arr.push(gvns);
		}
		return ret_arr;
		},
	});
	
