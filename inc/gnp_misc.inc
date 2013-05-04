
<?php

#####################
# Auxiliary classes #
#####################

class GDS
{
	/* A GDS key is an internal representation of a global variable name in Greystone Database Structure 
	 * Keys include a name portion and zero or more subscripts. Each subscript may be string or numeric.
	 * Directory Tree Keys never include subscripts
	 * Global Variable Tree keys include subscripts
	 * Single null (ascii 0) bytes separate the variable name and each of the subscripts.
	 * Two contiguous null bytes terminate keys.
	 * String subscripts and numeric subscripts are encoded differently
	 * ----------------------------------------------------------------------------------------------------------
	 * global variable name is represented by an ascii representation of the name excluding the ^
	 * string subscripts are defined as a variable length sequence of 8-bit ascii codes ranging from 0 to 255
	 *	a byte containing hex FF is added to the front of all string subscripts. A null is an acceptable char 
	 *	in keys.
	 * All codes except 00 and 01 represent the corresponding ASCII value
	 * 00 is a terminator
	 * 01 is an indicator to translate the next code using:
	 *		01	means 00	ascii<nul>
	 *		02	means 01	ascii<soh>
	 * Example:
	 *		^NAME("A",$C(2,1,0),"B")
	 *	coded:  4E 41 4D 45 00 FF 41 00 FF 02 01 02 01 01 00 FF 42 00 00
	 *
	 *----------------------------------------------------------------------------------------------------------
	 * Numeric subscripts : [sign bit, biased exponent] [normalized mantissa]
	 * 	zero subscript: represented as a single byte with hex 80
	 *	Mantissa:	normalized by adjusting the exponent;
	 *			creates packed-decimal representation;
	 *			if number has an odd number of digits, append zero;
	 *			adds one to each byte in mantissa.
	 *	Exponent:	stores exponent in first byte of subscript;
	 *			bias exponent by adding hex 3F;
	 *			resulting value is between hex 3F to 7D if positive, or 0 to 3E if negative
	 *	Sign:		sets exponent sign bit<7> in preparation;
	 *			if mantissa is negative, convert each byte of the subscript (including the exponent)
	 *			to its one's-compliment and append a byte containing hex FF to the mantissa.
	 * Example:
	 *		^NAME(.12,0,"STR",-34.56)
	 *	coded:	4E 41 4D 45 00 BE 13 00 80 00 FF 53 54 52 00 3F CA A8 FF 00 00
	 */


	/* variables related to the whole GDS key */

	public $m_global_name;
	public $m_subscripts;
	public $m_encoded;		
	
	public $m_key_top;		//corresponding to gv_key#top
	public $m_key_end;		//corresponding to gv_key#end
	public $m_key_prev;	//corresponding to gv_key#prev
	public $m_key_len;		//top(2) + end(2) + prev(2) + actual encoding + filler(1)

	public $m_order;		//TRUE if ORDER is being encoded or decoded
	public $m_query;
	public $m_prev;

	/* variables related to a numeric GDS key */

	public $m_positive_sub;	//true if positive subscript
	public $m_arrayRep;	//array representation of sign|exp|mantissa, where sign|exp is the first array element
	public $m_exp;		//exponent
	public $m_bare_digits;	//digits stripped of '.', 'e/E', and signs, may be padded with 0 if odd number of digits

	/* CONSTRUCTOR
	*  input: 	a binary string which is encoded, such as when received from the server
	*	 or	an array containing global name and subscripts, as received from the application
	*  output:	none
	*  action:	assign values to the member variables of the class
	*/

public	function __construct($seed_string, $binary=0)
	{
		unset($this->m_encoded);
		$this->m_key_top = 0x4c;	//as per gtcmz_doop
		$this->m_key_prev = 0x00;
		unset($this->m_key_end, $this->m_key_len);

		//if $seed_string is an encoded binary string, as indicated by $binary == 1
		if($binary==1)
		{
			$this->m_encoded = $seed_string;
			$decoded = $this->decode($this->m_encoded);
			$this->m_global_name = array_shift($decoded);
			$this->m_subscripts = $decoded;
		}
		//if $seed_string is an array, just assign it to m_global_name and m_subscripts
		elseif(is_array($seed_string))
		{
			//$seed_string's first element is the name
			$this->m_global_name = array_shift($seed_string);
			$this->m_subscripts = $seed_string;
		}
		else
		{
			assert(false);
		}
		
		if(defined("D"))
		{
			echo "In GDS()<br>\n";
			var_dump($this->m_global_name);
			var_dump($this->m_subscripts);
		}
	}

	/* DECODE
	*  input:	a binary string representing the GDS key to be decoded
	*  output:	$decoded, contains the array of global namd and subscripts
	*  action:	decode global name and subscripts in sequence
	*/

public	function decode($binary_string)
	{
		if(defined("DEBUG"))
			echo "In decode()", "<br>\n";

		//identify key_top, key_end, key_prev
		$packed_key = substr($binary_string, 0, 6);
		$binary_string = substr($binary_string, 6);

		$unpacked = unpack("S3data", $packed_key);
		$this->m_key_top = $unpacked['data1'];
		$this->m_key_end = $unpacked['data2'];
		$this->m_key_prev = $unpacked['data3'];
			
		$decoded = array();

		//rtrim off the ending double NULL's, and explode into an array
		$trimmed = rtrim($binary_string);
		
		if(defined("DEBUG"))
		{
			echo "Trimed string: ";
			$this->showEncoding($trimmed);
		}

		$boundary = 0;
		$bound_so_far = 0;

		//find the global name first
		$boundary = strcspn($trimmed, chr(0));
		$bound_so_far += $boundary;
		$substr = substr($trimmed, 0, $boundary);
		$trimmed = substr($trimmed, $boundary + 1);
		
		$strlen = strlen($substr);
		$template = "a" . $strlen . "glbname";
		$arr = unpack($template, $substr);
		$decoded[] = $arr['glbname'];

		if(defined("DEBUG"))
		 	echo "Global name: ", $decoded[0], "<br>\n";
		 
		while($boundary = strcspn($trimmed, chr(0)))
		{
			$bound_so_far += $boundary + 1;
			
			if(defined("DEBUG"))
				echo "boundary at [" . $bound_so_far . "]<br>\n";
			
			$substr = substr($trimmed, 0, $boundary);
			$trimmed = substr($trimmed, $boundary + 1);
			$decoded[] = $this->decodeSub($substr);
		}
		
		if(defined("DEBUG"))
			$this->showDecoded($decoded);

		return $decoded;			
	}

	/* DECODESUB
	*  input:	a string representing the subscript to be decoded
	*  output:	decoded string
	*  action:	unpack the string into array of 1 byte long unsigned characters,
	*		select decodeStringSub or decodeNumericSub based on first byte of input
	*/

public	function decodeSub($string)
	{
		if(defined("DEBUG"))
			echo "In decodeSub()", "<br>\n";	

		$char_arr = unpack("C*char", $string);

		if(defined("DEBUG"))
		{
			echo "The string is";
			print_r ($char_arr);
		}
				
		//distinguish between string and numbers
		if($char_arr['char1'] == 0xff)
		{
			if(defined("DEBUG"))
				echo "It's a string <br>\n";
				
			array_shift($char_arr);
			return $this->decodeStringSub($char_arr);
		}
		else		
		{
			if(defined("DEBUG"))
				echo "It's a number <br>\n";

			return $this->decodeNumericSub($char_arr);
		}
	}

	/* DECODESTRINGSUB
	*  input:	an array representing the string subscript
	*  output:	$dec_string, the decoded string, with ASCII translated
	*  action:	decode character by character, concatenate result strings along the way
	*/

public	function decodeStringSub($array)
	{
		if(defined("DEBUG"))
			echo "In decodeStringSub()", "<br>\n";		

		$dec_string = "";
		$special = FALSE;				//no 0x01 encountered yet

		if(defined("DEBUG"))
		{
			echo "The string to be decoded is:";
			print_r ($array);
			echo "It has ", count($array), " bytes", "<br>\n";
		}

		$cnt = count($array);

		for($i = 0; $i < $cnt ; $i++)
		{
			if(defined("DEBUG"))
				echo "\$i = ", $i, "<br>\n";

			$char = array_shift($array);
			
			if(defined("DEBUG"))
				echo "ord(\$char) = ", $char, "<br>\n";			
			
			//0x0101 and 0x0102 are special
			if ((!$special) && ($char == 0x01))
			{
				$special = TRUE;
				
				if(defined("DEBUG"))
					echo "0x01 encountered, special to follow...", "<br>\n";

				continue;
			}

			if (($special) && ($char == 0x01))	//ASCII NUL
			{
				$dec_string .= chr(0x00);
				$special = FALSE;

				if(defined("DEBUG"))
					echo "0x00 encountered", "<br>\n";
	
				continue;
			}
		
			if (($special) && ($char == 0x02))	//ASCII SOH					
			{
				$dec_string .= chr(0x01);
				$special = FALSE;
				
				if(defined("DEBUG"))
					echo "0x01 encountered", "<br>\n";

				continue;
			}

			$dec_string .= chr($char);

			if(defined("DEBUG"))
				echo "Normal char", "<br>\n";
		}

		if(defined("DEBUG"))
		{
			echo "\$dec_string to be returned: ";
			var_dump($dec_string);
		}

		return $dec_string;
	}
		
	/* DECODENUMERICSUB
	*  input: 	an array containing the arrays of unsigned bytes of the numeric subscript
	*  output:	$dec_string, the string representation of the number
	*  action:	determine sign, exponent, mantissa in that order
	*/

public	function decodeNumericSub($array)
	{ 
		if(defined("DEBUG"))
			echo "In decodeNumericSub", "<br>\n";

		$cnt = count($array);
		
		if(defined("DEBUG"))
			echo $cnt, " bytes in \$array<br>\n";

		//zero is a special case
		if(($cnt == 1) && (array_shift($array) == 0x80))
			return "0";

		//determine whether the number is positive or negative by looking at the last array element
		$last = array_pop($array);
		
		if ($last == 0xff)
		{
			if(defined("DEBUG"))
				echo "Negative sub", "<br>\n";
			$cnt--;
			$this->m_positive_sub = FALSE;
			$this->m_arrayRep = $array;
		}
		else
		{
			if(defined("DEBUG"))
				echo "Positive sub", "<br>\n";

			$this->m_positive_sub = TRUE;
			array_push($array, $last);
			$this->m_arrayRep = $array;
		}

		if(defined("DEBUG"))
		{
			echo "\$this->m_arrayRep = ";
			print_r ($this->m_arrayRep);
		}

		if (!$this->m_positive_sub)
		{
			$temp_arr = array();
			foreach($this->m_arrayRep as $element)
				$temp_arr[] = ~$element;
			$this->m_arrayRep = $temp_arr;
		}

		if(defined("DEBUG"))
		{
			echo "\$this->m_arrayRep, reverted:";
			print_r ($this->m_arrayRep);
		}

		//clear the sign bit <7>
		$first = array_shift($this->m_arrayRep);
		$first = $first & 0xff;

		if(defined("DEBUG"))
			echo "Exponent with sign bit set is: ", dechex($first), "<br>\n";

		$first = $first - 0x80;
		
		if(defined("DEBUG"))
			echo "Biased exponent is: ", dechex($first), "<br>\n";
		
		//unbias the exponent
		if ($first > 0x3e)
			$this->m_exp = $first - 0x3f;
		else
			$this->m_exp = -(0x3f - $first);

		if(defined("DEBUG"))
			echo "Exponent = ", $this->m_exp, "<br>\n";

		//subtract one from each byte in mantissa
		unset($temp_arr);
		foreach($this->m_arrayRep as $element)
			$temp_arr[] = ($element & 0xff) - 0x01;
		$this->m_arrayRep = $temp_arr;
		
		//revert packed-decimal representation
		$this->unpackDecimal();
		
		//finally, piece together the whole number
		unset($dec_string);
		if ($this->m_positive_sub)
			$dec_string = "+";
		else
			$dec_string = "-";

		$dec_string .= $this->m_bare_digits[0];
		$dec_string .= ".";
		for($i = 1; $i < count($this->m_bare_digits); $i++)
			$dec_string .= $this->m_bare_digits[$i];
		
		//shave off trailing zero's
		$dec_string = str_replace("$+0+$", "", $dec_string);

		if($this->m_exp != 0)
			$dec_string .= "E" . $this->m_exp;
				
		return $dec_string;
	}	

	/* UNPACKDECIMAL
	*  input:	none
	*  output:	none
	*  action:	turn $this->m_arrayRep into $this->m_bare_digits
	*/

public	function unpackDecimal()
	{
		//assumption: $this->m_arrayRep contains  char data unpacked using unpack("C*",...)
		unset($this->m_bare_digits);
		$string = "";
		$cnt = count($this->m_arrayRep);		//there should be 2*$cnt number of digits
		foreach($this->m_arrayRep as $element)
			$string .= pack("C", $element);
		
		$template = "H" . 2*$cnt . "data";
			$arr = unpack($template, $string);
		$data = $arr['data'];

		for ($i = 0; $i < strlen($data); $i++)
			$this->m_bare_digits[] = $data[$i];
	}

	/* ENCODE
	*  input:	none
	*  output:	$this->m_encoded, the encoded string of the GDS key
	*  action:	encode global name, and subscripts, in that order
	*/
	
public	function encode($op_code)
	{
		if($op_code == CMMS_Q_ORDER)
			$this->m_order = TRUE;
		else
			$this->m_order = FALSE;

		if($op_code == CMMS_Q_PREV)
			$this->m_prev = TRUE;
		else
			$this->m_prev = FALSE;
		
		if($op_code == CMMS_Q_QUERY)
			$this->m_query = TRUE;
		else
			$this->m_query = FALSE;

		$this->m_encoded = "";
		
		
		if(!count($this->m_subscripts))
		{
			$this->encodeName(TRUE);
			$this->end();
			$this->m_key_prev = 0x00;
		}
		else
		{
			$this->encodeName(FALSE);
			$this->encodeSubs(); 		
			$this->end();
			
		}

		$this->packKey();

		if(defined("DEBUG"))
		{
			echo "In encode()", "<br>\n";
			$this->showEncoding($this->m_encoded);
		}

		return $this->m_encoded;
	}

	/* PACK_KEY
	*  input:	none
	*  output:      none
	*  action:	append 1 byte of ASCII NULL and prepend top, end, prev
	*/

public	function packKey()
	{
		$this->m_encoded .= pack("C", chr(0));
		$encodedLen = strlen($this->m_encoded);
		$this->m_key_end = $encodedLen - 2;
		$this->m_key_len = $encodedLen + 6;	//sizeof(gv_key) - 2
		$gv_key = "";
		$gv_key .= pack("S3", $this->m_key_top, $this->m_key_end, $this->m_key_prev);
		$this->m_encoded = $gv_key . $this->m_encoded;
	}

	/* SEPARATE
	*  input: 	none
	*  output:	none
	*  action:	append an ASCII NULL to $this->m_encoded
	*/

public	function separate()
	{
		$this->m_encoded .= pack("C",chr(0));
	}
	
	/* END
	*  input:	none
	*  output:	none
	*  action:	append two ASCII NULL's to $this->m_encoded
	*/

public	function end()
	{
		$this->m_encoded .= pack("C", chr(0));
	}

	/* ENCODENAME
	*  input:	none
	*  output:	none
	*  action:	encode the global name according to its ASCII value
	*/
		
public	function encodeName($naked)
	{
		$strLength = strlen($this->m_global_name);
		//only the first 8 characters count
		if($strLength > GNP_GLBNAME_MAX_LENGTH)
			$strLength = GNP_GLBNAME_MAX_LENGTH;
		for($i = 0; $i < $strLength; $i++)
		{
			$code = ord(substr($this->m_global_name, $i, 1));
			$this->m_encoded .= pack("C", $code);
		}
		if(($naked) && ($this->m_order))
			$this->m_encoded .= pack("C", 0x01);
		if(($naked) && ($this->m_query))
			$this->m_encoded .= pack("C2", 0x00, 0x01);
		
		$this->separate();
	}	
		
	/* ENCODESUBS
	*  input:	none
	*  output:	none
	*  action:	encode the subscripts one by one, distinguishing between string and numeric ones
	*/

public	function encodeSubs()
	{
		$subListLength = count($this->m_subscripts);
		$i = 0;
		while($subListLength>0)
		{
			
			$this->m_key_prev = strlen($this->m_encoded);
			//process next
			if(is_numeric($this->m_subscripts[$i]))
				$this->encodeNumericSub($i);
			else
				$this->encodeStringSub($i);
	
			if(($this->m_order) && ($subListLength == 1))
			{
				$this->m_encoded .= pack("C", 0x01);
			}
			
			if(($this->m_query) && ($subListLength == 1))
				$this->m_encoded .= pack("C2", 0x00, 0x01);
			//pad with 00
			$this->separate();		
		
			$subListLength--;
			$i++;
		}

	
			
	}

	/* ENCODENUMERICSUB
	*  input:	index into the array $this->m_subscripts
	*  output:	none
	*  action:	encode $this->m_subscripts[$index] according to the rules regarding numeric subscripts 
	*		laid out in GNP protocol
	*/	

public	function encodeNumericSub($index)
	{
		if(defined("DEBUG"))
			echo "In encodeNumericSub()", "<br>\n";
		
		if($this->m_subscripts[$index] == 0)
		{
			if(defined("DEBUG"))
				echo "Zero subscript encountered<br>\n";
			
			$this->zero();	
		}
		else
		{
			if($this->m_subscripts[$index] > 0)
			{
				if(defined("DEBUG"))
					echo "Positive sub<br>\n";

				$this->m_positive_sub = TRUE;
			}
			else
			{
				if(defined("DEBUG"))
					echo "Negative sub<br>\n";

				$this->m_positive_sub = FALSE;
			}
			
			unset($this->m_arrayRep);

			$this->doMantissa($index);
			$this->doExp();
			$this->doSign();
			$this->assemble(); 		//glue sign|exp|mantissa into m_encoded
		}

	}

	/* ZERO
	*  input:	none
	*  output:	none
	*  action:	encode a numeric subscript of value zero
	*/

public	function zero()
	{
		if(is_null($this->m_encoded))
			$this->m_encoded = "";
		$this->m_encoded .= pack("C", 0x80);
	}

	/* IS_DIGIT
	*  input:	a single character
	*  output:	true if it is a digit [0-9]
	*  action:
	*/

static	function isDigit($char)
	{
		return ((ord($char) >= ord('0')) && (ord($char) <= ord('9')));
	}

	/* STRIP
	*  input:	data to be stripped
	*  output:	none
	*  action:	stip everything escept digits, hacking off leading zero's
	*		place the result in $this->m_bare_digits
	*/

public	function strip($data)
	{
		if(defined("DEBUG"))
			echo "In strip()", "<br>\n";
		
		//need to deal with trailing zero's in integer's too
		//because PHP automatically expands scientific notation to normal notation

		$new = str_replace("$+0+$", "", $data);

		$lead_zero = TRUE;
		unset($this->m_bare_digits);		

		$raw = (string) $new;
		if(defined("DEBUG"))
			echo "Raw data is: " . $raw . "<br>\n";		
		
		for($i = 0; $i < strlen($raw); $i++)
		{
			if($this->isDigit($raw[$i]))
			{
				$digit = (int) $raw[$i];
				if (($digit == 0) && (!$lead_zero))
					$this->m_bare_digits[] = $digit;
				elseif($digit != 0)
				{
					$lead_zero = FALSE;
					$this->m_bare_digits[] = $digit;
				}
				else
				{}
			}
		}

		if(defined("DEBUG"))
		{
			echo "bare digits are: ";
			print_r($this->m_bare_digits);		
		}
	}

	/* NORMALIZE
	*  input:	integer or float to be normalized
	*  output:	number of decimal places to move, positive if large number, negative if small number
	*  action:	find the position of "." and act accordingly
	*
	 * 1	$dotPos === false		==>		integer, return positive number
	 * 2	$dotPos == 0			==>		.xxx, combine with case 4
	 * 3	$dotPos == 1 (a.b) and a != 0	==>		normalized, return 0
	 * 4	$dotPos == 1 (a.b) and a == 0 	==>		find the first non-zero digit, return negative number
	 * 5	$dotPos > 1, combine with case 1, return positive number
	*/

public	function normalize($data)
	{
		if(defined("DEBUG"))
			echo "In normalize", "<br>\n";

		assert(strlen($data));
		$data = abs($data);
		$dotPos = strpos($data, ".");

		if(defined("DEBUG"))
			echo "In normalize(), dotPos = ", $dotPos, "<br>\n";
		
		if($dotPos === false)
			return (strlen($data) - 1);
		$preDot = substr($data, 0, $dotPos);
	
		if(defined("DEBUG"))	
			echo $preDot, "|";

		$postDot = substr($data, $dotPos + 1);

		if(defined("DEBUG"))
			echo $postDot, "<br>\n";

		switch($dotPos)
		{
			case 0: $preDot = 0;
			case 1:
				if($preDot != 0)
					return 0;
				else
				{
					$right = -1;
					for($j = 0; $j < strlen($postDot); $j++)
					{
						$digit = (int) $postDot[$j];
						if($digit != 0)
							break;
						else
							$right--;
					}
					return $right;
				}
			default:	
				return ($dotPos - 1);
		}
	}

	/* DOMANTISSA
	*  input:	index into the array $this->m_subscripts
	*  output:	none
	*  action:
	*
	*  Parse the data first
	*	distinguish between scientific notation and non-scientific notation
	*	normalize and store exponent in m_exp
	* 	store each individual digit in m_bare_digits, appends 0 if odd number of digits
	*/

public	function doMantissa($index)
	{
		if(defined("DEBUG"))
			echo "In doMantissa()", "<br>\n";

		$sub = $this->m_subscripts[$index];		
		$scientific = TRUE;
		
		//scientific numbers
		$ePos = strpos($sub, "e");
		$EPos = strpos($sub, "E");
		if(($ePos === false) && ($EPos === false))
			$scientific = FALSE;
		else
		{
			$expPos = ($ePos === false? $EPos : $ePos);
			
			//get original exponent
			$this->m_exp = substr($sub, $expPos + 1);

			//adjust the exponent if the part prior to $expPos is not normalized
			$mant = substr($sub, 0, $expPos);
			$this->m_exp += $this->normalize($mant);
			
			if(defined("DEBUG"))
				echo "m_exp = ", $this->m_exp, "<br>\n";
		
			$this->strip($mant);
			if(count($this->m_bare_digits)%2)
				$this->m_bare_digits[] = 0;	
		}

		//non-scientific numbers
		if(!$scientific)
		{
			$this->m_exp = $this->normalize($sub);

			if(defined("DEBUG"))
				echo "m_exp= ", $this->m_exp, "<br>\n";
			
			$this->strip($sub);
			if(count($this->m_bare_digits)%2)
				$this->m_bare_digits[] = 0;
		}	
		
		/* Create packed-decimal representation, adding one to each byte in mantissa */
		assert(!(count($this->m_bare_digits)%2));
		$group = count($this->m_bare_digits) / 2;

		if(defined("DEBUG"))
			echo "\$group = ", $group, "<br>\n";
		
		unset($this->m_arrayRep);
		for($i = 0; $i < $group; $i++)
		{
			$data_hi = array_shift($this->m_bare_digits);
			$data_lo = array_shift($this->m_bare_digits);
			$data_lo = $data_lo + 1;
			$data = $data_hi . dechex($data_lo);
					
			if(defined("DEBUG"))
				echo "\$data = ", hexdec($data), "<br>\n";
		
			$this->m_arrayRep[] = $data;
		}

		if(defined("DEBUG"))
			print_r($this->m_arrayRep);
	
	}

	/* DOEXP
	*  input:	none
	*  output:	none
	*  action:	bias the exponent by adding 0x3f, and prepend it to the array representation
	*/

public	function doExp()
	{
		if(defined("DEBUG"))
			echo "In doExp()", "<br>\n";

		$this->m_exp += 0x3F;

		//prepend it to the rest of encoding
		array_unshift($this->m_arrayRep, $this->m_exp);
		
		if(defined("DEBUG"))
		{
			echo "<br>\n ";
			print_r($this->m_arrayRep);
			echo "The biased exponent is: ";
			print (dechex($this->m_arrayRep[0]));
			echo "<br>\n";
		}
	}

	/* DOSIGN
	*  input:	none
	*  output:	none
	*  action:	set bit<7> of the first byte of the encoding
	*/

public	function doSign()
	{
		if(defined("DEBUG"))
			echo "In doSign()", "<br>\n";

		//set exponent sign bit <7>
		$this->m_arrayRep[0] += 0x80;

		if(defined("DEBUG"))
		{
			echo "<br>\n";
			print_r($this->m_arrayRep);
			echo "The exponent now is: ";
			print(dechex($this->m_arrayRep[0]));
			echo "<br>\n";
		}
	}

	/* ASSEMBLE
	*  input:	none
	*  output:	none
	*  action:	glue sign, exponent, mantissa together, place the encoded result in $this->m_encoded
	*/

public	function assemble()
	{
		if(defined("DEBUG"))
			echo "In assemble()", "<br>\n";

		$temp = "";
		$temp .= pack("C", array_shift($this->m_arrayRep));
		
		if(defined("DEBUG"))
		{
			echo "After shifting the exponent,";
			print_r($this->m_arrayRep);
			echo "\$temp length = ", strlen($temp), "<br>\n";
			echo "m_arrayRep has this number of elements: ", count($this->m_arrayRep), "<br>\n";
		}

		for($i = 0; $i < count($this->m_arrayRep); $i++)
		{
			if(defined("DEBUG"))
				echo "m_arrayRep[" . $i . "] -- " . $this->m_arrayRep[$i] . " is being packed.<br>\n";

			//need to adjust for things like 6a
			$temp_char = $this->m_arrayRep[$i];
			if(($temp_char[1] == 'a') || ($temp_char[1] == 'A'))
			{
				if(defined("DEBUG"))
					echo "a encountered", "<br>\n";
				
				$hi = $temp_char[0];
				$hi_hex = dechex($hi);
				$lo = 0x0a;
				$this->m_arrayRep[$i] = (($hi_hex << 4) | ($lo & 0xff));

				if(defined("DEBUG"))
					echo "a data changed to: ", $this->m_arrayRep[$i], "<br>\n";
				$temp .= pack("C", $this->m_arrayRep[$i]);
				continue;
			}
				
				
			if($this->m_arrayRep[$i] >= 10)
				$temp .= pack("H2", $this->m_arrayRep[$i]);
			else
			{
				if(defined("DEBUG"))
					echo "Single digit data need to be left padded<br>\n";
				$temp .= pack("C", $this->m_arrayRep[$i]);
			}

		}
		
		if(defined("DEBUG"))
		{
			echo "Finally, \$temp length = ", strlen($temp);
			echo "<br>\n";
		}

		if(!$this->m_positive_sub)
		{
			$temp = ~$temp;
			$temp .= pack("C", 0xFF);
		}


		if(defined("DEBUG"))
		{
			$ntemp = '';
			$arr = unpack("C*code", $ntemp);
			for($i = 1; $i <= strlen($ntemp); $i++)
			{
				$sub = "code" . $i;
				echo "code [" . $i . "]: " . dechex($arr[$sub]);
				echo "<br>\n";
			}
		}

		$this->m_encoded .= $temp; 
	}

	/* SHOWENCODING
	*  input:	a binary string representing the encoded verion of data
	*  output:	none
	*  action:	display the encoding in a human readable format
	*/

public	function showEncoding($binary)
	{
		if(strlen($binary))
		{
			echo "Encoded: ", "<br>\n";
			$encoded = unpack("C*code", $binary);
			for($i = 1; $i <= count($encoded); $i++)
			{
				$sub = "code" . $i;
				$data = dechex($encoded[$sub]);
				$j = $i - 1;
				echo "code [" . $j . "]: " . $data;
				echo "<br>\n";
			}  
		}
		else
			echo "Subscript is empty!<br>\n";
	}

	/* SHOWDECODED
	*  input:	an array representing the decoded subscripts
	*  output:	none
	*  action:	display the subscripts in a human readable format
	*/

public	function showDecoded($array)
	{
		if(count($array))
		{
			echo "Decoded: ", "<br>\n";
			foreach ($array as $sub)
			{
				var_dump($sub);
			}
		}
	}	

	/* ENCODESTRINGSUB
	*  input:	index into the arraly $this->m_subscripts
	*  output:	none
	*  action:	encode the string subscript character by character
	*/

public	function encodeStringSub($index)
	{
		if(defined("DEBUG"))
			echo "In encodeStringSub()", "<br>\n";
		
		if(($index == (count($this->m_subscripts) - 1)) && 
				($this->m_subscripts[$index] == "")&& 
				($this->m_order || $this->m_query || $this->m_prev))
		{
			return;
		}
		//prefix with 0xFF
		$this->m_encoded .= pack("C", 0xFF);

		//now go through each character, paying attention to 0x00 and 0x01
		$strLength = strlen($this->m_subscripts[$index]);
		
		if(defined("DEBUG"))
		{
			echo "\$index = " . $index . "<br>\n";
			echo "\$this->m_subscripts:";
			var_dump($this->m_subscripts);
			echo "\$this->m_subscripts[$index] has length: " . $strLength . "<br>\n";
		}

		for($i = 0; $i < $strLength; $i++)
		{	
			$ord = ord($this->m_subscripts[$index][$i]);
			if(defined("DEBUG"))
				echo "\$ord = " . $ord . "<br>\n";

			if($ord ==0)
			{
				$this->m_encoded .= pack("C", 0x01);
				$this->m_encoded .= pack("C", 0x01);
				if(defined("DEBUG"))
					echo "0x00 encountered for encoding", "<br>\n";
			}
			elseif($ord ==1)
			{
				$this->m_encoded .= pack("C", 0x01);
				$this->m_encoded .= pack("C", 0x02);
				if(defined("DEBUG"))
					echo "0x01 encountered for encoding", "<br>\n";
			}
			else
			{
				$this->m_encoded .= pack("C", $ord);
				if(defined("DEBUG"))
					echo "Normal character for encoding: $ord<br>\n";
			}
		}
	}

}

class SUBSC
{
	public $m_reg_no;
	public $m_GDS_key;

public  function __construct($reg_no, $GDS_key = '')
	{
		$this->m_reg_no = $reg_no;
		$this->m_GDS_key = $GDS_key;
	}

	//this function should be redefined to suit the user collating sequence, if necessary
public   function before($subsc)
	{
		return (strcmp($this->m_GDS_key->m_global_name, $subsc->m_GDS_key->m_global_name) < 0);
	}

}

class REGION_INFO
{
	public $m_region_no;
	public $m_nullsubs;
	public $m_max_rec_len;
	public $m_max_subsc_len;

public  function __construct($reg_no, $nullsubs, $max_rec_len, $max_subsc_len)
	{
		$this->m_region_no = $reg_no;
		$this->m_nullsubs = $nullsubs;
		$this->m_max_rec_len = $max_rec_len;
		$this->m_max_subsc_len = $max_subsc_len;
	}
}

class PROTOCOL
{
	#########################
	#	DATA MEMBERS	#
	#########################

	public $m_cpu_type;
	public $m_os;
	public $m_implementation;
	public $m_version;
	public $m_proto_type;
	public $m_proto_level;
	public $m_endian;	//'B' for big-endian
	public $m_proto_filler;
	//filled with space

	#################################
	#	PUBLIC FUNCTIONS	#
	#################################

public	function __construct()
	{
		
		if(!defined("S_PROTOCOL"))
		{		
			$num_args = func_num_args();

			if(!$num_args)
			{
				$this->m_cpu_type = $this->encodeCPU(GTM_RELEASE_NAME);
				$this->m_os = $this->encodeOS(GTM_RELEASE_NAME);
				$this->m_implementation = (string) PROTO_IMPLEMENTATION;
				$this->m_version = (string) PROTO_VERSION;
				$this->m_proto_type = (string) CMM_PROTOCOL_TYPE;
				$this->m_proto_level = (string) PROTO_LEVEL;
				if($this->isBigEndian($this->m_cpu_type))
					$this->m_endian = 'B';
				else
					$this->m_endian = ' ';
				$this->m_proto_filler = (string)S_PROTOCOL_FILLER;
			}
			else	//the argument is a 33 byte string
			{
				$arg = func_get_arg(0);
				$arg = substr($arg, 0, CM_PROTOCOL_LENGTH);
							
				$split = chunk_split($arg, 3, '\0');
				$arr = explode('\0', $split);
				array_pop($arr);
				
				$this->m_cpu_type = $arr[0];
				$this->m_os = $arr[1];
				$this->m_implementation = $arr[2];
				$this->m_version = $arr[3];
				$this->m_proto_type = $arr[4];
				$this->m_proto_level = $arr[5];
				if($this->isBigEndian($this->m_cpu_type))
					$this->m_endian = 'B';
				else
					$this->m_endian = ' ';
				$this->m_proto_filler = $arr[6];
			}
		}
		else
		{
			$split = chunk_split(S_PROTOCOL, 3, '\0');
			$arr = explode('\0', $split);
			array_pop($arr);
			
			$this->m_cpu_type = $arr[0];
			$this->m_os = $arr[1];
			$this->m_implementation = $arr[2];
			$this->m_version = $arr[3];
			$this->m_proto_type = $arr[4];
			$this->m_proto_level = $arr[5];
			if($this->isBigEndian($this->m_cpu_type))
				$this->m_endian = 'B';
			else
				$this->m_endian = ' ';
			$this->m_proto_filler = $arr[6];
		}
		assert(strlen($this->m_cpu_type) == 3);
		assert(strlen($this->m_os) == 3);
		assert(strlen($this->m_implementation) == 3);
		assert(strlen($this->m_version) == 3);
		assert(strlen($this->m_proto_level) == 3);
		assert(strlen($this->m_proto_type) == 3);
		assert(strlen($this->m_endian) == 1);

		if(defined("DEBUG"))
		{
			echo "Endian in PROTOCCOL:";
			var_dump($this->m_endian);
		}
	
	}

public	function matchEndian($peer)
	{
		return TRUE;	// Debug
		if(!defined("S_PROTOCOL"))
		{
			return ($this->m_endian == $peer->m_endian);
		}
		return TRUE;
	}

public	function match($peer)
	{
		return TRUE;	// Debug
		if(!defined("S_PROTOCOL"))
		{
			if(strcmp($this->m_version, $peer->m_version))
				return FALSE;
			if(strcmp($this->m_proto_type, $peer->m_proto_type))
				return FALSE;
			if(strcmp($this->m_proto_level, $peer->m_proto_level))
				return FALSE;
			return TRUE;
		}
		else return TRUE;
	}

	#################################
	#	PRIVATE FUNCTIONS	#
	#################################

private	function encodeCPU($rname)
	{
		$arr = explode(" ", $rname);
		$cpu = $arr[3];
		
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
		return  substr($cpu, 0, 3);   
	}

private	function encodeOS($rname)
	{
		$arr = explode(" ", $rname);
		$os = $arr[2];

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
		return  substr($os, 0, 3);   
	}		
	
private	function isBigEndian($cpu)
	{
		$myvar = unpack("S1newval", "10");
		if ($myvar['newval'] == 0x3031)
			return FALSE;
		else
		{
			assert($myvar['newval'] == 0x3130);
			return TRUE;
		}
	}
}

?>
