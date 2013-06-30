
/* For Testing and examples of how to use this php class */
<?php
class timer2 {
	public $start;
	public $pause_time;

	/*  start the timer  */
	function timer2($start = 0) {
		if($start) { $this->start(); }
	}

	/*  start the timer  */
	function start() {
		$this->start = $this->get_time();
		$this->pause_time = 0;
	}

	/*  pause the timer  */
	function pause() {
		$this->pause_time = $this->get_time();
	}

	/*  unpause the timer  */
	function unpause() {
		$this->start += ($this->get_time() - $this->pause_time);
		$this->pause_time = 0;
	}

	/*  get the current timer value  */
	function get($decimals = 8) {
		return round(($this->get_time() - $this->start),$decimals);
	}

	/*  format the time in seconds  */
	function get_time() {
		list($usec,$sec) = explode(' ', microtime());
		return ((float)$usec + (float)$sec);
	}
}

?>

<?php
require ("util.inc");
/*$lockmgr = new GNP_LOCK_MANAGER();*/
$db1 = new GTCM_GNP("db1", DB1, HOST1, PORTNO);
$db2 = new GTCM_GNP("db2", DB2, HOST2, PORTNO);
/*
echo "> lock ^a:0<br>\n";
$ret = $lockmgr->lockAddImmed("db1", array(array('a')));
if(!strcmp($ret, OPERATION_FAILURE))
	echo "Error<br>\n";
elseif($ret)
	echo "locked<br>\n";
else
	echo "Nope<br>\n";

echo "Sleeping for 3 seconds...<br>\n";
sleep(3);
echo "> lock +^b:0<br>\n";
$ret = $lockmgr->lockIncrAddImmed("db1", array(array('b')));

if (!strcmp($ret, OPERATION_FAILURE))
	echo "Error<br>\n";
elseif($ret)
	echo "Locked<br>\n";
else echo "Nope<br>\n";

echo "> lock +^b:20<br>\n";
$ret = $lockmgr->lockIncrAdd("db1",array(array('b')), 20);

echo "> unlock -^b<br>\n";
$ret = $lockmgr->unlock("db1",array(array('b')));
if(!strcmp($ret, OPERATION_FAILURE))
	echo "Error<br>\n";
elseif($ret)
	echo "UnLocked<br>\n";
else echo "Nope<br>\n";

$val = $db1->PUT(array(0 => 'X', 1 => 1, 2 => 2, 3 => 3), "1,2,3");
$val2 = $db1->PUT(array(0 => 'X', 1 => 1, 2 => 2, 3 => 4), "1,2,4");
$valid = $db1->ORDER(array(0 => 'X', 1 => 1, 2 => 2, 3 => 3));
if(!$valid) 
echo "Failed<br>\n";
else{
$subsc = $db1->PREV(array(0 => 'X', 1 => 1, 2 => 2, 3 => 999999999999999));
$temp = $subsc->m_GDS_key;
$subs = $temp->m_subscripts;
//print_r($subs);
$value = $db1->GET($subsc);
echo "Success!<br>\n";
echo "$value<br>\n";
}
$valid = $db1->ORDER(array(0 => 'X', 1 => 1, 2 => 2, 3 => 3));
if(!$valid) 
echo "Failed<br>\n";
else{
$subsc = $db1->ORDER(array(0 => 'X', 1 => 1, 2 => 2, 3 => 3));
$value = $db1->GET($subsc);
echo "Success!<br>\n";
echo "$value<br>\n";
}
$valid = $db1->ORDER(array(0 => 'X', 1 => 1, 2 => 2, 3 => 4));
if(!$valid)
echo "Failed<br>\n";
else{
$subsc = $db1->ORDER(array(0 => 'X', 1 => 1, 2 => 2, 3 => 3));
$value = $db1->GET($subsc);
echo "Success!<br>\n";
echo "$value<br>\n";
}*/
$timer = new timer2(1);
for ($i = 1; $i <= 10000; $i++) {
$value = $db1->GET(array(0 => 'X', 1 => 1, 2 => 2, 3 => 3));
}
echo "$value<br>\n";
$query_time = $timer->get();
print_r($query_time);
$db1->destroy();
$db2->destroy();
?>
