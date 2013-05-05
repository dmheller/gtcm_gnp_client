
<?php

#################################################################################
#			USER DEFINED CONSTANTS					#
#################################################################################	

define ("CLIENT_WAIT_TIMEOUT", 	30);
define ("NO_IMPLEMENT",  "Not implemented");
define ("OPERATION_FAILURE", 	"Non-fatal error occurred");
define ("LOCK_INTERVAL",	1);
define ("SOCKET_ALWAYS_NONBLOCK",	0);
define ("WAIT", 1000); //how long to wait in usecs for a socket to be available before
		      //a error message is triggered if php is set to emit warnings
		     //has no effect on performance otherwise. Increase to reduce or eliminate warnings 

?>

<?php

#################################################################################
# 		PHP CLIENT SIDE ERROR MESSAGE					#	
#################################################################################

define ("PHP_SOCKET_FAILURE", 		"Cannot establish TCP socket with remote server");
define ("PHP_CONNECT_FAILURE", 		"Cannot connect socket to remote server");
define ("PHP_TRANSACTION_TIMEOUT",	"Cannot complete the transaction in specified timeout");

?>

<?php

#################################################################################
# 	CMMS_X_YYYY: 	GNP Application Level Protocol Messsage Type Code	#
#################################################################################
#	CLASS(X)	APPLICATION->SERVER	SERVER->APPLICATION		#
#	SYSTEM		X			Y				#
#	SESSION		S			T				#
#	DATABASE		Q			R				#
#	LOCK		L			M				#
#	UTILITY		U			V				#
#	ERROR		D			E				#
#										#
# Version:	V 1.0								#
#################################################################################

#################################################################################
# 		Following are copied from cmmdef.h			 	#
#################################################################################

define ("GNP_PREHDR_LEN", 	2);
define ("PROTO_SIZE",		 33);
define ("CM_PROTOCOL_LENGTH",	 19);
define ("CM_FILLER_SIZE",	 14);
#define ("S_PROTOCOL",		"X86LNXGTM060GCM200               ");
#define ("S_PROTOCOL_FILLER",	"               ");
define ("S_PROTOCOL_FILLER",	"SPACE");
define ("S_PROTOCOL_PAD_LENGTH", 14);
define ("PROTO_IMPLEMENTATION",	 "GTM");
define ("PROTO_VERSION",	 "054"); // set to 054 for gtm version 5.4xxxx 055 for version 5.5xxxx
define ("PROTO_LEVEL", 		 "200");	// DO NOT CHANGE MUST BE SET TO 200 Do not attemp to set to Level 210!!!

$os = php_uname('s');
$cpu = php_uname('m');

$release = "PHP_GTM V1.3 " . decodeCPU($cpu) . " " . decodeOS($os);

if(!defined("S_PROTOCOL"))
	define ("GTM_RELEASE_NAME",	 $release);	
else
	define ("GTM_RELEASE_NAME",	S_PROTOCOL);


define ("VARIABLE_STRING_OVERHEAD", 	2);	
define ("GNP_GLBNAME_MAX_LENGTH", 	16);
define ("JUNK",				72);	   
define ("MAX_NUM_SUBSC_LEN",		10);


define ("PHP_TIMEOUT",		   30);		 



define ("ZAREQUEST_SENT",	16);
define ("LREQUEST_SENT",		8);
define ("REQUEST_PENDING",	4);
define ("REMOTE_ZALLOCATES",	2);
define ("REMOTE_LOCKS",		1);
define ("REMOTE_CLR_MASK",	(ZAREQUEST_SENT + LREQUEST_SENT + REMOTE_ZALLOCATES + REMOTE_LOCKS));
define ("CM_BUFFER_OVERHEAD",	20);
define ("CM_BLKPASS",		40);

define ("CMM_PROTOCOL_TYPE",	"GCM");

define ("S_HDRSIZE",		2);
define ("S_PROTSIZE",		33);
define ("S_REGINSIZE",		6);
define ("S_LAFLAGSIZE",		1);
define ("S_SUBLISTSIZE",	1);
define ("CM_MINBUFSIZE",	512 + CM_BUFFER_OVERHEAD);

define ("CMLCK_REQUEUE",	0);
define ("CM_LOCKS",		0);
define ("CM_LOCKS_INCR",	0x40);
define ("CM_ZALLOCATES",	0x80);
define ("CM_NOLKCANCEL",	256);

define ("CM_WRITE",		1);
define ("CM_READ",		0);
define ("CM_NOOP",		2);


define ("CMMS_E_ERROR",		1);
define ("CMMS_L_LKCANALL",	2);
define ("CMMS_L_LKCANCEL",	3);
define ("CMMS_L_LKDELETE",	4);
define ("CMMS_L_LKREQIMMED",       5);
define ("CMMS_L_LKREQNODE",        6);			
define ("CMMS_L_LKREQUEST",        7);
define ("CMMS_L_LKRESUME",         8);
define ("CMMS_L_LKACQUIRE",        9);
define ("CMMS_L_LKSUSPEND",        10);			
define ("CMMS_M_LKABORT",          11);
define ("CMMS_M_LKBLOCKED",        12);			
define ("CMMS_M_LKGRANTED",        13);
define ("CMMS_M_LKDELETED",        14);
define ("CMMS_M_LKSUSPENDED",	   15);
define ("CMMS_Q_DATA",             16);
define ("CMMS_Q_GET",              17);
define ("CMMS_Q_KILL",             18);			
define ("CMMS_Q_ORDER",            19);
define ("CMMS_Q_PREV",             20);			
define ("CMMS_Q_PUT",              21);
define ("CMMS_Q_QUERY",            22);
define ("CMMS_Q_ZWITHDRAW",        23);
define ("CMMS_R_DATA",             24);
define ("CMMS_R_GET",              25);
define ("CMMS_R_KILL",             26);
define ("CMMS_R_ORDER",            27);			
define ("CMMS_R_PREV",             28);
define ("CMMS_R_PUT",		   29);			
define ("CMMS_R_QUERY",            30);
define ("CMMS_R_ZWITHDRAW",	   31);
define ("CMMS_R_UNDEF",            32);
define ("CMMS_S_INITPROC",         33);
define ("CMMS_S_INITREG",          34);
define ("CMMS_S_TERMINATE",        35);			
define ("CMMS_S_INTERRUPT",        36);		//USED AS LKURGCANCEL
define ("CMMS_T_INITPROC",         37);
define ("CMMS_T_REGNUM",           38);
define ("CMMS_X_INQPROC",         39);
define ("CMMS_X_INQPRRG",         40);
define ("CMMS_X_INQREG",          41);
define ("CMMS_Y_STATPROCREC",     42);
define ("CMMS_Y_STATPRRGREC",     43);		
define ("CMMS_Y_STATREGREC",      44);
define ("CMMS_U_LKEDELETE",       45);
define ("CMMS_U_LKESHOW", 	   46);
define ("CMMS_V_LKESHOW",         47);
define ("CMMS_E_TERMINATE",        48);
define ("CMMS_B_BUFRESIZE", 	   49);
define ("CMMS_B_BUFFLUSH",	   50);
define ("CMMS_C_BUFRESIZE",	   51);
define ("CMMS_C_BUFFLUSH",	   52);			


define ("CM_CPU_OFFSET",		0);
define ("CM_OS_OFFSET",			3);
define ("CM_IMPLEMENTATION_OFFSET",	6);
define ("CM_VERSION_OFFSET",		9);
define ("CM_TYPE_OFFSET",		12);
define ("CM_LEVEL_OFFSET",		15);
define ("CM_ENDIAN_OFFSET",		18);

###########################END OF CMMS_X_YYYY DEFINITIONS########################

?>

<?php

#################################
#	ENDIAN TEMPLATE		#
#################################

define ("USHORT_BIG", 		"n");		//unsigned short, 16 bits, big-endian order
define ("USHORT_LIT",		"v");		//unsigned short, 16 bits, little-endian order
define ("USHORT_MAC",		"S");		//unsigned short, 16 bits, machine byte order
define ("ULONG_BIG",		"N");		//unsigned long, 32 bits, big-endian order
define ("ULONG_LIT",		"V");		//unsigned long, 32 bits, little-endian order
define ("ULONG_MAC",		"L");		//unsigned long, 32 bits, machine byte order

$g_ushort_template = array('B' => USHORT_BIG, 'L' => USHORT_LIT, 'D' => USHORT_MAC);		//D for Default
$g_ulong_template = array('B' => ULONG_BIG, 'L' => ULONG_LIT, 'D' => ULONG_MAC);

?>

<?php

#################################
#	GLOBAL VARIABLES	#
#################################

global $g_endian;		//endian used in encoding and decoding UShort and ULong (B or L)
global $g_self_big_endian;	//boolean
global $g_max_key_size;
global $g_endian_convert;	//boolean

?>
<?php
function decodeCPU($name)
	{
		
		$cpu = $name;
		
		if(preg_match("/RS6000/i", $cpu))
			return "PPC";
		if(preg_match("/AXP/i", $cpu))
			return "AXP";
		if(preg_match("/HP-PA/i", $cpu))
			return "PAR";
		if(preg_match("/86/i", $cpu))
			return "X86";
		if(preg_match("/S390/i", $cpu))
			return "390";
		if(preg_match("/SPARC/i", $cpu))
			return "SPA";
		if(preg_match("/VAX/i", $cpu))
			return "VAX";
		if(preg_match("/ALPHA/i", $cpu))
			return "ALP";
		 
	}

	function decodeOS($name)
	{
		
		$os = $name;

		if(preg_match("/AIX/i", $os))
			return "AIX";
		if(preg_match("/OSF1/i", $os))
			return "OSF";
		if(preg_match("/HP-UX/i", $os))
			return "HPX";
		if(preg_match("/Linux/i", $os))
			return "LNX";
		if(preg_match("/OS390/i", $os))
			return "zOS";
		if(preg_match("/Solaris/i", $os))
			return "SOL";
		if(preg_match("/VMS/i", $os))
			return "VMS";
		
	}		
	
?>
