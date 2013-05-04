<?php

/****************************************************************
 *								*
 *	Copyright 2001 Sanchez Computer Associates, Inc.	*
 *								*
 *	This source code contains the intellectual property	*
 *	of its copyright holder(s), and is made available	*
 *	under a license.  If you do not know the terms of	*
 *	the license, please stop and do not read further.	*
 *								*
 ****************************************************************/

##################################################################
#	Please change followins appropriate for your environment #
##################################################################
#define ("MSG", 		     FALSE);
#define ("DEBUG", 		     FALSE);			//debug flag, if defined, dump program traces
define ("SILENT", 		     FALSE);			//if true, no error message will be reported to the browser
define ("LOG_FATAL_ONLY",	     FALSE);			//if true, only fatal errors will be reported and logged
define ("DEFAULT_SERVER",      "127.0.0.1");	//GTCM server is running on
define ("DEFAULT_PORT",		     30000);			//port number that GTCM server is listening to
define ("LOGFILE", 	       "error.log");		//errors are logged in this file, make sure to chmod a+w
define ("DB1",		"/opt/lsb-gtm/gtm_V60001_x8664/mumps.dat");
define ("DB2",		"/opt/lsb-gtm/gtm_V60001_x8664/mumps.dat");
define ("HOST1",	       "127.0.0.1");
define ("HOST2",               "127.0.0.1");
define ("PORTNO",	             30000);

?>
