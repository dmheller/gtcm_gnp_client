var m_address;
var m_portno;
var m_tcpSocket;
var defineClass = require('defineClass').defineClass;

var tcp_socket = defineClass({
	constructor: function(server, port){ 

	this.m_address = '127.0.0.1'; //must always be dotted-quad IP
	this.m_portno = 30000;  //must always be number
	this.m_tcpSocket = 0;
	this.tcpSocket = false;

	//Usage: $newSock = new tcp_socket(...)
	var net = require('net').net;
	var client = new net.Socket();
	    client.connect(port, server, function() {
		console.log('CONNECTED TO: ' + server + ':' + port);
		this.m_tcpSocket = client;
		this.m_address = server;
		this.m_portno = port;
		});
	},
	//Usage: sock = aSocket.getSocket()	function getSocket() 
	getSocket: function()
	{
		return this.m_tcpSocket;
	},

	closeSocket: function()
	{
		client.on('close', function() {
		    console.log('Connection closed');
		});
	},
});