

<?php

require_once("gtcm_gnp_class.inc");
require_once("constants.inc");
require_once("exception.inc");
require_once("config.inc");

class GNP_DIRECTORY
{
	/* rule examples:
	*  ^abc: all m_globals beginning with abc
	*  abc$: all m_globals ending in abc
	*  .*abc.*: all m_globals containing abc
	*  ^abc$: exactly abc
	*/
	public $m_rules;	//array of array of patterns (one name can have multiple associated rules
	public $m_names; 	//array of GTCM_GNP object names
	public $m_cnt;
	static $instances = array();
	
	/* Usage: $dir = new GNP_DIRECTORY(); */
public	function __construct()
	{
		$this->m_rules = array();
		$this->m_names = array();
		$this->m_cnt = 0;
		self::$instances[] = $this;
	}

	/* Usage: $db1 = $dir->register("db1", "^abc", "~/mumps.dat"[, "127.0.0.1"[,30000]]);
	 *		if(!$db1) //deal with error
	 */
public	function register($name, $rule, $dat_file, $server = DEFAULT_SERVER, $port = DEFAULT_PORT)
	{
		if(in_array($name, $this->m_names))
		{
			$this->m_rules[$name][] = $rule;
			return self::$instances[$name];
		}
		$$name = new GTCM_GNP($name, $dat_file, $server, $port);
		if(!$$name)
		  {
		  return FALSE;
		  }
		$this->m_cnt++;
		$this->m_names[] = $name;
		$this->m_rules[$name] = array($rule);
		return $$name;
	}

	/* Usage: $var_name = $dir->lookUp("Mabc"); 
	*		if(!is_null($var_name))
	*			//do something with $m_globals[$var_name], the GTCM_GNP object
	*/
public	function lookUp($gvn_name)
	{
		foreach($this->m_rules as $name => $rule)
			foreach($rule as $r)
				if(preg_match("/$r/", $gvn_name))
					return $name;		
		return null;
	}

	/* Usage: $dir->unregister("db1"); */
public	function unregister($name)
	{
		if(!in_array($name, $this->m_names))
			return;
		$rule = $this->m_rules[$name];
		$this->m_rules = array_diff($this->m_rules, array($rule));
		$this->m_names = array_diff($this->m_names, array($name));
		$this->m_cnt--;
	}

public	function getRegistered()
	{
		return $this->m_names;
	}

public  function getglobal($i)
    {
    if(array_key_exists($i, $this->m_names)){
    return $this->m_names[$i];
    } else
    return null;
    }
}
?>
