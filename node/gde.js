
var rules;
var names;
var count;
var defineClass = require("./defineClass.js").defineClass;
/* rule examples:
 * ^abc: all globals beginning with abc
	*  abc$: all globals ending in abc
	*  .*abc.*: all globals containing abc
	*  ^abc$: exactly abc
	*/
/* Usage: dir = new gnp_directory(); */

var gnp_directory = defineClass({ 
	constructor: function(){
	this.rules = {};
	this.names = {};
	this.count = 0;
	},
    
	/* Usage: var db1 = new register('db1', '^abc', '/home/daveh/mumps.dat'[, '127.0.0.1'[,30000]]);	 *		if(!$db1) //deal with error
	 */
	register: function(name, rule, dat_file, server, port) {
		
		var gtcm_gnp = require('./gtcm_gnp_class');

		var connections = {};
		var count = 0;
		if (typeof server === 'undefined') {
		    server = '127.0.0.1'; }
		if (typeof port === 'undefined') {
		    port = 30000; }
		
			  if (connections[name]) {
			    connections[name].rules.push(rule);
			    return connections[name].connection;
			  }
			  var newConnection = new gtcm_gnp(dat_file, server, port);
			  count++;
			  connections[name] = {
			    connection: newConnection,
			    rules: [rule]
			  };
			  return newConnection;
			},

	 lookUp: function (gvn_name) {
			  var names = Object.keys(connections);
			  for (var i = 0; i < count; i++) {
			    var name = names[i];
			    if (connections[name].rules.some(function(rule) {
			      return rule.test(gvn_name);
			    })) return name;
			  }
			  return null;
			},

	  unregister: function (name) {
			  if (!connections[name]) return;
			  delete connections[name];
			  count--; 
			},
	  getRegistered: function() {
		  return connections[name];
	  }
 });







