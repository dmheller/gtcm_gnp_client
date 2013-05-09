

<?php
//Make sure to edit config.inc first
include_once("config.inc");
//Make sure to edit php.ini if necessary, especially include_path

$curDir = getcwd();
$addonDir = $curDir . "/inc/";
$file = $addonDir . "util.inc";

//util.inc is the only file that needs to be explicitly 'included'
include_once($file);
?>


<HTML>

<?php
include("./support/header.hd");
?>

<?php
//format for variable in lock commands: a(1,2);b(a);c(dd,4)
function parseLockRef()
{
	global $lock_gvns, $variable;
	$lock_gvns = (array)getParsed(";", $variable);
}


function getParsed($delim, $str)
{
	$arr = (array) explode($delim, $str);
	return $arr;
}

/*  parses the input and returns an array of name arrays
*/
function mkNameArray($parsed)
{
	$cnt = count($parsed);
	assert($cnt > 0);
	$arr = array();
	while($gvn = array_shift($parsed))
		$arr[] = makeOneName($gvn);
	return $arr;
}

function makeOneName($gvn)
{
	$name_arr = array();
	$subs = array();
	$name_arr[]= getName($gvn);
	$subs = getSubs($gvn);
	while($sub = array_shift($subs))
		$name_arr[] = $sub;
	return $name_arr;
}

function getName(&$str)
{
	$p_pos = strcspn($str, "(");
	assert($p_pos > 0);
	$g_name = substr($str, 0, $p_pos);
	$str = substr($str, $p_pos);
	return $g_name;
}
	
function getSubs($str)
{
	//get rid of the parentheses first
	$len = strlen($str);
	$string = substr($str, 1, ($len - 2));
	$subs = makeArr($string);
	return $subs;
}

function makeArr($str)
{
	return getParsed(",", $str);
}

function makeGVN($gds)
{
	if(is_null($gds))
		return "NULL";
	else
	{
		$name = "^" . $gds->m_global_name;
		$subs = $gds->m_subscripts;
		$cnt = count($subs);
		if($cnt)
		{ 
			$str = "(";
			while($sub = array_shift($subs))
			{
				$cnt--;
				if(is_numeric($sub)) $str .= number_format($sub,0,'','');
				else $str .= "\"$sub\"";
				if($cnt) $str .= ",";
			}
			$str .= ")";
		}
		$str = $name . $str;
	}
	return $str;
}

function isValid($inv)
{
	global $command, $variable, $value, $submit, $time_limit;
	$submit = $_POST['submit'];
	if((empty($variable)) && ($command != "TERMINATE") && ($command != "UNLOCKALL") && ($command != "ZDEALLOCATEALL"))
	{
		$inv = "Missing required field: VARIABLE";
		return FALSE;
	}
	if(($command == "PUT") && (empty($value)))
	{
		$inv = "Missing required field: VALUE";
		return FALSE;
	}
	if(!empty($variable))
	{
		$o_cnt = substr_count($variable, "(");
		$c_cnt = substr_count($variable, ")");
		if ($o_cnt != $c_cnt)
		{
			$inv = "Parentheses not balanced in VARIABLE";
			return FALSE;
		}
	} 
	if(!empty($time_limit) && (($command == "LOCKADD") || ($command == "LOCKINCRADD") 
		|| ($command == "ZALLOCATE")) && ($time_limit < 1))
	{
		$inv = "Time limit must be integers no less than one";
		return FALSE;
	}
	
	return TRUE;
}

function cmdName($cmd)
{
	switch($cmd)
	{
		case "QUERY":	
		case "ORDER": 	
		case "DATA":	return "\$" . $cmd;

		case "PREV":	return "\$ZPREVIOUS";
		case "WITHDRAW": return "\$ZWITHDRAW";
	
		case "KILL":	
		case "GET":		
		case "PUT":   	
		case "TERMINATE": return $cmd;
		case "LOCKADD":
		case "LOCKINCRADD":
		case "LOCKADDIMMED":
		case "LOCKINCRADDIMMED":
		case "UNLOCK":
		case "UNLOCKALL": return "LOCK";

		case "ZALLOCATE":
		case "ZALLOCATEIMMED": return "ZALLOCATE";

		case "ZDEALLOCATE":
		case "ZDEALLOCATEALL": return "ZDEALLOCATE";

		default: assert(FALSE);
	}
}
		
function cmdAddRef($arr) //this is already an array made by makeOneName
{
	$string = "^";
	$string .= array_shift($arr);

	$cnt = count($arr);
	if($cnt > 0)
		$string .= "(";
	while($cnt > 0)
	{
		$sub = array_shift($arr);
		if(is_numeric($sub))
			$string .= $sub;
		else
		{
			$last = strlen($sub)- 1;
			if(($sub[0] != "\"") && ($sub[$last] != "\""))
				$string .= "\"" . $sub . "\"";
			elseif($sub[0] != "\"")
				$string .= "\"" . $sub;
			elseif($sub[$last] != "\"")
				$string .= $sub . "\"";
			else
				$string .= $sub;
		}
		if($cnt > 1)
			$string .= ",";
		else
			$string .= ")";
		$cnt--;
	}
	return $string;
}

function cmdAddLockRef()
{
	global $lock_gvns, $command, $time_limit;

	$arr = $lock_gvns;
	$cnt = count($arr);
	assert ($cnt > 0);
	switch($command)
	{
		case "LOCKINCRADD":
		case "LOCKINCRADDIMMED": $string = "+";
					break;
		case "UNLOCK":		$string = "-";
					break;
		default: 		$string = "";
					break;
	}

	if($cnt > 1)
		$string .= "(";
	$left = $cnt; 
	while($gvn = array_shift($arr))
	{
		$left--;
		$string .= cmdAddRef(makeOneName($gvn));
		if($left > 0)
			$string .= ", ";
	}
	if($cnt >1)
		$string .= ")";
	switch($command)
	{
		case "LOCKADD":
		case "LOCKINCRADD":
		case "ZALLOCATE": 	if(empty($time_limit))
						$string .= ":" . CLIENT_WAIT_TIMEOUT;
					else $string .= ":" . $time_limit;
				  	break;
		case "LOCKADDIMMED":	
		case "LOCKINCRADDIMMED":
		case "ZALLOCATEIMMED":	$string .= ":0";
					break;
		default: break;
	}
	return $string;		
}

/*  parses the input and builds the M command
*/
function mkCmd()
{
	global $command, $variable, $value, $submit, $time_limit, $valid, $global_ref, $inv;
	$variable = $_POST['variable'];
	$command = $_POST['command'];
	$value = $_POST['value'];
	$time_limit = $_POST['time_limit'];
	if(!isValid($inv))
	{
		$valid = FALSE;
		return $inv;
	}

	$string .= cmdName($command);
	if(($command == "TERMINATE") || ($command == "UNLOCKALL") || ($command == "ZDEALLOCATEALL"))
	{	$valid = TRUE;	
	return $string; }
	$string .= " ";
	switch($command)
	{
		case "QUERY":	
		case "ORDER": 	
		case "DATA":	
		case "PREV":	
		case "WITHDRAW": 
		case "KILL":	
		case "GET":		
		case "PUT":   	$string .= cmdAddRef($global_ref = makeOneName($variable));
				break;
	
		case "LOCKADD":
		case "LOCKINCRADD":
		case "LOCKADDIMMED":
		case "LOCKINCRADDIMMED":
		case "UNLOCK":
		case "ZALLOCATE":
		case "ZALLOCATEIMMED": 
		case "ZDEALLOCATE":  parseLockRef();
				    $string .= cmdAddLockRef();
				    break;

		default: assert(FALSE);
	}
	 $_POST['global_ref'] = $global_ref;
	if((!empty($value)) && ($command == "PUT"))
	{
		$string .= "=";
		$sub = stripcslashes($value);
		if(is_numeric($sub))
			$string .= $sub;
		else
		{
			$last = strlen($sub)- 1;
			if(($sub[0] != "\"") && ($sub[$last] != "\""))
				$string .= "\"" . $sub . "\"";
			elseif($sub[0] != "\"")
				$string .= "\"" . $sub;
			elseif($sub[$last] != "\"")
				$string .= $sub . "\"";
			else
				$string .= $sub;
		}
	}
	
	$valid = TRUE;
	return $string;
}
	
if(isset($reset))
{
	$response = "";
	$command = "";
	$variable = "";
	$value = "";
	$time_limit = "";
	$cmd = "";
	$submit = "";
	$reset = "";
	$valid = "";
	$global_ref = "";
	$lock_gvns = "";
}

$lock_gvns = array();
$cmd = mkCmd();

//The following 4 lines are critical, as they specify the network environment, but $gld and $lockmgr are not
//required. The GTCM_GNP class has interface functions that can handle individual GNP messages
$gld = new GNP_DIRECTORY();
$db_1 = $gld->register("db_1", "^[^aA]", DB1, HOST1, PORTNO);
$db_2 = $gld->register("db_2", "^[aA]",  DB2, HOST2, PORTNO);
$lockmgr = new GNP_LOCK_MANAGER($gld);
//------------------------------------------------
if((isset($submit)) && ($submit == "submit") && ($valid))
{	

	switch($command)	//Note: PHP function names are case INsensitive
	{
		case "QUERY":	
		case "ORDER": 	
		case "PREV":	$db_name = $gld->lookUp($global_ref[0]);
				$response = $$db_name->{$command}($global_ref);
				if(is_string($response))
				 if(preg_match("/OPERATION_FAILURE/i",$response))
					$response = "ERROR, see <a href=\"showerr.php\">error.log</a>";
				$response = makeGVN($response->m_GDS_key);
				print_r($response->m_GDS_key->m_subscripts);
				break;

		case "DATA":	
		case "KILL":	
		case "GET":		
		case "WITHDRAW":$db_name = $gld->lookUp($global_ref[0]);
				$response = $$db_name->{$command}($global_ref);		
				if(preg_match("/OPERATION_FAILURE/i",$response))
					$response = "ERROR, see <a href=\"showerr.php\">error.log</a>";
				elseif((TRUE == $response) && (($command == "KILL") || ($command == "WITHDRAW")))
				{	
					$response = "OK";
				}
				else 
			        {}
				break;

		case "PUT":   	$db_name = $gld->lookUp($global_ref[0]);
				$response = $$db_name->{$command}($global_ref, $value);
				if(preg_match("/OPERATION_FAILURE/i",$response))
					$response = "ERROR, see <a href=\"showerr.php\">error.log</a>";
				elseif($response == TRUE)
					$response = "OK";
				else 
				{}
				break;

		case "TERMINATE": $db_names = $gld->getRegistered();
				$success = TRUE;
				foreach($db_names as $db_n)
				{
					$response = $$db_n->{$command}();
					if(preg_match("/OPERATION_FAILURE/i",$response))
						$success = FALSE;
					$$db_n->destroy();
				}
				if($success === TRUE)
					$response = "DISCONNECTED";
				else $response = "ERROR, see <a href=\"showerr.php\">error.log</a>";
				break;

		case "LOCKADD":
		case "LOCKINCRADD":
		case "ZALLOCATE": 
				$lock_param = mkNameArray($lock_gvns);
				
				  if(!empty($time_limit))
				  	array_push($lock_param, $time_limit);
				  else
					array_push($lock_param, CLIENT_WAIT_TIMEOUT);
				$com = $command . "_3";	
				$response = $lockmgr->{$com}($lock_param);
				if(preg_match("/OPERATION_FAILURE/i",$response))
					$response = "ERROR, see <a href=\"showerr.php\">error.log</a>";
				elseif($response == TRUE)
					$response = "LOCKED";
				else $response = "NOT LOCKED"; 
				break;

		case "LOCKADDIMMED":
		case "LOCKINCRADDIMMED":
		case "ZALLOCATEIMMED": $lock_param = mkNameArray($lock_gvns);
				$com = $command . "_3";
				$response = $lockmgr->{$com}($lock_param);
				if(preg_match("/OPERATION_FAILURE/i",$response))
					$response = "ERROR, see <a href=\"showerr.php\">error.log</a>";
				elseif($response == TRUE)
					$response = "LOCKED";
				else $response = "NOT LOCKED"; 
				break;

		case "UNLOCK":
		case "ZDEALLOCATE":$lock_param = mkNameArray($lock_gvns);
				$com = $command . "_3";		
				$response = $lockmgr->{$com}($lock_param);
				if(preg_match("/OPERATION_FAILURE/i",$response))
					$response = "ERROR, see <a href=\"showerr.php\">error.log</a>";
				elseif($response == TRUE)
					$response = "LOCK CANCELLED";
				else {};
				break;
		case "UNLOCKALL":
		case "ZDEALLOCATEALL": $response = $lockmgr->{$command}();
				if(preg_match("/OPERATION_FAILURE/i",$response))
					$response = "ERROR, see <a href=\"showerr.php\">error.log</a>";
				elseif($response == TRUE)
					$response = "LOCK CANCELLED";
				else {};
				break;

		default:	$response = "UNRECOGNIZED COMMAND";
				break;
	}
}

?>
<?
//The following are just for this particular application's sake
?>

<BODY >

<!--DIV CLASS=MAIN-->


<Form action=<?php echo $_SERVER['PHP_SELF'] . "#FEEDBACK"; ?> method="post">
<input type=hidden name="valid" value=<?php echo $valid;?>>
<DIV CLASS=INTROBLOCK>

<H1 CLASS=INTROHEAD>
<span style='font-size:28.0pt;mso-bidi-font-size:10.0pt;font-family:Helvetica;color:#FF6600'><b><font size=+3>M&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</font></span>
<span style='font-size:28.0pt;mso-bidi-font-size:10.0pt;font-family:Helvetica;color:blue'><font size=+3>M&nbsp;</span><span style='font-size:28.0pt;mso-bidi-font-size:10.0pt;font-family:Helvetica;color:#FFCC66'>a&nbsp;</span><span style='font-size:28.0pt;mso-bidi-font-size:10.0pt;font-family:Helvetica;color:#339966'>d&nbsp;</span><span style='font-size:28.0pt;mso-bidi-font-size:10.0pt;font-family:Helvetica;color:#00CCFF'>e&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</font>&nbsp;</span>
<span style='font-size:28.0pt;mso-bidi-font-size:10.0pt;font-family:Helvetica;color:#9900CC'><font size=+3>E&nbsp;</span><span style='font-size:28.0pt;mso-bidi-font-size:10.0pt;font-family:Helvetica;color:#FFFF66'>a&nbsp;</span><span style='font-size:28.0pt;mso-bidi-font-size:10.0pt;font-family:Helvetica;color:red'>s&nbsp;</span><span style='font-size:28.0pt;mso-bidi-font-size:10.0pt;font-family:Helvetica;color:lime'>i&nbsp;</span><span style='font-size:28.0pt;mso-bidi-font-size:10.0pt;font-family:Helvetica;color:#3366FF'>e&nbsp;</span><span 
style='font-size:28.0pt;mso-bidi-font-size:10.0pt;font-family:Helvetica;color:#FF9900'>r</font></b></span>
</H1>


<P ><SPAN CLASS=INITCAP>W</SPAN>elcome to M Made Easier
</P>
<P CLASS=INTROTEXT>
You can do the following at this site:
</P>


<UL CLASS=SQUAREDISCS>
<LI><A HREF="#ISSUE">Describe and issue command</A>
<LI><A HREF="#FEEDBACK">Check the result of your command execution</A>
<LI><A HREF="#INFO">Find out more about this site and M server/protocol</A>
<LI><A HREF="#LINKS">Useful links</A>
</UL>

</DIV>

<DIV CLASS = ISSUE>

<A NAME="ISSUE">
<H1 CLASS=ISSUEHEAD>

I&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
S&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
S&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
U&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
E
</H1>
</A>


<p><table border=0><tr><td colspan=2>
<span style='font-size:16.0pt;mso-bidi-font-size:10.0pt;font-family:Helvetica;color:blue'><b><font size=+1>Step 1: Select a command, provide global reference and value</font></b>
</span></td>
</tr>

<tr><td>
<span style='font-size:12.0pt;mso-bidi-font-size:10.0pt;font-family:Helvetica'><b>Command</b><font size="-1"><i>(required)</i></font>
</span></td><td>
<span style='font-size:16.0pt;mso-bidi-font-size:10.0pt;font-family:Helvetica'>
<select NAME="command"><font color="#000000">

<?php
$options = array("UNKNOWN" => "UNKNOWN", "GET" => "GET", "PUT" => "PUT", "ORDER" => "\$ORDER", "PREV" => "\$ZPREVIOUS",
		 "DATA" => "\$DATA", "QUERY" => "\$QUERY", "WITHDRAW" => "\$ZWITHDRAW", "KILL" => "KILL",
		 "LOCKADD" => "LOCK, NONINCREMENTAL", "LOCKADDIMMED" => "LOCK, IMMEDIATE", "LOCKINCRADD" => "LOCK, INCREMENTAL",
		 "LOCKINCRADDIMMED" => "LOCK, INCREMENTAL, IMMEDIATE", "UNLOCK" => "UNLOCK, DECREMENTAL", 
		 "UNLOCKALL" => "UNLOCK ALL", "ZALLOCATE" => "ZALLOCATE", "ZALLOCATEIMMED" => "ZALLOCATE, IMMEDIATE",
		 "ZDEALLOCATE" => "ZDEALLOCATE", "ZDEALLOCATEALL" => "ZDEALLOCATE ALL", "TERMINATE" => "TERMINATE");

if(!empty($command))
	echo "<option VALUE=" . $command . ">" . $options[$command];
foreach($options as $key => $opt)
{
	if(empty($command) || ($command != $key))
		echo "<option VALUE=" . $key . ">" . $opt;
}
?>
	
</font></select>
</span></td></tr><tr><td>
<span style='font-size:12.0pt;mso-bidi-font-size:10.0pt;font-family:Helvetica'><b>Global Reference</b><br>
<font size="-1"><i>e.g. a(1,2);b(ln)&nbsp;<b>no space inbetween, no quotes<b></i></font>
</span></td><td>
<span style='font-size:16.0pt;mso-bidi-font-size:10.0pt;font-family:Helvetica;color:blue'>
<input TYPE="TEXT" MAXLENGTH="200" SIZE="50" NAME="variable" VALUE=<?php echo $_POST['variable']; ?>>
</span></td></tr><tr><td>
<span style='font-size:12.0pt;mso-bidi-font-size:10.0pt;font-family:Helvetica'><b>Value</b><font size="-1"><i>(required for SET)</i></font>
</span></td><td>
<span style='font-size:16.0pt;mso-bidi-font-size:10.0pt;font-family:Helvetica;color:blue'>
<input TYPE="TEXT" MAXLENGTH="200" SIZE="15" NAME="value" VALUE=<?php echo $_POST['value']; ?>>
</span></td></tr><tr><td>
<span style='font-size:12.0pt;mso-bidi-font-size:10.0pt;font-family:Helvetica'><b>Time Limit</b><font size="-1"><i>(required for TIMED LOCK)</i></font>
</span></td><td>
<span style='font-size:16.0pt;mso-bidi-font-size:10.0pt;font-family:Helvetica;color:blue'>
<input TYPE="TEXT" MAXLENGTH="200" SIZE="15" NAME="time_limit" VALUE=<?php echo $_POST['time_limit']; ?>>
</span></td></tr>
<tr><td>
<input type="submit" name="submit" value="submit">
</td><td>
<input type="submit" name="reset" value="reset"></td>
</tr>
<br clear></table>
</p>

<!-- this ends the ISSUE section -->
</DIV>


<DIV CLASS=FEEDBACK>

<A NAME="FEEDBACK">
<H1 CLASS=FEEDBACKHEAD>
F &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
E &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
E &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
D &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
B &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
A &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
C &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
K
</H1></A>
<br><br><br><br><br><br>
<P><table border=1 cellpadding=10><b><tr><td><font color=#222266>
<SPAN CLASS=INITCAP>T</SPAN>he command parameters you entered:</font></td>
<td><font color=#222266>M command is:</font></td>
<td><font color=#222266>The response is:</font></td></tr></b>
<tr><td><font color=#222266>
<ul>
<li>Command: &nbsp;&nbsp;<?php echo $_POST['command']; ?>
<li>Global reference: &nbsp;&nbsp;<?php echo $_POST['variable']; ?>
<?php 
if(!empty($value))
	echo "<li>Value: &nbsp;&nbsp;$value"; 
if(!empty($time_limit))
	echo "<li>Time limit: &nbsp;&nbsp;$time_limit";

	echo "<li>Server:Region:Port: " . DEFAULT_SERVER . ": " . DB1 . ": " . PORTNO . "";  
?>
</ul></font></td><td><font color=#222266>
<?php  
if((!empty($submit)) && (empty($reset)))
	echo $cmd, "<br>"; ?></font></td>
<td><font color=#222266>
<?php  
if(isset($response))
print_r($response); 

?></font></td></tr></table>
<BR CLEAR=ALL>
</P>
<!--end of FEEDBACK-->
</DIV>


<DIV CLASS=INFO>
<A NAME="INFO">
<H1 CLASS=INFOHEAD>
A &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
B &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
O &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
U &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
T 
</H1></A>
<br><br><br><br><br><br>


<?php

echo "<ul>";
echo "<li><a href=\"support/about.php\">" . "About this site" . "</a>";
echo "<li><a href=\"support/aboutServer.php\">" . "Available servers" . "</a>";
echo "<li><a href=\"support/aboutProtocol.php\">" . "Available protocols" . "</a>";
echo "</ul>";

?>

<BR CLEAR>
</P>

<!--end of ABOUT-->
</DIV>



<DIV CLASS=LINKS>
<A NAME="LINKS">
<H1 CLASS=LINKSHEAD>
L &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
I &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
N &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
K &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
S 

</H1></A>
<br><br><br><br><br><br>
<?php
echo "<ul>
	<li><a href=\"http://en.wikipedia.org/wiki/MUMPS\">MUMPS Language</a>
	<li><a href=\"http://www.fisglobal.com/products-technologyplatforms-gtm-userdocumentation\">GT.M Documnetation</a>
  </ul>";
?>
<BR CLEAR>
</P>
<!--end of LINKS-->
</DIV>

</form>

<!--end of everything-->
<!--/DIV-->

<HR>
<P CLASS=INBACK>
<center>
<address>Copyright A&D Electronics East Rochester NY 14445</address>
Author:Yi Zheng, David M. Heller
&nbsp;Last updated on 04/19/2013.
</center>
</P>

</BODY>
</html>
