

<?php

require_once("exception.inc");
require_once("constants.inc");
require_once("util.inc");

?>

<?php

/* this class doesn't throwit any m_exception to upper level classes, because any socket-related error is deemed fatal */
class TCP_SOCKET
{
	private $m_address; //must always be dotted-quad IP
	private $m_portno;  //must always be number
	private $m_tcpSocket;
	private $m_blocked;

	//Usage: $newSock = new TCP_SOCKET(...)
public	function __construct($server, $port) 
	{
		$tcpSocket = socket_create(AF_INET,SOCK_STREAM,SOL_TCP) or die("Could not set up socket
listener\n");
		if ($tcpSocket < 0) 
		{
			catchit(throwit(__FILE__, __LINE__, "TCP_SOCKET", 0, socket_strerror($tcpSocket))); 
			return FALSE;	
		}	
		else
		{
			if (defined("DEBUG"))
				echo "Socket is established<br>\n";
			$host = gethostbyname($server);
			$connect_result = socket_connect($tcpSocket, $host, $port);
			
			if (defined("DEBUG"))
				echo "Connecting to $server on port $port...<br>\n";

			if($connect_result < 0)
			{
				catchit(throwit(__FILE__, __LINE__, "TCP_SOCKET", 0, socket_strerror($connect_result)));
				return FALSE;				
			}
			$this->m_tcpSocket = $tcpSocket;
			$this->m_address = $host;
			$this->m_portno = $port; 
			if (defined("DEBUG"))
				echo "Connection to database server " . $server . " established<br>\n";
		}

		$this->m_err = null;
		//hack for lack of set_block, every later synchronous read has to do busy wait
		if(SOCKET_ALWAYS_NONBLOCK == 1)
			$this->setNonBlock();
		else
			$this->setBlock();
	}

	
public	function setBlock()
	{
		$ret = socket_set_block($this->m_tcpSocket);
		if($ret < 0)
		{
			catchit(throwit(__FILE__, __LINE__, "setBlock", 0, "PHP-F-" . socket_strerror($ret)));
		}
		$this->m_blocked = TRUE;
	}

public	function setNonBlock()
	{
		if(defined("MSG"))
		{
			echo "In setNonBlock()<br>\n";
			echo PHP_VERSION, "<br>\n";
		}
		
		$ret = socket_set_nonblock($this->m_tcpSocket);
		
		if($ret < 0)	//fatal error, as it is critical to be able to read sockets asynchronously
		{
			catchit(throwit(__FILE__, __LINE__, "setNonBlock", 0, "PHP-F-" . socket_strerror($ret)));
		}
		$this->m_blocked = FALSE;
	}

	//Usage: $sock = $aSocket->getSocket()	function &getSocket() 
public	function getSocket()
	{
		return $this->m_tcpSocket;
	}

public	function closeSocket()
	{
		socket_close($this->m_tcpSocket);
	}
}

?>
				
	

