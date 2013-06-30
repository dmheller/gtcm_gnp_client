

var tcp_socket = require('./tcp_socket');
var c = require('./constants');
var m = require('./util');
var gnp_util = require('./gnp_util');
var gnp_buffer = require('./gnp_buffer');
var _ = require('underscore')._;
var defineClass = require("./defineClass.js").defineClass;

var gtcm_gnp = defineClass({ 
constructor: function(dat_file, server, port){


	/* User provided */
	 m_server; 		//must always be a string, provided by user
	 m_port;   		//must always be a number, provided by user
	 m_dat_file;	//database file name, may be on a remote machine, format???, provided by user

	/* gtcm_gnp provided */
	 m_socket;		//the socket over which communication is taking place
	 m_buffer;		//deals with message formatting and send/receive issues
	 m_proc_num;	//process number returned from INITPROC call
	 m_open_region;	//REGION INFO of the current region
	 m_cur_region_no;	//number of region currently in use, member of REGION_INFO type, returned from INITREG call


	/* Constructor
	*  action:	initialize class data members
	*  usage: 	$obj = new gtcm_gnp()
	*  notes:	requires $dat_file as a string correctly formatted to suit the platform
	*/
	  
	 
		if (typeof server === 'undefined') {
		    server = '127.0.0.1';}
		if (typeof port === 'undefined') {
		    port = 30000;}
		if(typeof dat_file === 'undefined')
		{
			return false;
		}
		
	var	socket = new tcp_socket(server, port); 
		if(socket) 
			return false;
	var	buffer = new gnp_buffer(socket);
		if(buffer)
			return false;
		proc_num = buffer.doop(CMMS_S_INITPROC);
		if(typeof proc_num === 'undefined')
			return false;
		new_region = buffer.doop(CMMS_S_INITREG, dat_file);
		if(typeof new_region === 'undefined')
			return false;

		this.m_socket = socket;
		this.m_buffer = buffer;
		this.m_dat_file = dat_file;
		this.m_server = server;
		this.m_port = port;
		this.m_proc_num = proc_num;
		this.m_cur_region_no = new_region.m_region_no;
		this.m_open_region = new_region;

		
	},

	/* GET
	*  action:	parse input, instantiate a SUBSC object and call buffer doop function
	*  		return the value if successful
			return OPERATION_FAILURE if other non-fatal error occurred.
	*  usage:	$value = $this->GET(array of name and subscripts)
	*  note:	make sure the correct region is being used
	*/
	GET: function(name_array){
		num_args = func_num_args();
		if(num_args < 1)
			return this.argError(1, 'GET', '');
		
		value = this.m_buffer.doop(CMMS_Q_GET, this.makeSUBSC(name_array));
		if (typeof value === 'undefined'){	
		return OPERATION_FAILURE;};
			
		if(is_null(value))
		{
			err_msg = 'Global reference undefined in GET().';
			console.log(err_msg);
			return OPERATION_FAILURE;
		}
		else
			return value;		
	},

	/* ORDER
	*  action:	parse input, instantiate a SUBSC object and call buffer doop function
	*  		return the value if successful
			return OPERATION_FAILURE if other non-fatal error occurred.
	*  usage:	$value = $this->ORDER(array of name and subscripts)
	*  note:	all global gtcm_gnp objects will be examined
	*/
	ORDER: function(name_array){
		num_args = func_num_args();
		if(num_args < 1)
			return this.argError(1, 'ORDER','' );	

		lowest = -99;
		for (var key in GLOBALS) {
                        value = GLOBALS[key];
			if((is_object(value)) && (util.get_class(value) == 'gtcm_gnp'))
			{	
				value = GLOBALS[key].m_buffer.doop(CMMS_Q_ORDER, this.makeSUBSC(name_array));
				if(typeof value === 'undefined')
				{
					return OPERATION_FAILURE;
				}
				
				if(lowest == -99)
				{
					lowest = value;
				}
				else if(is_null(value.m_gds_key))
				{
					if(is_null(lowest.m_gds_key) || value.before(lowest))
						lowest = value;
				}
			}
		}
		return lowest;					
	},

	/* PREV
	*  action:	parse input, instantiate a SUBSC object and call buffer doop function
	*  		return the value if successful
			return OPERATION_FAILURE if other non-fatal error occurred.
	*  usage:	$value = $this->PREV(array of name ans subscripts)
	*  note:	all global gtcm_gnp objects will be examined
	*/
	PREV: function(name_array){
		num_args = func_num_args();
		if(num_args < 1)
			return this.argError(1, 'PREV','' );
	
		highest = -99;	
		for (var key in GLOBALS) {
                        value = GLOBALS[key];
			if((is_object(value)) && (util.get_class(value) == 'gtcm_gnp'))
			{	
				value = GLOBALS[key].m_buffer.doop(CMMS_Q_PREV, this.makeSUBSC(name_array));
				if(typeof value === 'undefined')
					return OPERATION_FAILURE;
			
				if(highest == -99)
					highest = value;
				else if(is_null(value.m_gds_key))
				{
					if(is_null(highest.m_gds_key) || value.before(highest))
						highest = value;
				}

			}
		}
	
		return highest;			
	},

	/* DATA
	*  action:	parse input, instantiate a SUBSC object and call buffer doop function
	*  		return the value if successful
			return OPERATION_FAILURE if other non-fatal error occurred.
	*  usage:	$value = $this->DATA(array of name and subscripts)
	*  note:	make sure the correct region is being used
	*/
	DATA: function(name_array){
		num_args = func_num_args();
		if(num_args < 1)
			return this.argError(1, 'DATA','' );
		
		value = this.m_buffer.doop(CMMS_Q_DATA, this.makeSUBSC(name_array));
		if(typeof value === 'undefined')
			return OPERATION_FAILURE;
		return value;			
	},

	/* KILL
	*  action:	parse input, instantiate a SUBSC object and call buffer doop function
	*  		return true if successful
			return OPERATION_FAILURE if other non-fatal error occurred.
	*  usage:	$value = $this->KILL(array of name and subscripts)
	*  note:	make sure the correct region is being used
	*/
	KILL:  function(name_array){
		num_args = func_num_args();
		if(num_args < 1)
			return this.argError(1, 'KILL','' );		
		
		value = this.m_buffer.doop(CMMS_Q_KILL, this.makeSUBSC(name_array));
		if(typeof value === 'undefined')
			return OPERATION_FAILURE;
		return value;			
	},

	/* PUT
	*  action:	parse input, instantiate a SUBSC object and call buffer doop function
	*  		return true if successful
			return OPERATION_FAILURE if other non-fatal error occurred.
	*  usage:	$value = $this->PUT($val, array of name and subscripts)
	*  note:	make sure the correct region is being used
	*/
	PUT: function(name_array, val){
		num_args = func_num_args();
		if(num_args < 2)
			return this.argError(2, 'PUT','' );		
		
		value = this.m_buffer.doop(CMMS_Q_PUT, this.makeSUBSC(name_array), val);
		if(typeof value === 'undefined')
			return OPERATION_FAILURE;
		return value;			
	},

	/* QUERY
	*  action:	parse input, instantiate a SUBSC object and call buffer doop function
	*  		return the value if successful
			return OPERATION_FAILURE if other non-fatal error occurred.
	*  usage:	$value = $this->QUERY(array of name and subscripts)
	*  note:	all global gtcm_gnp objects will be examined
	*/
	QUERY: function(name_array){
		num_args = func_num_args();
		if(num_args < 1)
			return this.argError(1, 'QUERY','' );

		value = this.m_buffer.doop(CMMS_Q_QUERY, this.makeSUBSC(name_array));
		if(typeof value === 'undefined')
					return OPERATION_FAILURE;
		return value;			
	},

	/* WITHDRAW
	*  action:	parse input, instantiate a SUBSC object and call buffer doop function
	*  		return true if successful
			return OPERATION_FAILURE if other non-fatal error occurred.
	*  usage:	$value = $this->WITHDRAW(array of name and subscripts)
	*  note:	make sure the correct region is being used
	*/
	WITHDRAW: function(name_array){
		num_args = func_num_args();
		if(num_args < 1)
			return this.argError(1, 'WITHDRAW','' );
		
		value = this.m_buffer.doop(CMMS_Q_ZWITHDRAW, this.makeSUBSC(name_array));
		if(typeof value === 'undefined')
			return OPERATION_FAILURE;
		return value;			
	},
	
	/* TERMINATE
	*  action:	call buffer doop function
	*  		return true if successful
	*  usage:	$value = $this->TERMINATE()
	*  note:	no response from the server is expected
	*/
	TERMINATE: function()
	{
		value = this.m_buffer.doop(CMMS_S_TERMINATE);
		if(typeof value === 'undefined')
			return OPERATION_FAILURE;
		return true;
	},

	/* DESTROY 	(as destructor, but only to be called, if before_exit is not registered shutdown function)
	*  action:	send TERMINATE message,	close the TCP socket, unset all data members
	*  usage:	$this->destroy()
	*/
	destroy: function()
	{
		this.TERMINATE();
		this.cleanUp();
	},

	
	/* LKREQUEST
	*  action:	parse input, instantiate an array of SUBSC object and call buffer doop function
	*  		return CMMS_M_LKGRANTED or CMMS_M_LKBLOCKED message if successful
	*  usage:	$value = $this->LKREQUEST($laflag, $transnum, aray of array of name and subscripts)
	*  note:	make sure the correct region is being used
	*/
	LKREQUEST: function(laflag, transnum, name_array_array){
		num_args = func_num_args();
		if(num_args < 3)
			return this.argError(3, 'LKREQUEST','' );
		
		state = this.m_buffer.doop(CMMS_L_LKREQUEST, laflag, transnum, this.makeSUBLIST(name_array_array));
		if(typeof state === 'undefined')
		{
			return err;
		}

		return state;			
	},

	/* LKREQNODE
	*  action:	parse input, instantiate an array of SUBSC object and call buffer doop function
	*  		return true if successful
	*  usage:	$value = $this->LKREQNODE($laflag, $transnum, aray of array of name and subscripts)
	*  note:	make sure the correct region is being used
	*/
	LKREQNODE: function(laflag, transnum, name_array_array){
		num_args = func_num_args();
		if(num_args < 3)
			return this.argError(3, 'LKREQNODE','' );
		
		state = this.m_buffer.doop(CMMS_L_LKREQNODE, laflag, transnum, this.makeSUBLIST(name_array_array));
		if(state === 'undefined')
		{
			return err;
		}
		return true;
	},

	/* LKREQIMMED
	*  action:	parse input, instantiate an array of SUBSC object and call buffer doop function
	*  		return CMMS_M_LKGRANTED or CMMS_M_LKABORT message if successful
	*  usage:	$value = $this->LKREQIMMED($laflag, $transnum, aray of array of name and subscripts)
	*  note:	make sure the correct region is being used
	*/
	LKREQIMMED: function(laflag, transnum, name_array_array){
		num_args = func_num_args();
		if(num_args < 3)
			return this.argError(3, 'LKREQIMMED','' );

		state = this.m_buffer.doop(CMMS_L_LKREQIMMED, laflag, transnum, this.makeSUBLIST(name_array_array));
		if(state === 'undefined')
		{
			return err;
		}
		return state;			
	},

	/* LKDELETE
	*  action:	parse input, instantiate an array of SUBSC object and call buffer doop function
	*  		return CMMS_M_LKDELETED message if successful
	*  usage:	$value = $this->LKDELETE($laflag, $transnum, aray of array of name and subscripts)
	*  note:	make sure the correct region is being used
	*/
	LKDELETE: function(laflag, transnum, name_array_array){
		num_args = func_num_args();
		if(num_args < 3)
			return this.argError(3, 'LKDELETE','' );
		
		state = this.m_buffer.doop(CMMS_L_LKDELETE, laflag, transnum, this.makeSUBLIST(name_array_array));
		if(state === 'undefined')
		{
			return err;
		}

		return state;			
	},

	/* LKSUSPEND
	*  action:	
	*  		return CMMS_M_LKSUSPENDED if successful
	*  usage:	$value = $this->LKSUSPEND($laflag)
	*  note:	make sure the correct region is being used
	*/
	LKSUSPEND: function(laflag){
		num_args = func_num_args();
		if(num_args < 1)
			return this.argError(1, 'LKSUSPEND','' );
		
		state = this.m_buffer.doop(CMMS_L_LKSUSPEND, laflag);
		if(state === 'undefined')
		{
			return err;
		}

		return state;			
	},

	/* LKRESUME
	*  action:		
	*  		return CMMS_M_LKGRANTED or CMMS_M_LKBLOCKED if successful
	*  usage:	$value = $this->LKRESUME($laflag)
	*  note:	make sure the correct region is being used
	*/
	LKRESUME: function(laflag){
		num_args = func_num_args();
		if(num_args < 1)
			return this.argError(1, 'LKRESUME','' );
		
		state = this.m_buffer.doop(CMMS_L_LKRESUME, laflag);
		if(state === 'undefined')
		{
			return err;
		}

		return state;			
	},

	/* LKACQUIRE
	*  action:		
	*  		return CMMS_M_LKGRANTED if successful
	*		return false if no data was read from the socket (need more time)
	*  usage:	$value = $this->LKACQUIRE($laflag)
	*  note:	make sure the correct region is being used
	*/
	LKACQUIRE: function(laflag){
		num_args = func_num_args();
		if(num_args < 1)
			return this.argError(1, 'LKACQUIRE','' );
		
		state = this.m_buffer.doop(m.CMMS_L_LKACQUIRE, laflag);
		if(state === 'undefined')
		{
			return err;
		}

		return state;			//this could be false when no data is received
	},

	/* LKCANCEL
	*  action:	send LKCANCEL message via normal channel, c.f. LKURGCANCEL	
	*  		return CMMS_M_LKDELETED if successful
	*  usage:	$value = $this->LKCANCEL($laflag, $transnum)
	*  note:	make sure the correct region is being used
	*/
	LKCANCEL: function(laflag, transnum){
		num_args = func_num_args();
		if(num_args < 2)
			return this.argError(2, 'LKCANCEL','' );
		
		state = this.m_buffer.doop(m.CMMS_L_LKCANCEL, laflag, transnum);
		if(state === 'undefined')
		{
			return err;
		}

		return state;			
	},

	/* LKURGCANCEL 	(not part of GNP protocol, but crucial for the GTCM implementation)
	*  action:	send LKINTERRUPT message via urgent channel, c.f. LKCANCEL	
	*  		return CMMS_M_DELETED if successful
	*  usage:	$value = $this->LKURGCANCEL($transnum)
	*  note:	make sure the correct region is being used
	*/
	LKURGCANCEL: function(transnum){
	{
		num_args = func_num_args();
		if(num_args < 1)
			return this.argError(1, 'LKURGCANCEL','' );
		
		state = this.m_buffer.doop(CMMS_S_INTERRUPT, transnum);
		if(state === 'undefined')
		//	err = throwit(, , 'LKURGCANCEL', state.getLevel()  1);
			return err;
		}

		return state;			
	},
		
	/* LKCANALL
	*  action:	sent when unconditional lock cancel is needed 	
	*  		return CMMS_M_LKDELETED if successful
	*  usage:	$value = $this->LKCANALL($laflag, $transnum)
	*  note:	make sure the correct region is being used
	*/
	LKCANALL: function(laflag, transnum){
		num_args = func_num_args();
		if(num_args < 2)
			return this.argError(2, 'LKCANALL','' );
		
		state = this.m_buffer.doop(CMMS_L_LKCANALL, laflag, transnum);
		if(state === 'undefined')
		{
			return err;
		}

		return state;			
	},


	/* Create a gds object from an array of global references
	*  action:	transform $array into a gds object, only if $array is not already a SUBSC
	*  usage:	$sub = $this->makeSUBSC($name_array)
	*/
	makeSUBSC: function(name_array){
		if((is_object(name_array)) && (util.get_class(name_array) == 'subsc'))
		{
			return name_array;
		}
		gds_key_seed = {};
		assert.that(is_array(name_array));
		gds_key_seed = name_array;
		gds_key = new gds(gds_key_seed);
		subsc = new SUBSC(this.m_cur_region_no, gds_key);
		return subsc;
	},

	/* Create an array of gds object from an array of global references arrays
	*  action:	transform $array into a gds object, assemble an array of gds objects
	*  usage:	$sub = $this->makeSUBLIST($name_array_array)
	*  note:	the list is in reverse order for lock messages
	*/
	makeSUBLIST: function(name_array_array){
		subList = {};
		name_array_array = array(name_array_array);
		for(var n = 0; n < count(name_array_array); n++){
			subList.push = this.makeSUBSC(n);};
			subList_rev = array_reverse(sublist, false);
			return subList_rev;
	},

	/* CLEANUP
	*  action:	close the TCP socket, unset all data members
	*  usage:	$this->cleanUp()
	*/
	argError: function(num_args, func_name, line){
		err_msg = "Invalid Argument Error: " + func_name + "() requires at least " + num_args + " argument(s),<br>\n";
		console.log(err_msg);
		return OPERATION_FAILURE;
	},
	cleanUp: function(){
		this.m_socket.closeSocket();
		unset(this.m_server);
		unset(this.m_port);
		unset(this.m_dat_file);
		unset(this.m_timeOut);
		unset(this.m_socket);
		unset(this.m_buffer);
		unset(this.m_proc_num);
		unset(this.m_open_region);
		unset(this.m_cur_region_no);
},
});

