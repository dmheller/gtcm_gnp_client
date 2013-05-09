
<?php

class GNP_BUFFER
{
	#########################
	#	DATA MEMBERS	#
	#########################

	private $m_data;		//string holding the translated data to be read or raw data to be written
	private $m_index;		//byte length index
	private $m_socket;	//the socket to read in from and write out to
	private $m_last_request;	//remember the last request so as to verify the response	
	private $m_protocol;	//the PROTOCOL being used in INITPROC message
	

	#########################################
	#	PUBLIC FUNCTIONS		#	
	#########################################

public	function __construct($socket)
	{
		$this->init();
		$this->m_socket = $socket;
		
		$this->m_protocol = new PROTOCOL();


		unset($this->m_last_request);

		
	}

	//returns m_exception object on failure
public	function doop($op_code)	
	{	
		if($op_code != CMMS_S_INTERRUPT)
			$this->putHDR($op_code);
		$this->m_last_request = $op_code;

		if(($op_code >= CMMS_L_LKCANALL) && ($op_code <= CMMS_L_LKSUSPEND))
		{
			$laflag = func_get_arg(1);
			$this->putLAFLAG($laflag);
		}

		if(($op_code == CMMS_L_LKCANALL) || 
		    ($op_code == CMMS_L_LKCANCEL))
		{
			$transnum = func_get_arg(2);
			$this->putTRANSNUM($transnum);
		}
		elseif($op_code == CMMS_S_INTERRUPT)
		{
			$transnum = func_get_arg(1);
			$this->putTRANSNUM($transnum);
		}
		elseif(($op_code == CMMS_L_LKREQUEST) ||
			($op_code == CMMS_L_LKREQNODE) ||
			($op_code == CMMS_L_LKREQIMMED) ||
			($op_code == CMMS_L_LKDELETE))
		{
			$transnum = func_get_arg(2);
			$sublist = func_get_arg(3);
			assert(is_array($sublist));
			$num_nodes = count($sublist);
			$this->putTRANSNUM($transnum);
			$this->putSUBLIST($num_nodes, $sublist);
		} 		
		elseif($op_code == CMMS_S_INITPROC)
		{
			$this->putPROTOCOL();
		}
		elseif($op_code == CMMS_S_INITREG)
		{
			$reg_name = func_get_arg(1);
			$this->putREGNAME($reg_name);
		}
		elseif($op_code == CMMS_S_TERMINATE)
		{
		}
		elseif(($op_code == CMMS_Q_KILL) ||
			($op_code == CMMS_Q_PUT) ||
			($op_code == CMMS_Q_GET) ||
			($op_code == CMMS_Q_ORDER) ||
			($op_code == CMMS_Q_PREV) ||
			($op_code == CMMS_Q_DATA) ||
			($op_code == CMMS_Q_QUERY) ||
			($op_code == CMMS_Q_ZWITHDRAW))
		{
			$subsc = func_get_arg(1);
			$this->putSUBSC($subsc);
		}
		elseif (($op_code == CMMS_L_LKACQUIRE) || ($op_code == CMMS_L_LKRESUME) 
			|| ($op_code == CMMS_L_LKSUSPEND))
		{}
		else
			assert(FALSE);
		
		if($op_code == CMMS_Q_PUT)
		{
			$val = func_get_arg(2);
			$this->putVALUE($val);
		}

		if(($op_code == CMMS_S_TERMINATE) || ($op_code == CMMS_L_LKREQNODE))	
			$ok = $this->send();
		elseif($op_code == CMMS_L_LKACQUIRE)
		$ok = (($this->send()) && ($this->lockReceive()));
		elseif($op_code == CMMS_S_INTERRUPT)
		$ok = (($this->urgentSend()) && ($this->receive()));
		else		
		$ok = (($this->send()) && ($this->receive()));
			

		if(($op_code == CMMS_S_TERMINATE)|| ($op_code == CMMS_L_LKREQNODE))
		{
			if(!$ok)
			{
				if($op_code == CMMS_S_TERMINATE)
					$func = "TERMINATE()";
				else $func = "LKREQNODE()";
				$err_msg = $func . "failed. Message not sent.";
				$err = throwit(__FILE__, __LINE__, "doop", 1, $err_msg);
				return $err;
			}
			else
				return TRUE;
		}
		elseif(($op_code == CMMS_Q_KILL) || 
			($op_code == CMMS_Q_PUT) || 
			($op_code == CMMS_Q_ZWITHDRAW))
		{
			if(!$ok)
			{
				$err_msg = "Operation (" . $op_code . ") failed.";
				$err = throwit(__FILE__, __LINE__, "doop", 1, $err_msg);
				return $err;
			}
			else
				return TRUE;
		}
		elseif($op_code == CMMS_L_LKACQUIRE)
		{
			if(!$ok)
			{
				$err_msg = "lock(" . $op_code . ") failed.<br.\n";
				$err = throwit(__FILE__, __LINE__, "doop", 1, $err_msg);
				return $err;
			}
			elseif(is_null($this->m_data))
			{
				return FALSE;
			}
			else
			{
				return $this->m_data;			//contains CMMS_M_'s
			}
		}
		else
		{
			if(!$ok)
			{
				$err_msg = "Operation (" . $op_code . ") failed.";
				$err = throwit(__FILE__, __LINE__, "doop", 1, $err_msg);
				return $err;	
			}
			else
			{
				return $this->m_data;
			}
		}
	}


	#################################################################
	#	PRIVATE FUNCTIONS: GENERAL USE, HELPERS			#
	#################################################################

	/* send() returns TRUE if successful
	 * returns FALSE otherwise
	 */
private  function send()
	{
		

		//NEW: add 2 bytes to each message, indicating the length of the message,in BIG-ENDIAN mode
		$preHDR = $this->getLength();
		$preHDR_packed = pack("n", $preHDR);
		$this->m_data = $preHDR_packed . $this->m_data;
		$gnp_msg_len = $preHDR + GNP_PREHDR_LEN;
		$fp = $this->m_socket->getSocket();
		$neg_errno = socket_write($fp, $this->m_data, $gnp_msg_len);
		if($neg_errno === false)
		{
			$err_msg = socket_strerror(-$neg_errno);	
			catchit(throwit(__FILE__, __LINE__, "send", 0, $err_msg));
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}

	/* urgentSend(), used to send LKCANCEL messages, returns TRUE if successful 
	 * returns FALSE otherwise
	 */
private  function urgentSend()
	{
		
		$fp = $this->m_socket->getSocket();
		$neg_errno = socket_send($fp, $this->m_data, 1, MSG_OOB);
	
		if($neg_errno === FALSE)
		{
			$err_msg = strerror(-$neg_errno);	
			catchit(throwit(__FILE__, __LINE__, "urgentSend", 0, $err_msg));	
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}

	/* receive() returns TRUE if successful
	 * returns FALSE otherwise
	 */
private  function receive()
	{	
		global $g_endian_convert;
		global $g_endian;
		global $g_self_big_endian;


		//first, skip the 2 bytes in big endian order
		$skipped = $this->getPreHDR();
		assert($skipped == GNP_PREHDR_LEN);

		// read the one byte HDR
		$inBytes = $this->getHDR();

		if(!$inBytes)
		{
			$err_msg = "No header received, check network connection.";
			catchit(throwit(__FILE__, __LINE__, "receive", 0, $err_msg));	
			return FALSE;
		}
		else
		{	
			//force $header to be integer
			$hdr = 0;
			$hdr += $this->m_data;
			if(!$this->matchRequest($hdr))
			{
			    $err_msg = "request/response type mismatch. Request = " .
				$this->m_last_request . " Response = " . $hdr;
			    catchit(throwit(__FILE__, __LINE__, "receive", 0, $err_msg)); 
			    return FALSE;
			 }
			
			switch($hdr)
			{
				case CMMS_M_LKABORT:
				case CMMS_M_LKBLOCKED:
				case CMMS_M_LKGRANTED:
				case CMMS_M_LKDELETED:
				case CMMS_M_LKSUSPENDED: break;
				case CMMS_T_INITPROC:   
							if(!$this->getPROTOCOL()) 
								return FALSE;
							
							if(!$this->m_protocol->matchEndian($this->m_data))
								$g_endian_convert = TRUE;
							else
								$g_endian_convert = FALSE;

							if(!$g_endian_convert)
								$g_endian = ($g_self_big_endian? 'B' : 'L');
							else
								$g_endian = ($g_self_big_endian? 'L' : 'B');

							if(!$this->m_protocol->match($this->m_data))
							{
								$err_msg = "PROTOCOL mismatch.";
								catchit(throwit(__FILE__, __LINE__, "receive", 0, $err_msg));
								return FALSE;
							}							
							if(!$this->getPROCNUM()) return FALSE;
							break;

 				case CMMS_T_REGNUM:	if(!$this->getREGINFO()) return FALSE;
							break;

				case CMMS_R_GET:	if(!$this->getVALUE()) return FALSE;
							break;

				case CMMS_R_UNDEF:	if($this->m_last_request == CMMS_Q_GET)
							{
								$this->m_data = null;
								break;
							}
							else
							{
								$err_msg = "GTCM_GVN value reference undefined.";
								catchit(throwit(__FILE__, __LINE__, "receive", 0, $err_msg));
								return FALSE;
							}

				case CMMS_R_ORDER:	if(!$this->getSUBSC()) return FALSE;
							break;

				case CMMS_R_PREV:	if(!$this->getSUBSC()) return FALSE;
							break;

				case CMMS_R_DATA:	if(!$this->getDATA()) return FALSE;
							break;

				case CMMS_R_KILL:	
				case CMMS_R_PUT:	break;

				case CMMS_R_QUERY:	if(!$this->getSUBSC()) return FALSE;
							break;

				case CMMS_R_ZWITHDRAW:	break;
					
				case CMMS_E_ERROR:	if(!$this->getERROR()) return FALSE;
							$err_msg = "From server: " . $this->m_data;
							catchit(throwit(__FILE__, __LINE__, "receive", 0, $err_msg, TRUE));
							return FALSE;

				case CMMS_E_TERMINATE:	if(!$this->TERMINATE()) return FALSE;
							break;

				default:
					return TRUE;
			}
		}

		return TRUE;
	}
	
	/* lockReceive() returns TRUE if CMMS_M was received. It fills m_data with CMMS_M_ message
	 *		 returns TRUE if CMMS_M was not received and no error occurred. It sets m_data to null
	 * 		returns  FALSE otherwise
	 * note: 	nonblocking mode
	 */
private  function lockReceive()
	{
		sleep(LOCK_INTERVAL);		

		if(SOCKET_ALWAYS_NONBLOCK == 0)
			$this->m_socket->setNonBlock();		

		//first, skip the 2 bytes in big endian order and get the HDR
		$this->init();
		$fp = $this->m_socket->getSocket();
		$buf = '';
		$inBytes = socket_recv($fp, $buf, (GNP_PREHDR_LEN + 1), MSG_NOWAIT);
		
		if($inBytes <= 0)
		{
			$this->m_data = null;
			if(SOCKET_ALWAYS_NONBLOCK == 0)
				$this->m_socket->setBlock();
			return TRUE;
		}
		
		//data is coming, so pick them up all at once
		if(($inBytes > 0) && ($inBytes < (GNP_PREHDR_LEN + 1)))	
		{
						$ibuf = $buf;
			$alreadyIn = strlen($buf);
			if(SOCKET_ALWAYS_NONBLOCK == 0)
			{
				$this->m_socket->setBlock();
				do
				{
					$buf = "";
					$buf = socket_read($this->m_socket->getSocket(), (GNP_PREHDR_LEN + 1 - $alreadyIn));
					$ibuf .= $buf;
                                        $alreadyIn += $inBytes;
                                }while($alreadyIn < (GNP_PREHDR_LEN + 1));
                                $inBytes = $alreadyIn;
                                $buf = $ibuf;
				$this->m_socket->setNonBlock();
			}
			else
			{
				do
				{
					$buf = "";
					$buf = socket_read($this->m_socket->getSocket(), (GNP_PREHDR_LEN + 1 - $alreadyIn));
                  			$ibuf .= $buf;
					$alreadyIn += $inBytes;
				}while($alreadyIn < (GNP_PREHDR_LEN + 1));
				$inBytes = $alreadyIn;
				$buf = $ibuf;
			}
		}	
	
		assert($inBytes == (GNP_PREHDR_LEN + 1));
		$hdr_packed = substr($buf, 2, 1);
		$this->m_data = $this->getUByte($hdr_packed);

		//force $header to be integer
		$hdr = 0;
		$hdr += $this->m_data;

		if(!$this->matchRequest($hdr))
		{
			$err_msg = "request/response type mismatch. Request = " .
				$this->m_last_request . " Response = " . $hdr;
			catchit(throwit(__FILE__, __LINE__, "lockReive", 0, $err_msg ));
			if(SOCKET_ALWAYS_NONBLOCK == 0)
				$this->m_socket->setBlock();
			return FALSE;
		}
		switch($hdr)
		{
			case CMMS_M_LKABORT:
			case CMMS_M_LKBLOCKED:
			case CMMS_M_LKGRANTED:
			case CMMS_M_LKDELETED:
			case CMMS_M_LKSUSPENDED: break;
							
			case CMMS_E_ERROR:	if(SOCKET_ALWAYS_NONBLOCK == 0)
							$this->m_socket->setBlock();
						if(!$this->getERROR())
							return FALSE;
						$err_msg = "From server: " . $this->m_data;
						catchit(throwit(__FILE__, __LINE__, "lockReceive", 0, $err_msg, TRUE));
						return FALSE;

			case CMMS_E_TERMINATE:  if(SOCKET_ALWAYS_NONBLOCK == 0)
							$this->m_socket->setBlock();
						if(!$this->TERMINATE()) 
							return FALSE;
								
						if(SOCKET_ALWAYS_NONBLOCK == 0)
							$this->m_socket->setNonBlock();
						break;
			default:
				if(SOCKET_ALWAYS_NONBLOCK == 0)
					$this->m_socket->setBlock();
				assert (FALSE);
		}

		if(SOCKET_ALWAYS_NONBLOCK == 0)
			$this->m_socket->setBlock();
		return TRUE;
	}

private  function init()
	{
		$this->m_index = 0;
		$this->m_data = '';
	}

	/* can only be used with messages to be sent */
private  function getLength()
	{
		return $this->m_index;
	}

	/* used with both messsages to be sent and received */
private  function getRealLength()
	{
		return strlen($this->m_data);
	}

private  function matchRequest($response)
	{
		//CMMS_R type
		if(($this->m_last_request >= CMMS_Q_DATA) && ($this->m_last_request <= CMMS_Q_ZWITHDRAW))
		{
			if(($response == $this->m_last_request + 8) || ($response == CMMS_R_UNDEF)
				|| ($response == CMMS_E_ERROR) || ($response == CMMS_E_TERMINATE))
				return TRUE;
			else return FALSE;
		}

		//CMMS_S type
		
		if(($this->m_last_request >= CMMS_S_INITPROC) && ($this->m_last_request <= CMMS_S_INITREG))
		{
			if(($response == $this->m_last_request + 4)
				|| ($response == CMMS_E_ERROR) || ($response == CMMS_E_TERMINATE))
				return TRUE;
			else return FALSE;
		}

		//locks
	
		if($this->m_last_request == CMMS_S_INTERRUPT)	//urgentCancel
		{
			if(($response == CMMS_M_LKDELETED) || ($response == CMMS_E_ERROR)
				|| ($response == CMMS_E_TERMINATE))
				return TRUE;
			else return FALSE;
		}
	
		if(($this->m_last_request >= CMMS_L_LKCANALL) && ($this->m_last_request <= CMMS_L_LKDELETE ))
		{
			if(($response == CMMS_M_LKDELETED) || ($response == CMMS_E_ERROR)
				|| ($response == CMMS_E_TERMINATE))
				return TRUE;
			else return FALSE;
		}

		if($this->m_last_request == CMMS_L_LKREQIMMED)
		{
			if(($response == CMMS_M_LKGRANTED) || ($response == CMMS_M_LKABORT)
				|| ($response == CMMS_E_ERROR) || ($response == CMMS_E_TERMINATE))
				return TRUE;
			else return FALSE;
		}

		if(($this->m_last_request == CMMS_L_LKREQUEST) || ($this->m_last_request == CMMS_L_LKRESUME))
		{
			if(($response == CMMS_M_LKGRANTED) || ($response == CMMS_M_LKBLOCKED)
				|| ($response == CMMS_E_ERROR) || ($response == CMMS_E_TERMINATE))
				return TRUE;
			else return FALSE;
		}

		if($this->m_last_request == CMMS_L_LKACQUIRE)
		{
			if(($response == CMMS_M_LKGRANTED) 
				|| ($response == CMMS_E_ERROR) || ($response == CMMS_E_TERMINATE))
				return TRUE;
			else return FALSE;
		}

		if($this->m_last_request == CMMS_L_LKSUSPEND)
		{
			if(($response == CMMS_M_LKSUSPENDED) 
				|| ($response == CMMS_E_ERROR) || ($response == CMMS_E_TERMINATE))
				return TRUE;
			else return FALSE;
		}

		return FALSE;		
	}

	#################################################################
	#	PRIVATE FUNCTIONS: FUNCTIONS RELATED TO DATA TYPES	#
	#################################################################
	#		WITH ENDIAN SUPPORT				#
	#################################################################

	/* unsigned char, for one byte integer */
private  function putUByte($int_data)
	{	
		assert(is_integer($int_data));
		$this->m_data .= pack("C", $int_data);
		$this->m_index++;
	}

static  function getUByte($bin_data)	
	{	
		$arr = unpack("C", $bin_data);
		return $arr[1];
	}
		

	/* unsigned char, for character */
private  function putUChar($char_data)
	{
		assert((ord($char_data) >=0) && (ord($char_data < 255)));
		$this->m_data .= pack("C", ord($char_data));
		$this->m_index++;
	}

static  function getUChar($bin_data)
	{
		$arr = unpack("Cdata", $bin_data);
		return chr($arr);
	}
		
	/* for string, optionally padded with \0 */
private  function putString($str_data, $pad_len=0)
	{
		$len = strlen($str_data);
		$pack_len = $len + $pad_len;

		$template = "a" . $pack_len;
		$list = $str_data;

		$this->m_data .= pack($template, $list);
		$this->m_index += $pack_len;
	}

static  function getString($bin_data, $length, $trim)
	{
	
		$realLen = strlen($bin_data);
		if($realLen != $length)
			$length = $realLen;
		
		$template = "a" . $length . "string";
		$arr = unpack($template, $bin_data);
		
		
		if($trim == 0)
			return $arr['string'];
		if($trim == -1)
			return ltrim($arr['string']);
		if($trim == 1)
			return rtrim($arr['string']);
		if($trim == 2)
			return trim($arr['string']);
	}

	/* for string, optionally padded with spaces */
private  function putAString($str_data, $pad_len=0)
	{
		$len = strlen($str_data);
		$pack_len = $len + $pad_len;

		$template = "A" . $pack_len;
		$list = $str_data;

		$this->m_data .= pack($template, $list);
		$this->m_index += $pack_len;
	}
	
	/* for variable length string */
private  function putVLString($vString)
	{
		$len = strlen($vString);
		
		//indicate the length of the string first
		if(VARIABLE_STRING_OVERHEAD == 4)
		{
			$this->putULong($len);
		}
		else
		{
			//legacy variable length from VMS is 16 bits
			$this->putUShort($len);			
		}
		
		//now append the string itself
		$this->putString($vString);
	}

	/* unsigned long, for 32 bit integer */
private  function putULong($long_data)
	{
		global $g_ulong_template;
		global $g_endian;
		if(empty($g_endian))
			$g_endian = 'D';
		
		$template = $g_ulong_template[$g_endian];
		$this->m_data .= pack($template, $long_data);
		// 32 bits = 4 bytes
		$this->m_index += 4;		
	}

static  function getULong($bin_data)
	{
		global $g_ulong_template;
		global $g_endian;
		if(empty($g_endian))
			$g_endian = 'D';

		$template = $g_ulong_template[$g_endian] . "data";
		$arr = unpack($template, $bin_data);
		return $arr['data'];
	}

static  function getNetUShort($bin_data)
	{
		$arr = unpack("ndata", $bin_data);
		return $arr['data'];
	}

	/* unsigned short, for 16 bit integer */
private  function putUShort($short_data)
	{
		global $g_ushort_template;
		global $g_endian;
		if(empty($g_endian))
			$g_endian = 'D';
		$template = $g_ushort_template[$g_endian];
		$this->m_data .= pack($template, $short_data);
		$this->m_index += 2;
	}

private  function getUShort($bin_data)
	{
		global $g_ushort_template;
		global $g_endian;
		if(empty($g_endian))
			$g_endian = 'D';

		$template = $g_ushort_template[$g_endian] . "data";
		$arr = unpack($template, $bin_data);
		return $arr['data'];
	}
	
	#########################################################################
	#	PRIVATE FUNCTIONS: FUNCTIONS RELATED TO GNP MESSAGE SECTIONS	#
	#########################################################################	
	#									#
	# Getter functions complement all setter functions, return number of 	#	
	# bytes read and fill m_data if succeed, or return 0 and fill m_info	#
	# with error message if failed.						#
	#########################################################################

public  function bufRead($len, $err = 0)
	{
		$ibuf = '';
		$buf = '';
		$inBytes = 0;
		$fp = $this->m_socket->getSocket();
		do
		{	
			$e = NULL;
			$w = NULL;
			$r = array($this->m_socket->getSocket());
			socket_select($r, $w, $e, WAIT); // To avoid warning messages about socket being temporarily unavailbale     
			$inBytes += socket_recv($fp, $buf, $len, MSG_DONTWAIT);
			$ibuf .= $buf;
		}while((SOCKET_ALWAYS_NONBLOCK == 1) && ($inBytes < $len) && ($err != 1)); 
		return $ibuf;
	}		

private  function getPreHDR()
	{
	
		$this->init();
		
		$ibuf = $this->bufRead(GNP_PREHDR_LEN);
		$this->m_data = $this->getNetUShort($ibuf);
		
		return TRUE;
	}
		

private  function putHDR($message_type_code)
	{
		assert(!empty($message_type_code));
		$this->init();
		$this->putUByte($message_type_code);	
	}

private  function getHDR()
	{	

		$this->init();
		$ibuf = "";
		$ibuf = $this->bufRead(1);
		$this->m_data = $this->getUByte($ibuf);
		return TRUE;
	}


private  function getDATA()	
	{
			
		$this->init();
		$ibuf = $this->bufRead(VARIABLE_STRING_OVERHEAD);
		$strLength = $this->getUShort($ibuf);	
		$ibuf = $this->bufRead($strLength);
		$this->m_data = $this->getUByte($ibuf);
		return TRUE;
	}
	
private  function putSUBSC($subsc)					
	{
		//reg_no is provided in the reply to the INITREG message at start-up			
		$code = $subsc->m_GDS_key->encode($this->m_last_request);
		$regno = pack("C", $subsc->m_reg_no);
		$code = $regno . $code;
		$this->putVLString($code);	
	}

private  function getSUBSC()
	{
		$this->init();

		$ibuf = $this->bufRead(VARIABLE_STRING_OVERHEAD);
		//see how many more bytes need to be read
		$strLength = $this->getUShort($ibuf);
		
		if($strLength == 1)
		{
			$ibuf = $this->bufRead(1);
			$this->m_data =  $this->getString($ibuf, 1, 0);
			return TRUE;
		}

		$ibuf = $this->bufRead(1);
		$tmp_regno = $this->getUByte($ibuf);

		$ibuf = $this->bufRead($strLength - 1);
		//don't trim the string, 1 indicates a binary string
		$tmp_GDS = new GDS($this->getString($ibuf, $strLength - 1, 0), 1);			

		//take advantage of PHP's loose typing
		$this->m_data = new SUBSC($tmp_regno, $tmp_GDS);
		return TRUE;
	}

private  function putVALUE($value)
	{
		$this->putVLString($value);	
	}

private  function getVALUE()
	{
		$this->init();

		$ibuf = $this->bufRead(VARIABLE_STRING_OVERHEAD);
		$strLength = $this->getUShort($ibuf);
		
		$ibuf = $this->bufRead($strLength);
		$this->m_data = $this->getString($ibuf, $strLength, 0);
		return TRUE;
	}

private  function getERROR()
	{
		//VINAYA: currently implemented as variable length string following the error HDR
		$this->init();
		$ibuf = $this->bufRead(VARIABLE_STRING_OVERHEAD);
		$strLength = $this->getUShort($ibuf);
			$ibuf = $this->bufRead($strLength, 1);
		$this->m_data = $this->getString($ibuf, $strLength, 0);
		return TRUE;
	}
		
private  function putLAFLAG($flag)
	{
		assert(($flag == CM_LOCKS) || ($flag == CM_LOCKS_INCR) || ($flag == CM_ZALLOCATES));
		$this->putUByte($flag);
	}

private  function getLAFLAG()
	{
		$this->init();

		$ibuf = $this->bufRead(1);
		$this->m_data = $this->getUByte($ibuf);	
		return TRUE;
	}

private  function putPROTOCOL()
	{
		if(!defined("S_PROTOCOL"))
		{
			//$this->m_protocol is translated into a string of 6 * 3 + 1 bytes
			$proto_string = "";
			$proto_string .= $this->m_protocol->m_cpu_type;
			$proto_string .= $this->m_protocol->m_os;
			$proto_string .= $this->m_protocol->m_implementation;
			$proto_string .= $this->m_protocol->m_version;
			$proto_string .= $this->m_protocol->m_proto_type;
			$proto_string .= $this->m_protocol->m_proto_level;
			$proto_string .= $this->m_protocol->m_endian;
			$this->m_protocol->m_proto_filler_length = S_PROTOCOL_PAD_LENGTH;
			if($this->m_protocol->m_proto_filler == "SPACE")
				$this->putAString($proto_string, $this->m_protocol->m_proto_filler_length + JUNK);
		}
		else
		{
			$this->putAString(S_PROTOCOL, S_PROTOCOL_PAD_LENGTH + JUNK);
		}
		

		
	}

private  function getPROTOCOL()
	{
		$this->init();
	
		$ibuf = $this->bufRead(PROTO_SIZE);
		$this->m_data = new PROTOCOL($this->getString($ibuf, PROTO_SIZE, 0));
		return TRUE;
	}
		
private  function putREGNAME($region_name)
	{
		//$region_name is a fully expanded file name which specifies a segment
		$this->putVLString($region_name);
	}

private  function getREGNAME()
	{
		$this->init();
		$ibuf = $this->bufRead(VARIABLE_STRING_OVERHEAD);
		$strLength = $this->getUShort($ibuf);

		$ibuf = $this->bufRead($strLength);
		$this->m_data = $this->getString($ibuf, $strLength, 0);
		return TRUE;
	}
		
private  function getREGINFO()
	{
		$this->init();
		$ibuf = $this->bufRead(1);
		$tmp_regno = $this->getUByte($ibuf);
		
		$ibuf = $this->bufRead(1);
		$tmp_nullsub = $this->getUByte($ibuf);
		
		$ibuf = $this->bufRead(2);
		$tmp_max_rec_len = $this->getUShort($ibuf);
		
		$ibuf = $this->bufRead(2);
		$tmp_max_sub_len = $this->getUShort($ibuf);

		$this->m_data = new REGION_INFO($tmp_regno, $tmp_nullsub, $tmp_max_rec_len, $tmp_max_sub_len);
		return TRUE;
	}

private  function putLKSUBSC($subsc)
	{
		$string = "";
		$translev = 1;
		$sub_depth = count($subsc->m_GDS_key->m_subscripts);
		$global_len = strlen($subsc->m_GDS_key->m_global_name);		//counting the ^
		$car = 0x5e;
		$string .= pack("C5", $subsc->m_reg_no, $translev, ($sub_depth + 1), ($global_len + 1), $car);

		$template = "a" . $global_len;
		$global_name_packed = pack($template, $subsc->m_GDS_key->m_global_name);
		$subs = "";
		$i = 0;
		while($i < $sub_depth)
		{
			$subs .= pack("C", 0x01);
			$template = "a" . strlen($subsc->m_GDS_key->m_subscripts[$i]);
			$subs .= pack($template, $subsc->m_GDS_key->m_subscripts[$i]);
			$i++;
		}
		
		$string .= $global_name_packed . $subs;
		$this->putVLString($string);
	}

private  function putSUBLIST($node_count, $subscs)
	{
		$realCnt = count($subscs);
		$this->putUByte($realCnt==$node_count?$node_count:$realCnt);
		foreach ($subscs as $node)
			$this->putLKSUBSC($node);
	}

private  function getSUBLIST()
	{
		$this->init();
		
		$ibuf = $this->bufRead(1);
		$nodeCnt = $this->getUByte($ibuf);
		
		$subList = array();
		for($i = 0; $i < $nodeCnt; $i++)
		{
			$inBytes = $this->getSUBSC();
			if(!$inBytes)
			{
				$err_msg = "getSUBLIST() failed at " . $i . "th node";
				catchit(throwit(__FILE__, __LINE__, "getSUBLIST", 0, $err_msg));
				return 0;
			}

			$inBytes += $inBytes;
			$subList[] = $this->m_data;
		}

		$this->init();
		$this->m_data = $subList;
		return TRUE;
	}

	//proc_num is returned from INITPROC function
private  function putPROCNUM($proc_num)
	{
		$this->putUShort($proc_num);
	}

private  function getPROCNUM()
	{
	
		$this->init();
		
		$ibuf = $this->bufRead(2);
		$this->m_data = $this->getUShort($ibuf);
		return TRUE;
	}

private  function putREGNUM($reg_num)
	{
		$this->putUByte($reg_num);
	}
	
private  function getREGNUM()
	{

		$this->init();

		$ibuf = $this->bufRead(1);
		$this->m_data = $this->getUByte($ibuf);
		return TRUE;
	}

private  function putTRANSNUM($trans_num)
	{
		if($this->m_last_request == CMMS_S_INTERRUPT)
			$this->init();
		$this->putUByte($trans_num);
	}

private  function getTRANSNUM()
	{
		$this->init();
		
		$ibuf = $this->bufRead(1);
		$this->m_data = $this->getUByte($ibuf);
		return TRUE;
	}

}

?>
		
