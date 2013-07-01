
var assert = require('node-assertthat');
var m = require('./constants');
var gnp_util = require('./gnp_util');
var tcp_socket = require('./tcp_socket');
var util = require('./util');
var m_data;		//string holding the translated data to be read or raw data to be written
var m_index;		//byte length index
var m_last_request;	//remember the last request so as to verify the response	
var m_protocol = new PROTOCOL(); //the PROTOCOL being used in INITPROC message
var _ = require('underscore')._;
var defineClass = require("./defineClass.js").defineClass;

var gnp_buffer = defineClass({
	constructor: function(socket){

	
	this.init();
	this.m_socket = socket;
	this.m_last_request;

},

	
	doop: function(op_code) {
		if(op_code != m.CMMS_S_INTERRUPT)
		{	this.putHDR(op_code);
		this.m_last_request = op_code;}

		if((op_code >= m.CMMS_L_LKCANALL) && (op_code <= m.CMMS_L_LKSUSPEND))
		{
			laflag = func_get_arg(1);
			this.putLAFLAG(laflag);
		}

		if((op_code == m.CMMS_L_LKCANALL) || 
		    (op_code == m.CMMS_L_LKCANCEL))
		{
			transnum = func_get_arg(2);
			this.putTRANSNUM(transnum);
		}
		else if(op_code == m.CMMS_S_INTERRUPT)
		{
			transnum = func_get_arg(1);
			this.putTRANSNUM(transnum);
		}
		else if((op_code == m.CMMS_L_LKREQUEST) ||
			(op_code == m.CMMS_L_LKREQNODE) ||
			(op_code == m.CMMS_L_LKREQIMMED) ||
			(op_code == m.CMMS_L_LKDELETE))
		{
			transnum = func_get_arg(2);
			sublist = func_get_arg(3);
			assert(is_array(sublist));
			num_nodes = util.count(sublist);
			this.putTRANSNUM(transnum);
			this.putSUBLIST(num_nodes, sublist);
		} 		
		else if(op_code == m.CMMS_S_INITPROC)
		{
			this.putPROTOCOL();
		}
		else if(op_code == m.CMMS_S_INITREG)
		{
			reg_name = func_get_arg(1);
			this.putREGNAME(reg_name);
		}
		else if(op_code == m.CMMS_S_TERMINATE)
		{
		}
		else if((op_code == m.CMMS_Q_KILL) ||
			(op_code == m.CMMS_Q_PUT) ||
			(op_code == m.CMMS_Q_GET) ||
			(op_code == m.CMMS_Q_ORDER) ||
			(op_code == m.CMMS_Q_PREV) ||
			(op_code == m.CMMS_Q_DATA) ||
			(op_code == m.CMMS_Q_QUERY) ||
			(op_code == m.CMMS_Q_ZWITHDRAW))
		{
			subsc = func_get_arg(1);
			this.putSUBSC(subsc);
		}
		else if ((op_code == m.CMMS_L_LKACQUIRE) || (op_code == m.CMMS_L_LKRESUME) 
			|| (op_code == m.CMMS_L_LKSUSPEND))
		{}
		else
			assert(false);
		
		if(op_code == m.CMMS_Q_PUT)
		{
			val = func_get_arg(2);
			this.putVALUE(val);
		}

		if((op_code == m.CMMS_S_TERMINATE) || (op_code == m.CMMS_L_LKREQNODE))	
			ok = this.send();
		else if(op_code == m.CMMS_L_LKACQUIRE)
			ok = (this.send() && this.receive());
		else if(op_code == m.CMMS_S_INTERRUPT)
			ok = (this.urgentSend() && this.receive());
		else
			ok = (this.send() && this.receive());

		if((op_code == m.CMMS_S_TERMINATE)|| (op_code == m.CMMS_L_LKREQNODE))
		{
			if(!ok)
			{
				if(op_code == m.CMMS_S_TERMINATE)
					func = 'TERMINATE()';
				else func = 'LKREQNODE()';
				err_msg = func + 'failed. Message not sent.';
				console.log(err_msg);
				return err;
			}
			else
				return true;
		}
		else if((op_code == m.CMMS_Q_KILL) || 
			(op_code == m.CMMS_Q_PUT) || 
			(op_code == m.CMMS_Q_ZWITHDRAW))
		{
			if(!ok)
			{
				err_msg = 'Operation (' + op_code + ') failed.';
				console.log(err_msg);
				return err;
			}
			else
				return true;
		}
		else if(op_code == m.CMMS_L_LKACQUIRE)
		{
			if(!ok)
			{
				err_msg = 'lock(' + op_code + ') failed.<br.\n';
				console.log(err_msg);
				return err;
			}
			else if(is_null(this.m_data))
			{
				return false;
			}
			else
			{
				return this.m_data;			//contains CMMS_M_'s
			}
		}
		else
		{
			if(!ok)
			{
				err_msg = 'Operation (' + op_code + ') failed.';
				console.log(err_msg);
				return err;	
			}
			else
			{
				return this.m_data;
			}
		}
	},



	/* send() returns true if successful
	 * returns false otherwise
	 */
	send: function(){
		//NEW: add 2 bytes to each message, indicating the length of the message,in BIG-ENDIAN mode
		preHDR = this.getLength();
		preHDR_packed = pack('n', preHDR);
		this.m_data = preHDR_packed + this.m_data;
		gnp_msg_len = preHDR + GNP_PREHDR_LEN;
		
		neg_errno = socket_write(this.m_socket.getSocket(), this.m_data, gnp_msg_len);
	
		if(neg_errno === false)
		{
			err_msg = strerror(-neg_errno);	
			console.log(err_msg);
			return false;
		}
		else
		{
			return true;
		}
	},

	/* urgentSend(), used to send LKCANCEL messages, returns true if successful 
	 * returns false otherwise
	 */
	urgentSend: function(){
		

		neg_errno = socket_send(this.m_socket.getSocket(), this.m_data, 1, MSG_OOB);
	
		if(neg_errno < 0)
		{
			err_msg = strerror(-neg_errno);	
			console.log(err_msg);	
			return false;
		}
		else
		{
			
			return true;
		}
	},

	/* receive() returns true if successful
	 * returns false otherwise
	 */
	receive: function(){
		 g_endian_convert;
		 g_endian;
		 g_self_big_endian;

		

		//first, skip the 2 bytes in big endian order
		skipped = this.getPreHDR();
		assert(skipped == GNP_PREHDR_LEN);

		// read the one byte HDR
		inBytes = this.getHDR();
		if(!inBytes)
		{
			err_msg = 'No header received, check network connection.';
			console.log(err_msg);	
			return false;
		}else
		{	
			//force $header to be integer
			hdr = 0;
			hdr += this.m_data;

			if(this.matchRequest(hdr))
			{
				err_msg = 'request/response type mismatch. Request = ' +
				this.m_last_request + ' Response = ' + hdr;
				console.log(err_msg);
				return false;
			}
        switch(hdr)
	    {
		case m.CMMS_M_LKABORT:
		case m.CMMS_M_LKBLOCKED:
		case m.CMMS_M_LKGRANTED:
		case m.CMMS_M_LKDELETED:
		case m.CMMS_M_LKSUSPENDED: break;
		case m.CMMS_T_INITPROC:   
							if(!this.getPROTOCOL()) 
								return false;
							
							if(this.m_protocol.matchEndian(this.m_data))
								g_endian_convert = true;
							else
								g_endian_convert = false;

							if(g_endian_convert)
							g_endian = (g_self_big_endian, 'B',  'L');
							else
							g_endian = (g_self_big_endian, 'L',  'B');

							if(!this.m_protocol.match(this.m_data))
							{
								err_msg = 'PROTOCOL mismatch.';
								console.log(err_msg);
				
								return false;
							}							
							if(!this.getPROCNUM()) return false;
							break;

		case	m.CMMS_T_REGNUM:	if(this.getREGINFO()) return false;
							break;

		case	m.CMMS_R_GET:	if(this.getVALUE()) return false;
							break;

		case	m.CMMS_R_UNDEF:	if(this.m_last_request == m.CMMS_Q_GET)
							{
								this.m_data = null;
								break;
							}
							else
							{
								err_msg = 'GTCM_GVN value reference undefined.';
							    console.log(err_msg);
								return false;
							}

		case	m.CMMS_R_ORDER:	if(this.getSUBSC()) return false;
							break;

		case	m.CMMS_R_PREV:	if(this.getSUBSC()) return false;
							break;

		case	m.CMMS_R_DATA:	if(this.getDATA()) return false;
							break;

		case	m.CMMS_R_KILL:	
		case	m.CMMS_R_PUT:	break;

		case   m.CMMS_R_QUERY:	if(this.getSUBSC()) return false;
							break;

		case	m.CMMS_R_ZWITHDRAW:	break;
					
		case	m.CMMS_E_ERROR:	if(this.getERROR()) return false;
							err_msg = 'From server: ' + this.m_data;
						    console.log(err_msg);
							return false;

		case	m.CMMS_E_TERMINATE:	if(this.TERMINATE()) return false;
							break;

				
					assert (false);
			};
		};

		return true;
	},
	
	/* lockReceive() returns true if CMMS_M was received. It fills m_data with CMMS_M_ message
	 *		 returns true if CMMS_M was not received and no error occurred. It sets m_data to null
	 * 		returns  false otherwise
	 * note: 	nonblocking mode
	 */
	lockReceive: function(){
		sleep(LOCK_INTERVAL);		
		//first, skip the 2 bytes in big endian order and get the HDR
		this.init();
		
		inBytes = socket_read(this.m_socket.getSocket(), (GNP_PREHDR_LEN + 1));
		document.write( 'Success');

		if(inBytes <= 0)
		{
			this.m_data = null;
			return true;
		}
		
		//data is coming, so pick them up all at once
		if((inBytes > 0) && (inBytes < (GNP_PREHDR_LEN + 1)))	
		{
			mesg = 'lock_receiv() : Message is coming.<br>\n';
			
			ibuf = buf;
			alreadyIn = strlen(buf);
			{
					buf = '';
					buf = socket_read(this.m_socket.getSocket(), (GNP_PREHDR_LEN + 1 - alreadyIn));
					ibuf +=  buf;
                                        alreadyIn += inBytes;
                                }while(alreadyIn < (GNP_PREHDR_LEN + 1));
                                inBytes = alreadyIn;
                                buf = ibuf;
				this.m_socket.setNonBlock();
				document.write( 'Success');
			}
			else
			{
				
				{
					buf = '';
					buf = socket_read(this.m_socket.getSocket(), (GNP_PREHDR_LEN + 1 - alreadyIn));
                  			ibuf +=  buf;
					alreadyIn += inBytes;
				}
				while(alreadyIn < (GNP_PREHDR_LEN + 1));
				inBytes = alreadyIn;
				buf = ibuf;
				document.write( 'SUCCESS');
			 }
		assert(inBytes == (GNP_PREHDR_LEN + 1));
		hdr_packed = substr(buf, 2, 1);
		this.m_data = this.getUByte(hdr_packed);

		//force $header to be integer
		hdr = 0;
		hdr += this.m_data;

		
		if(this.matchRequest(hdr))
		{
			err_msg = 'request/response type mismatch. Request = ' +
				this.m_last_request + ' Response = ' + hdr;
			console.log(err_msg);
			return false;
		};

		switch(hdr)
		{
		case   m.CMMS_M_LKABORT:
		case   m.CMMS_M_LKBLOCKED:
		case   m.CMMS_M_LKGRANTED:
		case   m.CMMS_M_LKDELETED:
		case   m.CMMS_M_LKSUSPENDED: break;
							
		case   m.CMMS_E_ERROR:	if(SOCKET_ALWAYS_NONBLOCK == 0)
							this.m_socket.setBlock();
						if(this.getERROR())
							return false;
						err_msg = 'From server: ' + this.m_data;
						console.log(err_msg);
						return false;

		case	 m.CMMS_E_TERMINATE:  if(SOCKET_ALWAYS_NONBLOCK == 0)
							this.m_socket.setBlock();
						if(this.TERMINATE()) 
							return false;
								
						if(SOCKET_ALWAYS_NONBLOCK == 0)
							this.m_socket.setNonBlock();
						break;
			
				if(SOCKET_ALWAYS_NONBLOCK == 0)
					this.m_socket.setBlock();
				assert (false);
		}

		if(SOCKET_ALWAYS_NONBLOCK == 0)
			this.m_socket.setBlock();
		return true;
	},

	init: function()
	{
		this.m_index = 0;
		this.m_data = '';
	},

	/* for debugging */
	show: function(){
		if ((this.m_data))
			document.write( 'Buffer empty!<br>\n');
		else
		{
			document.write( 'Buffer length = ', strlen(this.m_data), '<br>\n');
			document.write( 'Buffer contains: <br>\n');
		
			encoded = unpack('C*code', this.m_data);
		for	(var i = 1; i <= util.count(encoded); i++)
			{
				sub = 'code' + i;
				data = dechex(encoded[sub]);
				j = i - 1;
				
				document.write( 'm_data [' + j + ']: ' + data);
				document.write( '<br>\n');
			};
		
		};
	},

	/* can only be used with messages to be sent */
	getLength: function(){
		return this.m_index;
	},

	/* used with both messsages to be sent and received */
	getRealLength: function()
	{
		return strlen(this.m_data);
	},

	matchRequest: function(response){
		//CMMS_R type
		if((this.m_last_request >= m.CMMS_Q_DATA) && (this.m_last_request <= CMMS_Q_ZWITHDRAW))
		{
			if((response == this.m_last_request + 8) || (response == m.CMMS_R_UNDEF)
				|| (response == m.CMMS_E_ERROR) || (response == m.CMMS_E_TERMINATE))
				return true;
			else return false;
		}

		//CMMS_S type
		
		if((this.m_last_request >= m.CMMS_S_INITPROC) && (this.m_last_request <= m.CMMS_S_INITREG))
		{
			if((response == this.m_last_request + 4)
				|| (response == m.CMMS_E_ERROR) || (response == m.CMMS_E_TERMINATE))
				return true;
			else return false;
		}

		//locks
	
		if(this.m_last_request == m.CMMS_S_INTERRUPT)	//urgentCancel
		{
			if((response == m.CMMS_M_LKDELETED) || (response == m.CMMS_E_ERROR)
				|| (response == m.CMMS_E_TERMINATE))
				return true;
			else return false;
		}
	
		if((this.m_last_request >= CMMS_L_LKCANALL) && (this.m_last_request <= CMMS_L_LKDELETE ))
		{
			if((response == m.CMMS_M_LKDELETED) || (response == m.CMMS_E_ERROR)
				|| (response == m.CMMS_E_TERMINATE))
				return true;
			else return false;
		}

		if(this.m_last_request == m.CMMS_L_LKREQIMMED)
		{
			if((response == m.CMMS_M_LKGRANTED) || (response == m.CMMS_M_LKABORT)
				|| (response == m.CMMS_E_ERROR) || (response == m.CMMS_E_TERMINATE))
				return true;
			else return false;
		}

		if((this.m_last_request == m.CMMS_L_LKREQUEST) || (this.m_last_request == m.CMMS_L_LKRESUME))
		{
			if((response == m.CMMS_M_LKGRANTED) || (response == m.CMMS_M_LKBLOCKED)
				|| (response == m.CMMS_E_ERROR) || (response == m.CMMS_E_TERMINATE))
				return true;
			else return false;
		}

		if(this.m_last_request == m.CMMS_L_LKACQUIRE)
		{
			if((response == m.CMMS_M_LKGRANTED) 
				|| (response == m.CMMS_E_ERROR) || (response == m.CMMS_E_TERMINATE))
				return true;
			else return false;
		}

		if(this.m_last_request == m.CMMS_L_LKSUSPEND)
		{
			if((response == m.CMMS_M_LKSUSPENDED) 
				|| (response == m.CMMS_E_ERROR) || (response == m.CMMS_E_TERMINATE))
				return true;
			else return false;
		}

		return false;		
	},


	/* unsigned char, for one byte integer */
	putUByte: function(int_data){	
		assert(is_integer(int_data));
		this.m_data +=  pack('C', int_data);
		this.m_index++;
	},

	getUByte: function(bin_data)	{
		arr = unpack('Cfirst', bin_data);
		return arr['first'];
	},
		

	/* unsigned char, for character */
	putUChar: function(char_data){
		assert((util.ord(char_data), '>=0') && (util.ord(char_data < 255)));
		this.m_data +=  pack('C', util.ord(char_data));
		this.m_index++;
	},

	getUChar: function(bin_data){
		arr = unpack('Cdata', bin_data);
		return chr(arr['data']);
	},
		
	/* for string, optionally padded with \0 */
	putString: function(str_data, pad_len){
		if (typeof pad_len === 'undefined') {
		    pad_len = 0;}
		len = strlen(str_data);
		pack_len = len + pad_len;

		template = 'a' + pack_len;
		list = str_data;

		this.m_data +=  pack(template, list);
		this.m_index += pack_len;
	},

	getString: function(bin_data, length, trim){
		
		realLen = strlen(bin_data);
		if(realLen != length)
			length = realLen;
		
		template = 'a' + length + 'string';
		arr = unpack(template, bin_data);
		
		if(trim == 0)
			return arr['string'];
		if(trim == -1)
			return ltrim(arr['string']);
		if(trim == 1)
			return rtrim(arr['string']);
		if(trim == 2)
			return trim(arr['string']);
	},

	/* for string, optionally padded with spaces */
	putAString: function(str_data, pad_len){
		if (typeof pad_len === 'undefined') {
		    pad_len = 0;}
		len = strlen(str_data);
		pack_len = len + pad_len;

		template = 'A' + pack_len;
		list = str_data;

		this.m_data +=  pack(template, list);
		this.m_index += pack_len;
	},
	
	/* for variable length string */
	putVLString: function(vString){
		len = strlen(vString);
		
		//indicate the length of the string first
		if(VARIABLE_STRING_OVERHEAD == 4)
		{
			this.putULong(len);
		}
		else
		{
			//legacy variable length from VMS is 16 bits
			this.putUShort(len);			
		}
		
		//now append the string itself
		this.putString(vString);
	},

	/* unsigned long, for 32 bit integer */
	putULong: function(long_data){
		 g_ulong_template;
		 g_endian;
		if((g_endian))
			g_endian = 'D';
		
		template = g_ulong_template[g_endian];
		this.m_data +=  pack(template, long_data);
		// 32 bits = 4 bytes
		this.m_index += 4;		
	},

	getULong: function(bin_data){
		 g_ulong_template;
		 g_endian;
		if((g_endian))
			g_endian = 'D';

		template = g_ulong_template[g_endian] + 'data';
		arr = unpack(template, bin_data);
		return arr['data'];
	},

	getNetUShort: function(bin_data){
		arr = unpack('ndata', bin_data);
		return arr['data'];
	},

	/* unsigned short, for 16 bit integer */
	putUShort: function(short_data){
		 g_ushort_template;
		 g_endian;
		if((g_endian))
			g_endian = 'D';
		template = g_ushort_template[g_endian];
		this.m_data +=  pack(template, short_data);
		this.m_index += 2;
	},

	getUShort: function(bin_data){
		 g_ushort_template;
		 g_endian;
		if((g_endian))
			g_endian = 'D';

		template = g_ushort_template[g_endian] + 'data';
		arr = unpack(template, bin_data);
		return arr['data'];
	},
	
    bufRead: function(len, err){
	if (typeof err === 'undefined') {
	    err = 'false';
		buf = '';
		inBytes = 0;
		{
			buf = '';
			buf = socket_read(this.m_socket.getSocket(), len);
			ibuf +=  buf;
		}while((strlen(buf) < len) && (err != 1)); 

		//assert($inBytes == $len);
		return ibuf;
		document.write( 'Success');
	}},		

	getPreHDR: function(){
		
		this.init();
		ibuf = this.bufRead(GNP_PREHDR_LEN);
		this.m_data = this.getNetUShort(ibuf);
		return true;
	},
		

	putHDR: function(message_type_code){
		assert(message_type_code);
		this.init();
		this.putUByte(message_type_code);	
	},

	getHDR: function()
	{
		this.init();
		ibuf = this.bufRead(1);
		this.m_data = this.getUByte(ibuf);
		return true;
	},


	getDATA: function()	
	{
			

		this.init();
		ibuf = this.bufRead(VARIABLE_STRING_OVERHEAD);
		strLength = this.getUShort(ibuf);		

		ibuf = this.bufRead(strLength);
		this.m_data = this.getUByte(ibuf);
		return true;
	},
	
	putSUBSC: function(subsc)					{
		//reg_no is provided in the reply to the INITREG message at start-up			
		code = subsc.m_gds_key.encode(this.m_last_request);
		regno = pack('C', subsc.m_reg_no);
		code = regno + code;
		this.putVLString(code);	
	},

	getSUBSC: function()
	{
		this.init();

	var	ibuf = this.bufRead(VARIABLE_STRING_OVERHEAD);
		//see how many more bytes need to be read
		strLength = this.getUShort(ibuf);
		
		if(strLength == 1)
		{
			ibuf = this.bufRead(1);
			this.m_data =  this.getString(ibuf, 1, 0);
			return true;
		}

		ibuf = this.bufRead(1);
	var	tmp_regno = this.getUByte(ibuf);

		ibuf = this.bufRead(strLength - 1);
		//don't trim the string, 1 indicates a binary string
	var	tmp_gds = new gds(this.getString(ibuf, strLength - 1, 0), 1);			

		//take advantage of PHP's loose typing
		this.m_data = new SUBSC(tmp_regno, tmp_gds);
		return true;
	},

	putVALUE: function(value){
		this.putVLString(value);	
	},

	getVALUE: function()
	{
		
		this.init();

		ibuf = this.bufRead(VARIABLE_STRING_OVERHEAD);
		strLength = this.getUShort(ibuf);
		
		ibuf = this.bufRead(strLength);
		this.m_data = this.getString(ibuf, strLength, 0);
		return true;
	},

	getERROR: function()
	{
		//VINAYA: currently implemented as variable length string following the error HDR
		this.init();
		ibuf = this.bufRead(VARIABLE_STRING_OVERHEAD);
		strLength = this.getUShort(ibuf);
		ibuf = this.bufRead(strLength, 1);
		this.m_data = this.getString(ibuf, strLength, 0);
		return true;
	},
		
	putLAFLAG: function(flag){
		assert((flag == CM_LOCKS) || (flag == CM_LOCKS_INCR) || (flag == CM_ZALLOCATES));
		this.putUByte(flag);
	},

	getLAFLAG: function()
	{
		this.init();

		ibuf = this.bufRead(1);
		this.m_data = this.getUByte(ibuf);	
		return true;
	},

	putPROTOCOL: function()
	{
		if(defined('S_PROTOCOL'))
		{
			
			//$this->m_protocol is translated into a string of 6 * 3 + 1 bytes
	var		proto_string = '';
			proto_string +=  this.m_protocol.m_cpu_type;
			proto_string +=  this.m_protocol.m_os;
			proto_string +=  this.m_protocol.m_implementation;
			proto_string +=  this.m_protocol.m_version;
			proto_string +=  this.m_protocol.m_proto_type;
			proto_string +=  this.m_protocol.m_proto_level;
			proto_string +=  this.m_protocol.m_endian;

			if(this.m_protocol.m_proto_filler == 'SPACE')
				this.putAString(proto_string, this.m_protocol.m_proto_filler_length + JUNK);
		}
		else
		{
			this.putAString(S_PROTOCOL, S_PROTOCOL_PAD_LENGTH + JUNK);
		}
		
	},

	getPROTOCOL: function()
	{
		this.init();
	
		ibuf = this.bufRead(PROTO_SIZE);
		this.m_data = new PROTOCOL(this.getString(ibuf, PROTO_SIZE, 0));
		return true;
	},
		
	putREGNAME: function(region_name){
		//$region_name is a fully expanded file name which specifies a segment
		this.putVLString(region_name);
	},

	getREGNAME: function()
	{
		this.init();
		ibuf = this.bufRead(VARIABLE_STRING_OVERHEAD);
		strLength = this.getUShort(ibuf);

		ibuf = this.bufRead(strLength);
		this.m_data = this.getString(ibuf, strLength, 0);
		return true;
	},
		
	getREGINFO: function()
	{
		this.init();
		ibuf = this.bufRead(1);
		tmp_regno = this.getUByte(ibuf);
		
		ibuf = this.bufRead(1);
		tmp_nullsub = this.getUByte(ibuf);
		
		ibuf = this.bufRead(2);
		tmp_max_rec_len = this.getUShort(ibuf);
		
		ibuf = this.bufRead(2);
		tmp_max_sub_len = this.getUShort(ibuf);

		this.m_data = new REGION_INFO(tmp_regno, tmp_nullsub, tmp_max_rec_len, tmp_max_sub_len);
		return true;
	},

	putLKSUBSC: function(subsc){
		string = '';
		translev = 1;
		sub_depth = util.count(subsc.m_gds_key.m_subscripts);
		global_len = strlen(subsc.m_gds_key.m_global_name);		//util.counting the ^
		car = 0x5e;
		string +=  pack('C5', subsc.m_reg_no, translev, (sub_depth + 1), (global_len + 1), car);

		template = 'a' + global_len;
		global_name_packed = pack(template, subsc.m_gds_key.m_global_name);
		subs = '';
		i = 0;
		while(i < sub_depth)
		{
			subs +=  pack('C', 0x01);
			template = 'a' + strlen(subsc.m_gds_key.m_subscripts[i]);
			subs +=  pack(template, subsc.m_gds_key.m_subscripts[i]);
			i++;
		}
		
		string +=  global_name_packed + subs;
		this.putVLString(string);
	},

	putSUBLIST: function(node_count, subscs){
		realCnt = util.count(subscs);
		this.putUByte(realCnt==node_countnode_countrealCnt);
		
		this.init();
		
	var ibuf = this.bufRead(1);
	var	nodeCnt = this.getUByte(ibuf);
		
	var	subList = {};
	for	(var i = 0; i < nodeCnt; i++)
		{
	var	inBytes = this.getSUBSC();
			if(inBytes)
			{
				err_msg = 'getSUBLIST() failed at ' + i + 'th node';
				console.log(err_msg);
				return 0;
			}

		inBytes1 += inBytes;
		subList.push = this.m_data;
		}

		this.init();
		this.m_data = subList;
		return true;
	},

	//proc_num is returned from INITPROC function
	putPROCNUM: function(proc_num){
		this.putUShort(proc_num);
	},

	getPROCNUM: function()
	{
		

		this.init();
		
		ibuf = this.bufRead(2);
		this.m_data = this.getUShort(ibuf);
        return true;
	},

	putREGNUM: function(reg_num){
		this.putUByte(reg_num);
	},
	
	getREGNUM: function()
	{
		
        this.init();
        ibuf = this.bufRead(1);
		this.m_data = this.getUByte(ibuf);
        return true;
	},
	putTRANSNUM: function(trans_num){
		if(this.m_last_request == m.CMMS_S_INTERRUPT)
			this.init();
		this.putUByte(trans_num);
	},

	getTRANSNUM: function()
	{
		this.init();
		
		ibuf = this.bufRead(1);
		this.m_data = this.getUByte(ibuf);
		return true;
	}
	
});

