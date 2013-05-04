[PURPOSE]

This file is intended for PHP programmers who are familiar with GT.M.
After reading this file, one should be able to deploy and configure the included application
program, provided that 1) there is a web server + PHP interpreter 2) there is a GTCM/GNP 
server available over TCP socket on UNIX platform. The application was tested on Linux machines,
but it is expected to run on a Windows based web-server too. 

[CONTENTS]

The PHP-GTCM interface is comprised of several layered classes,  The classes are distributed into a number
of files, ending in ".inc The layers are as follows:

*Top to bottom

CLASS			IN FILE				PURPOSE
gnp_directory		gde.inc				maps a variable name to a GTCM_GNP object
							according to naming conventions
gnp_lock_manager	gnp_lock_manager_class.inc	coordinates locks among multiple servers

gtcm_gnp		gtcm_gnp_class.inc		main interface for individual GNP messages

gnp_buffer		gnp_buffer.inc			performs message formatting

tcp_socket		tcp_socket.inc			abstracts tcp socket behavior

subsc
gds
region_info
protocol		gnp_misc.inc			helper data structures that are defined by GNP

exception		exception.inc			reports and logs error messages

[CHANGES]


The application no longer depends on having register globals set to "On" making it
more production level (and more secure) ready. The library will now also works on
the command line giving the ability to create scripts that can run without user 
intervention. The application has been tested on php 5.3.10 . It should work
fine on 5.4 also. The program has also been checked with E_STRICT E_ALL activared
in php no notices or errors are reported. The demonstration script provided (index.php)
does however emit notices "variable undefined" it would not be a trivial task to 
eliminate those however. A test script is included (test_client.php) that demonstrates 
how to use this interface.

[INSTALLATION]

1. Make sure PHP5.0 or later version is available. This version tested on PHP5.3.10

2. Make sure web server is available and enabled for PHP. For testing Apache/2.2.x was used. 

3. Find the document root directory of your web browser (DocumentRoot
entry in /etc/httpd/conf/httpd.conf). Create sub-directory (say
gtcm_gnp_client)
under DocumentRoot. For example, if DocumentRoot on your system is
/var/www, create /var/www/gtcm_gnp_client.

4. Copy all the files into /var/www/html/gtcm_gnp_client

5. Verify that index.php is under /var/www/gtcm_gnp_client and *.inc under ./inc
subdirectory.

6. /var/www/gtcm_gnp_client/inc has some files for the PHP-GTCM client interface. 
Add appropriate entry in /etc/php5/apache2/php.ini so that these files can be located by
PHP. (php.ini could be some other place for your system.) For example:

include_path            = ".:/usr/share/php:/var/www/gtcm_gnp_client/inc"

if you plan on running scripts on the command line also you have to also modify /etc/php5/cli/php.ini.  

7. Register Globals can and should be set to Off in this release and MAGIC QUOTES has to be DISABLED 
to use the M demonstration application included in this package otherwise it will cause BIG problems!

8. Create an empty error.log file in /var/www/gtcm_gnp_client. Change the
permissions on this file to be world writable. For example:

touch /var/www/html/gtcm_gnp_client/error.log
chmod 766 /var/www/gtcm_gnp_client/error.log

9. To troubleshoot uncomment these defines: #define ("MSG", 		FALSE);
#define ("DEBUG", 		FALSE);	


[EXAMPLE USAGE]

Make any wanted modifications to the file server_start.sh it will use 
your current gtm enviroment settings and run server_start.sh to start 
the server. The server port will be 30000 on localhost as its currently 
set.


[CONFIGURATION]

Modify /inc/config.inc and header.hd to suit your needs. 
The items that are configurable are: debugging mode switch,
silent mode switch, error log level switch, error log file name switch. For further explanation,
refer to the comments in this config.inc. Please also make necessary changes in constants.inc:
the constant WAIT might need to be changed for very slow connections and PROTO_VERSION should be
set to the version of gtm you which to use ie: set to 054 055 or 060 as needed.


[ABOUT THE APPLICATION]

The purpose of the application (index.php) is to allow the browser to be used as a User Interface to issue
M commands. The contents of the database are shown at the same time. 

The ISSUE section takes user input, FEEDBACK section displays the M syntax for the command issued and 
the response, formatted for human eyes. Errors will be flagged with a hyperlink to the error log.
The rest of the sections contain some hyperlinks to informational pages or web-sites.



[SPECIAL NOTES]

To write your own application using this interface, only the files under include directory are needed.
Read the in-file documentation (such as file header, function header, comments), as well as the technical
specification for details of the interface. Normally the class methods listed as public are supposed
to be used by higher level application programs, whereas there is nothing to prevent one from using all
other functions, thanks to PHP. Some test scripts are provided in the test directory to serve as examples.
util.inc is the only file that needs to be explicitly included for everything else to work. 

Functions in gtcm_gnp class and gnp_lock_manager class generally return OPERATION_FAILURE (a defined
constant) upon error, while the details of errors are either logged into the log file or reported to the
screen or both. To find out whether $ret is an error, use 
	<?php 
		if(!strcmp($ret, OPERATION_FAILURE))
			echo "error!"; //process error
	?>

The PHP client behaves differently from the GT.M client when it comes to timed
locks. The interrupt driven approach used by GT.M client was not easily
replicated into the PHP environment. Instead of the GT.M client approach, the
PHP client sets a timer so that when a lock with no time limit is specified, the
client waits for 30 seconds for a response from the server after sending an
LKACQUIRE message. A GT.M client waits for the server to respond to the
LKACQUIRE message.  During this wait, a GT.M client can be forced to cancel
the lock request by keying Ctrl-C.

The socket module in current PHP versions doesn't easily switch between blocking and non-blocking mode.
The simplifying approach taken was to make all reads nonblocking, which entail busy wait at points. This
is not the most efficient way, but it may be the only way.

To test the functionality of lock commands in the demo application (M syntax converter), you need to 
launch a regular GTM client so that it can hold the lock for a sufficiently long period of time. 

[KNOWN BUGS]

$ORDER prints entire key instead of the subscript itself.


