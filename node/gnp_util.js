

var c = require('constants');
var m = require('./util');
var _ = require('underscore')._;
var defineClass = require("./defineClass.js").defineClass;

function gds(seed_string, binary)
{
	/* A gds key is an internal representation of a global variable name in Greystone Database Structure 
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
	 *		^NAME('A',$C(2,1,0),'B')
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
	 *		^NAME(.12,0,'STR',-34.56)
	 *	coded:	4E 41 4D 45 00 BE 13 00 80 00 FF 53 54 52 00 3F CA A8 FF 00 00
	 */


	/* variables related to the whole gds key */

	 m_global_name;
	 m_subscripts;
	 m_encoded;		
	
	 m_key_top;		//corresponding to gv_key#top
	 m_key_end;		//corresponding to gv_key#end
	 m_key_prev;	//corresponding to gv_key#prev
	 m_key_len;		//top(2) + end(2) + prev(2) + actual encoding + filler(1)

	 m_order;		//true if ORDER is being encoded or decoded
	 m_query;
	 m_prev;

	/* variables related to a numeric gds key */

	 m_positive_sub;	//true if positive subscript
	 m_arrayRep;	//array representation of sign|exp|mantissa, where sign|exp is the first array element
	 m_exp;		//exponent
	 m_bare_digits;	//digits stripped of '.', 'e/E', and signs, may be padded with 0 if odd number of digits

	/* CONSTRUCTOR
	*  input: 	a binary string which is encoded, such as when received from the server
	*	 or	an array containing global name and subscripts, as received from the application
	*  output:	none
	*  action:	assign values to the member variables of the class
	*/

	
	{
		if (typeof binary === 'undefined') {
		    binary = 0; }
		(this.m_encoded);
		this.m_key_top = 0x4c;	//as per gtcmz_doop
		this.m_key_prev = 0x00;
		(this.m_key_end, this.m_key_len);

		//if $seed_string is an encoded binary string, as indicated by $binary == 1
		if(binary==1)
		{
			this.m_encoded = seed_string;
			decoded = this.decode(this.m_encoded);
			this.m_global_name = array_shift(decoded);
			this.m_subscripts = decoded;
		}
		//if $seed_string is an array, just assign it to m_global_name and m_subscripts
		else if(is_array(seed_string))
		{
			//$seed_string's first element is the name
			this.m_global_name = array_shift(seed_string);
			this.m_subscripts = seed_string;
		}
		else
		{
			assert(false);
		}
		
		if(defined('D'))
		{
			document.write( 'In gds()<br>\n');
			var_dump(this.m_global_name);
			var_dump(this.m_subscripts);
		}
	};

	/* DECODE
	*  input:	a binary string representing the gds key to be decoded
	*  output:	$decoded, contains the array of global namd and subscripts
	*  action:	decode global name and subscripts in sequence
	*/

	this.decode = function(binary_string)
	{

		//identify key_top, key_end, key_prev
		packed_key = substr(binary_string, 0, 6);
		binary_string = substr(binary_string, 6);

		unpacked = unpack('S3data', packed_key);
		this.m_key_top = unpacked['data1'];
		this.m_key_end = unpacked['data2'];
		this.m_key_prev = unpacked['data3'];
			
		decoded = {};

		//rtrim off the ending double null's, and explode into an array
		trimmed = rtrim(binary_string);
		
		boundary = 0;
		bound_so_far = 0;

		//find the global name first
		boundary = strcspn(trimmed, chr(0));
		bound_so_far += boundary;
		substr = substr(trimmed, 0, boundary);
		trimmed = substr(trimmed, boundary + 1);
		strlen = strlen(substr);
		template = 'a' + strlen + 'glbname';
		arr = unpack(template, substr);
		decoded.push(arr['glbname']);

		
		 
		while(boundary = strcspn(trimmed, chr(0)))
		{
			bound_so_far += boundary + 1;
			substr = substr(trimmed, 0, boundary);
			trimmed = substr(trimmed, boundary + 1);
			decoded.push(this.decodeSub(substr));
		}
		
		

		return decoded;			
	};

	/* DECODESUB
	*  input:	a string representing the subscript to be decoded
	*  output:	decoded string
	*  action:	unpack the string into array of 1 byte long unsigned characters,
	*		select decodeStringSub or decodeNumericSub based on first byte of input
	*/

	this.decodeSub = function(string)
	{
		

		char_arr = unpack('C*char', string);
        //distinguish between string and numbers
		if(char_arr[char1] == 0xff)
		{
			
			array_shift(char_arr);
			return this.decodeStringSub(char_arr);
		}
		else		
		{
			return this.decodeNumericSub(char_arr);
		}
	};

	/* DECODESTRINGSUB
	*  input:	an array representing the string subscript
	*  output:	$dec_string, the decoded string, with ASCII translated
	*  action:	decode character by character, concatenate result strings along the way
	*/

	this.decodeStringSub = function(array)
	{
		

		dec_string = '';
		special = false;				//no 0x01 enutil.countered yet

		

		cnt = util.count(array);

	for	(var i = 0; i < cnt ; i++)
		{
			
			char = array_shift(array);
			//0x0101 and 0x0102 are special
			if ((special) && (char == 0x01))
			{
				special = true;
				
			}

			if ((special) && (char == 0x01))	//ASCII NUL
			{
				dec_string +=  chr(0x00);
				special = false;

				
			}
		
			if ((special) && (char == 0x02))	//ASCII SOH					
			{
				dec_string +=  chr(0x01);
				special = false;
				

			}

			dec_string +=  chr(char);
		}

		
		return dec_string;
	};
		
	/* DECODENUMERICSUB
	*  input: 	an array containing the arrays of unsigned bytes of the numeric subscript
	*  output:	$dec_string, the string representation of the number
	*  action:	determine sign, exponent, mantissa in that order
	*/

	this.decodeNumericSub = function(array)
	{ 
		
		cnt = util.count(array);
		
		
		//zero is a special case
		if((cnt == 1) && (array_shift(array) == 0x80))
			return '0';

		//determine whether the number is positive or negative by looking at the last array element
		last = array_pop(array);
		
		if (last == 0xff)
		{
			
			cnt;
			this.m_positive_sub = false;
			this.m_arrayRep = array;
		}
		else
		{
			
			this.m_positive_sub = true;
			array_push(array, last);
			this.m_arrayRep = array;
		}

	

		if (this.m_positive_sub)
		{
			temp_arr = {};
			
			document.write( '\$this->m_arrayRep, reverted:');
			print_r (this.m_arrayRep);
		}

		//clear the sign bit <7>
		first = array_shift(this.m_arrayRep);
		first = first & 0xff;

	
		first = first - 0x80;
	
		//unbias the exponent
		if (first > 0x3e)
			this.m_exp = first - 0x3f;
		else
			this.m_exp = -(0x3f - first);

		
		//subtract one from each byte in mantissa
		(temp_arr);
		
		//assumption: $this->m_arrayRep contains  char data unpacked using unpack('C*',...)
		(this.m_bare_digits);
		string = '';
		cnt = util.count(this.m_arrayRep);		//there should be 2*$cnt number of digits
		
		if(op_code == m.CMMS_Q_ORDER)
			this.m_order = true;
		else
			this.m_order = false;

		if(op_code == m.CMMS_Q_PREV)
			this.m_prev = true;
		else
			this.m_prev = false;
		
		if(op_code == m.CMMS_Q_QUERY)
			this.m_query = true;
		else
			this.m_query = false;

		this.m_encoded = '';
		
		
		if(util.count(this.m_subscripts))
		{
			this.encodeName(true);
			this.end();
			this.m_key_prev = 0x00;
		}
		else
		{
			this.encodeName(false);
			this.encodeSubs(); 		
			this.end();
			
		}

		this.packKey();

	

		return this.m_encoded;
	};

	/* PACK_KEY
	*  input:	none
	*  output:      none
	*  action:	append 1 byte of ASCII null and prepend top, end, prev
	*/

	this.packKey = function()
	{
		this.m_encoded +=  pack('C', chr(0));
		encodedLen = strlen(this.m_encoded);
		this.m_key_end = encodedLen - 2;
		this.m_key_len = encodedLen + 6;	//sizeof(gv_key) - 2
		gv_key = '';
		gv_key +=  pack('S3', this.m_key_top, this.m_key_end, this.m_key_prev);
		this.m_encoded = gv_key + this.m_encoded;
	};

	/* SEPARATE
	*  input: 	none
	*  output:	none
	*  action:	append an ASCII null to $this->m_encoded
	*/

	this.separate = function()
	{
		this.m_encoded +=  pack('C',chr(0));
	};
	
	/* END
	*  input:	none
	*  output:	none
	*  action:	append two ASCII null's to $this->m_encoded
	*/

	this.end = function()
	{
		this.m_encoded +=  pack('C', chr(0));
	};

	/* ENCODENAME
	*  input:	none
	*  output:	none
	*  action:	encode the global name according to its ASCII value
	*/
		
	this.encodeName = function(naked)
	{
		strLength = strlen(this.m_global_name);
		//only the first 8 characters util.count
		if(strLength > m.GNP_GLBNAME_MAX_LENGTH)
			strLength = m.GNP_GLBNAME_MAX_LENGTH;
		for (var i = 0; i < strLength; i++)
		{
			code = util.ord(substr(this.m_global_name, i, 1));
			this.m_encoded +=  pack('C', code);
		}
		if((naked) && (this.m_order))
			this.m_encoded +=  pack('C', 0x01);
		if((naked) && (this.m_query))
			this.m_encoded +=  pack('C2', 0x00, 0x01);
		
		this.separate();
	};	
		
	/* ENCODESUBS
	*  input:	none
	*  output:	none
	*  action:	encode the subscripts one by one, distinguishing between string and numeric ones
	*/

	this.encodeSubs = function()
	{
		subListLength = util.count(this.m_subscripts);
		i = 0;
		while(subListLength>0)
		{
			
			this.m_key_prev = strlen(this.m_encoded);
			//process next
			if(is_numeric(this.m_subscripts[i]))
				this.encodeNumericSub(i);
			else
				this.encodeStringSub(i);
	
			if((this.m_order) && (subListLength == 1))
			{
				this.m_encoded +=  pack('C', 0x01);
			}
			
			if((this.m_query) && (subListLength == 1))
				this.m_encoded +=  pack('C2', 0x00, 0x01);
			//pad with 00
			this.separate();		
		
			subListLength;
			i++;
		}

	
			
	};

	/* ENCODENUMERICSUB
	*  input:	index into the array $this->m_subscripts
	*  output:	none
	*  action:	encode $this->m_subscripts[$index] according to the rules regarding numeric subscripts 
	*		laid out in GNP protocol
	*/	

	this.encodeNumericSub = function(index)
	{
		
		
		if(this.m_subscripts[index] == 0)
		{
		
			this.zero();	
		}
		else
		{
			if(this.m_subscripts[index] > 0)
			{
				
				this.m_positive_sub = true;
			}
			else
			{
				
				this.m_positive_sub = false;
			}
			
			(this.m_arrayRep);

			this.doMantissa(index);
			this.doExp();
			this.doSign();
			this.assemble(); 		//glue sign|exp|mantissa into m_encoded
		}

	};

	/* ZERO
	*  input:	none
	*  output:	none
	*  action:	encode a numeric subscript of value zero
	*/

	this.zero = function()
	{
		if(is_null(this.m_encoded))
			this.m_encoded = '';
		this.m_encoded +=  pack('C', 0x80);
	};

	/* IS_DIGIT
	*  input:	a single character
	*  output:	true if it is a digit [0-9]
	*  action:
	*/

	this.isDigit = function(char)
	{
		return ((util.ord(char) >= util.ord('0')) && (util.ord(char) <= util.ord('9')));
	};

	/* STRIP
	*  input:	data to be stripped
	*  output:	none
	*  action:	stip everything escept digits, hacking off leading zero's
	*		place the result in $this->m_bare_digits
	*/

	this.strip = function(data)
	{
	
		//need to deal with trailing zero's in integer's too
		//because PHP automatically expands scientific notation to normal notation

	var	mnew = str_replace('$+0+$', '', data);

	var	lead_zero = true;
		(this.m_bare_digits);		

	var	raw =  mnew;
	
	for	(var i = 0; i < strlen(raw); i++)
		{
			if(this.isDigit(raw[i]))
			{
				digit =  raw[i];
				if ((digit == 0) && (lead_zero)){
					this.m_bare_digits.push(digit);}else
						if(digit != 0)
				{
					lead_zero = false;
					this.m_bare_digits.push(digit);
				}else
				{};
			};
		};
	};

	/* NORMALIZE
	*  input:	integer or float to be normalized
	*  output:	number of decimal places to move, positive if large number, negative if small number
	*  action:	find the position of '.' and act accordingly
	*
	 * 1	$dotPos === false		==>		integer, return positive number
	 * 2	$dotPos == 0			==>		.xxx, combine with case 4
	 * 3	$dotPos == 1 (a.b) and a != 0	==>		normalized, return 0
	 * 4	$dotPos == 1 (a.b) and a == 0 	==>		find the first non-zero digit, return negative number
	 * 5	$dotPos > 1, combine with case 1, return positive number
	*/

	this.normalize = function(data)
	{
		
		assert(strlen(data));
		data = abs(data);
		dotPos = strpos(data, '.');

		
		if(dotPos === false)
			return (strlen(data) - 1);
		preDot = substr(data, 0, dotPos);
	
		postDot = substr(data, dotPos + 1);

		switch(dotPos)
		{
		case 0: preDot = 0;
		case 1:	 
				if(preDot != 0){					return 0;
				}else
				{
					right = -1;
				for	(var j = 0; j < strlen(postDot); j++)
					{
						digit =  postDot[j];
						if(digit != 0)
							break;
						else
							right;
					}
					return right;
				}
				
				return (dotPos - 1);
		};
	};

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

	this.doMantissa = function(index)
	{
		
		sub = this.m_subscripts[index];		
		scientific = true;
		
		//scientific numbers
		var ePos = strpos(sub, 'e');
		var EPos = strpos(sub, 'E');
		if((ePos === false) && (EPos === false))
			scientific = false;
		else{
			expPos = (ePos === false? EPos :  ePos);
			
			//get original exponent
			this.m_exp = substr(sub, expPos + 1);

			//adjust the exponent if the part prior to $expPos is not normalized
			mant = substr(sub, 0, expPos);
			this.m_exp += this.normalize(mant);
			
		
			this.strip(mant);
			if(util.count(this.m_bare_digits)%2)
				this.m_bare_digits.push(0);	
		};

		//non-scientific numbers
		if(scientific)
		{
			this.m_exp = this.normalize(sub);

			this.strip(sub);
			if(util.count(this.m_bare_digits)%2)
				this.m_bare_digits.push(0);
		}	
		
		/* Create packed-decimal representation, adding one to each byte in mantissa */
		assert.that((util.count(this.m_bare_digits), 2));
		group = util.count(this.m_bare_digits) / 2;

		
		(this.m_arrayRep);
		for (var i = 0; i < group; i++)
		{
			data_hi = array_shift(this.m_bare_digits);
			data_lo = array_shift(this.m_bare_digits);
			data_lo = data_lo + 1;
			data = data_hi + dechex(data_lo);
			
		
			this.m_arrayRep.push(data);
		}
		
	};
	/* DOEXP
	*  input:	none
	*  output:	none
	*  action:	bias the exponent by adding 0x3f, and prepend it to the array representation
	*/

	this.doExp = function()
	{
		
		this.m_exp += 0x3F;

		//prepend it to the rest of encoding
		array_unshift(this.m_arrayRep, this.m_exp);
		
	};

	/* DOSIGN
	*  input:	none
	*  output:	none
	*  action:	set bit<7> of the first byte of the encoding
	*/

	this.doSign = function()
	{
			//set exponent sign bit <7>
		this.m_arrayRep[0] += 0x80;

	};

	/* ASSEMBLE
	*  input:	none
	*  output:	none
	*  action:	glue sign, exponent, mantissa together, place the encoded result in $this->m_encoded
	*/

	this.assemble = function()
	{
		

		temp = '';
		temp +=  pack('C', array_shift(this.m_arrayRep));
		

	for	(var i = 0; i < util.count(this.m_arrayRep); i++)
		{
			
			//need to adjust for things like 6a
			temp_char = this.m_arrayRep[i];
			if((temp_char[1] == 'a') || (temp_char[1] == 'A'))
			{
				
				
				hi = temp_char[0];
				hi_hex = dechex(hi);
				lo = 0x0a;
				this.m_arrayRep[i] = ((hi_hex << 4) | (lo & 0xff));

				temp +=  pack('C', this.m_arrayRep[i]);
				
			};
				
				
			if(this.m_arrayRep[i] >= 10)
				temp +=  pack('H2', this.m_arrayRep[i]);
			else
			{
				temp +=  pack('C', this.m_arrayRep[i]);
			};

		}
		
		{
			document.write( 'Finally, \$temp length = ', strlen(temp));
			document.write( '<br>\n');
		}

		if(this.m_positive_sub)
		{
			temp = ~temp;
			temp +=  pack('C', 0xFF);
		}
			this.m_encoded +=  temp; 
	};

	/* SHOWENCODING
	*  input:	a binary string representing the encoded verion of data
	*  output:	none
	*  action:	display the encoding in a human readable format
	*/

	this.showEncoding = function(binary)
	{
		if(strlen(binary))
		{
			document.write( 'Encoded: ', '<br>\n');
			encoded = unpack('C*code', binary);
			for (var i = 1; i <= util.count(encoded); i++)
			{
				sub = 'code' + i;
				data = dechex(encoded[sub]);
				j = i - 1;
				document.write( 'code [' + j + ']: ' + data);
				document.write( '<br>\n');
			};  
		}else
			document.write( 'Subscript is empty!<br>\n');
	};

	/* SHOWDECODED
	*  input:	an array representing the decoded subscripts
	*  output:	none
	*  action:	display the subscripts in a human readable format
	*/

	this.showDecoded = function(array)
	{
		if(util.count(array))
		{
			document.write( 'Decoded: ', '<br>\n');
			for (var subVal in array) {
                        sub = array[subVal];
				var_dump(sub);
			};
		};
	};	

	/* ENCODESTRINGSUB
	*  input:	index into the arraly $this->m_subscripts
	*  output:	none
	*  action:	encode the string subscript character by character
	*/

	this.encodeStringSub = function(index)
	{

		if((index == (util.count(this.m_subscripts) - 1)) && 
				(this.m_subscripts[index] == '')&& 
				(this.m_order || this.m_query || this.m_prev))
		{
			return;
		}
		//prefix with 0xFF
		this.m_encoded +=  pack('C', 0xFF);

		//now go through each character, paying attention to 0x00 and 0x01
		strLength = strlen(this.m_subscripts[index]);
		
		for (var i = 0; i < strLength; i++)
		{	
			var ord = util.ord(this.m_subscripts[index][i]);
		

			if(ord ==0)
			{
				this.m_encoded +=  pack('C', 0x01);
				this.m_encoded +=  pack('C', 0x01);
				
			}else if(ord ==1)
			{
				this.m_encoded +=  pack('C', 0x01);
				this.m_encoded +=  pack('C', 0x02);
				
			}else
			{
				this.m_encoded +=  pack('C', ord);
			};
		};
	};



function SUBSC() 
{
	 m_reg_no;
	 m_gds_key;

	this.SUBSC = function(reg_no, gds_key)
	{
		this.m_reg_no = reg_no;
		this.m_gds_key = gds_key;
	};

	//this function should be redefined to suit the user collating sequence, if necessary
	this.before = function(subsc)
	{
		return (strcmp(this.m_gds_key.m_global_name, subsc.m_gds_key.m_global_name) < 0);
	};

}

function REGION_INFO() 
{
	 m_region_no;
	 m_nullsubs;
	 m_max_rec_len;
	 m_max_subsc_len;

	this.REGION_INFO = function(reg_no, nullsubs, max_rec_len, max_subsc_len)
	{
		this.m_region_no = reg_no;
		this.m_nullsubs = nullsubs;
		this.m_max_rec_len = max_rec_len;
		this.m_max_subsc_len = max_subsc_len;
	};
}

function PROTOCOL() 
{

	 m_cpu_type;
	 m_os;
	 m_implementation;
	 m_version;
	 m_proto_type;
	 m_proto_level;
	 m_endian;	//'B' for big-endian
	 m_proto_filler;
	//filled with space



	this.PROTOCOL = function()
	{
		
		if(defined(m.S_PROTOCOL))
		{		
			num_args = func_num_args();

			if(num_args)
			{
				this.m_cpu_type = this.encodeCPU(m.GTM_RELEASE_NAME);
				this.m_os = this.encodeOS(m.GTM_RELEASE_NAME);
				this.m_implementation =  m.PROTO_IMPLEMENTATION;
				this.m_version = m.PROTO_VERSION;
				this.m_proto_type =  m.CMM_PROTOCOL_TYPE;
				this.m_proto_level = m.PROTO_LEVEL;
				if(this.isBigEndian(this.m_cpu_type))
					this.m_endian = 'B';
				else
					this.m_endian = ' ';
				this.m_proto_filler = m.S_PROTOCOL_FILLER;
			}
			else	//the argument is a 33 byte string
			{
				arg = func_get_arg(0);
				arg = substr(arg, 0, m.CM_PROTOCOL_LENGTH);
							
				split = chunk_split(arg, 3, '\0');
				arr = explode('\0', split);
				array_pop(arr);
				
				this.m_cpu_type = arr[0];
				this.m_os = arr[1];
				this.m_implementation = arr[2];
				this.m_version = arr[3];
				this.m_proto_type = arr[4];
				this.m_proto_level = arr[5];
				if(this.isBigEndian(this.m_cpu_type))
					this.m_endian = 'B';
				else
					this.m_endian = ' ';
				this.m_proto_filler = arr[7];
			}
		}
		else
		{
			split = chunk_split(m.S_PROTOCOL, 3, '\0');
			arr = explode('\0', split);
			array_pop(arr);
			
			this.m_cpu_type = arr[0];
			this.m_os = arr[1];
			this.m_implementation = arr[2];
			this.m_version = arr[3];
			this.m_proto_type = arr[4];
			this.m_proto_level = arr[5];
			if(this.isBigEndian(this.m_cpu_type))
				this.m_endian = 'B';
			else
				this.m_endian = ' ';
			this.m_proto_filler = arr[7];
		}
		assert(strlen(this.m_cpu_type) == 3);
		assert(strlen(this.m_os) == 3);
		assert(strlen(this.m_implementation) == 3);
		assert(strlen(this.m_version) == 3);
		assert(strlen(this.m_proto_level) == 3);
		assert(strlen(this.m_proto_type) == 3);
		assert(strlen(this.m_endian) == 1);
	
	};

	this.matchEndian = function(peer)
	{
		return true;	// Debug
		if(defined(m.S_PROTOCOL))
		{
			return (this.m_endian == peer.m_endian);
		}
		return true;
	};

	this.match = function(peer)
	{
		return true;	// Debug
		if(defined(m.S_PROTOCOL))
		{
			if(strcmp(this.m_version, peer.m_version))
				return false;
			if(strcmp(this.m_proto_type, peer.m_proto_type))
				return false;
			if(strcmp(this.m_proto_level, peer.m_proto_level))
				return false;
			return true;
		}
		else return true;
	};


	this.encodeCPU = function(rname)
	{
		arr = explode(' ', rname);
		cpu = arr[3];
		
		if(preg_match('/RS6000/i', cpu))
			return 'PPC';
		if(preg_match('/AXP/i', cpu))
			return 'AXP';
		if(preg_match('/HP-PA/i', cpu))
			return 'PAR';
		if(preg_match('86', cpu))
			return 'X86';
		if(preg_match('/S390/i', cpu))
			return '390';
		if(preg_match('/SPARC/i', cpu))
			return 'SPA';
		if(preg_match('/VAX/i', cpu))
			return 'VAX';
		if(preg_match('/ALPHA/i', cpu))
			return 'ALP';
		return  substr(cpu, 0, 3);   
	};

	this.encodeOS = function(rname)
	{
		arr = explode(' ', rname);
		os = arr[2];

		if(preg_match('/AIX/i', os))
			return 'AIX';
		if(preg_match('/OSF1/i', os))
			return 'OSF';
		if(preg_match('/HP-UX/i', os))
			return 'HPX';
		if(preg_match('/Linux/i', os))
			return 'LNX';
		if(preg_match('/OS390/', os))
			return 'zOS';
		if(preg_match('/Solaris/i', os))
			return 'SOL';
		if(preg_match('/VMS/i', os))
			return 'VMS';
		return  substr(os, 0, 3);   
	};		
	this.preg_match = function(pattern, s, matches) {
		var mod = '';
		if ((pattern.charAt(0) == '/') || (pattern.charAt(0) == '@')) {
		var n = pattern.lastIndexOf(pattern.charAt(0));
		if (n != 0) {
		pattern = pattern.substring(1, n);
		mod = pattern.substring(n+1);
		}
		}
		var rx = new RegExp(pattern, mod);
		if (!(matches instanceof Array)) {
		matches = [];
		}
		matches[0] = rx.exec(s);
		return (matches[0] === null)? 0: 1;
		};	
	this.isBigEndian = function(cpu)
	{
		myvar = unpack('S1newval', '10');
		if (myvar['newval'] == 0x3031)
			return false;
		else
		{
			assert(myvar['newval'] == 0x3130);
			return true;
		}
	};
  };
};
