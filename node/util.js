exports.func_num_args = function () {  
        // Get the number of arguments that were passed to the function    
        //   
        // version: 812.1714  
        // discuss at: http://phpjs.org/functions/func_num_args  
        // +   original by: Brett Zamir  
        // %        note 1: May not work in all JS implementations  
        // *     example 1: function tmp_a() {return func_num_args();}  
        // *     example 1: tmp_a('a', 'b');  
        // *     returns 1: 2  
        if (!arguments.callee.caller) {  
            try {  
                throw new Error('Either you are using this in a browser which does not support the "caller" property or you are calling this from a global context');  
                return false;  
            } catch(e){  
                return false;  
            }  
        }  
      
        return arguments.callee.caller.arguments.length;  
    };  
exports.in_array = function(needle, haystack) {
	if (haystack instanceof Array) {
	for (var i=0; i < haystack.length; ++i) {
	if (haystack[i] == needle) {
	return true;
	}
	}
	} else {
	for (var p in haystack) {
	if (haystack[p] === needle) {
	return true;
	}
	}
	}
	return false;
	};
	
exports.count = function(a) {
		var c = 0;
		if (a instanceof Array) {
		for (var i=0; i < a.length; ++i) {
		if ((typeof a[i]) != 'undefined') {
		++c;
		}
		}
		} else {
		c = Object.keys(a).length;
		}
		return c;
		};
exports.str_replace = function(search, replace, subject) {
			var ret = '';
			var p = 0;
			var n = subject.indexOf(search, p);
			while (n != -1) {
			ret += subject.substring(p, n) + replace;
			p = n + search.length;
			n = subject.indexOf(search, p);
			}
			return ret + subject.substring(p);
			};
exports.get_class = function(obj) {
					  // http://kevin.vanzonneveld.net
					  // +   original by: Ates Goral (http://magnetiq.com)
					  // +   improved by: David James
					  // +   improved by: David Neilsen
					  // *     example 1: util.get_class(new (function MyClass() {}));
					  // *     returns 1: "MyClass"
					  // *     example 2: util.get_class({});
					  // *     returns 2: "Object"
					  // *     example 3: util.get_class([]);
					  // *     returns 3: false
					  // *     example 4: util.get_class(42);
					  // *     returns 4: false
					  // *     example 5: util.get_class(window);
					  // *     returns 5: false
					  // *     example 6: util.get_class(function MyFunction() {});
					  // *     returns 6: false
					  if (obj && typeof obj === 'object' &&
					      Object.prototype.toString.call(obj) !== '[object Array]' &&
					      obj.constructor && obj !== this.window) {
					    var arr = obj.constructor.toString().match(/function\s*(\w+)/);

					    if (arr && arr.length === 2) {
					      return arr[1];
					    }
					  }

					  return false;
					};
exports.ord = function(string) {
						  // http://kevin.vanzonneveld.net
						  // +   original by: Kevin van Zonneveld (http://kevin.vanzonneveld.net)
						  // +   bugfixed by: Onno Marsman
						  // +   improved by: Brett Zamir (http://brett-zamir.me)
						  // +   input by: incidence
						  // *     example 1: ord('K');
						  // *     returns 1: 75
						  // *     example 2: ord('\uD800\uDC00'); // surrogate pair to create a single Unicode character
						  // *     returns 2: 65536
						  var str = string + '',
						    code = str.charCodeAt(0);
						  if (0xD800 <= code && code <= 0xDBFF) { // High surrogate (could change last hex to 0xDB7F to treat high private surrogates as single characters)
						    var hi = code;
						    if (str.length === 1) {
						      return code; // This is just a high surrogate with no following low surrogate, so we return its value;
						      // we could also throw an error as it is not a complete character, but someone may want to know
						    }
						    var low = str.charCodeAt(1);
						    return ((hi - 0xD800) * 0x400) + (low - 0xDC00) + 0x10000;
						  }
						  if (0xDC00 <= code && code <= 0xDFFF) { // Low surrogate
						    return code; // This is just a low surrogate with no preceding high surrogate, so we return its value;
						    // we could also throw an error as it is not a complete character, but someone may want to know
						  }
						  return code;
						};
exports.before_exit = function()
						{
							for (var key in GLOBALS) {
						                        val = GLOBALS[key];
								if (util.get_class(GLOBALS[key]) == 'gtcm_gnp')
									val.destroy(); 
							}
						};

exports.array_keys = function(input, search_value, argStrict) {
	  // http://kevin.vanzonneveld.net
	  // +   original by: Kevin van Zonneveld (http://kevin.vanzonneveld.net)
	  // +      input by: Brett Zamir (http://brett-zamir.me)
	  // +   bugfixed by: Kevin van Zonneveld (http://kevin.vanzonneveld.net)
	  // +   improved by: jd
	  // +   improved by: Brett Zamir (http://brett-zamir.me)
	  // +   input by: P
	  // +   bugfixed by: Brett Zamir (http://brett-zamir.me)
	  // *     example 1: array_keys( {firstname: 'Kevin', surname: 'van Zonneveld'} );
	  // *     returns 1: {0: 'firstname', 1: 'surname'}

	  var search = typeof search_value !== 'undefined',
	    tmp_arr = [],
	    strict = !!argStrict,
	    include = true,
	    key = '';

	  if (input && typeof input === 'object' && input.change_key_case) { // Duck-type check for our own array()-created PHPJS_Array
	    return input.keys(search_value, argStrict);
	  }

	  for (key in input) {
	    if (input.hasOwnProperty(key)) {
	      include = true;
	      if (search) {
	        if (strict && input[key] !== search_value) {
	          include = false;
	        }
	        else if (input[key] != search_value) {
	          include = false;
	        }
	      }

	      if (include) {
	        tmp_arr[tmp_arr.length] = key;
	      }
	    }
	  }

	  return tmp_arr;
	};

 exports.is_array = function(input){
        		    return typeof(input)=='object'&&(input instanceof Array);
        	  };

 exports.pack = function(format) {
        		  // http://kevin.vanzonneveld.net
        		  // +   original by: Tim de Koning (http://www.kingsquare.nl)
        		  // +      parts by: Jonas Raoni Soares Silva (http://www.jsfromhell.com)
        		  // +   bugfixed by: Tim de Koning (http://www.kingsquare.nl)
        		  // %        note 1: Float encoding by: Jonas Raoni Soares Silva
        		  // %        note 2: Home: http://www.kingsquare.nl/blog/12-12-2009/13507444
        		  // %        note 3: Feedback: phpjs-pack@kingsquare.nl
        		  // %        note 4: 'machine dependent byte order and size' aren't
        		  // %        note 4: applicable for JavaScript; pack works as on a 32bit,
        		  // %        note 4: little endian machine
        		  // *     example 1: pack('nvc*', 0x1234, 0x5678, 65, 66);
        		  // *     returns 1: '4xVAB'
        		  var formatPointer = 0,
        		    argumentPointer = 1,
        		    result = '',
        		    argument = '',
        		    i = 0,
        		    r = [],
        		    instruction, quantifier, word, precisionBits, exponentBits, extraNullCount;

        		  // vars used by float encoding
        		  var bias, minExp, maxExp, minUnnormExp, status, exp, len, bin, signal, n, intPart, floatPart, lastBit, rounded, j, k, tmpResult;

        		  while (formatPointer < format.length) {
        		    instruction = format.charAt(formatPointer);
        		    quantifier = '';
        		    formatPointer++;
        		    while ((formatPointer < format.length) && (format.charAt(formatPointer).match(/[\d\*]/) !== null)) {
        		      quantifier += format.charAt(formatPointer);
        		      formatPointer++;
        		    }
        		    if (quantifier === '') {
        		      quantifier = '1';
        		    }

        		    // Now pack variables: 'quantifier' times 'instruction'
        		    switch (instruction) {
        		    case 'a':
        		      // NUL-padded string
        		    case 'A':
        		      // SPACE-padded string
        		      if (typeof arguments[argumentPointer] === 'undefined') {
        		        throw new Error('Warning:  pack() Type ' + instruction + ': not enough arguments');
        		      } else {
        		        argument = String(arguments[argumentPointer]);
        		      }
        		      if (quantifier === '*') {
        		        quantifier = argument.length;
        		      }
        		      for (i = 0; i < quantifier; i++) {
        		        if (typeof argument[i] === 'undefined') {
        		          if (instruction === 'a') {
        		            result += String.fromCharCode(0);
        		          } else {
        		            result += ' ';
        		          }
        		        } else {
        		          result += argument[i];
        		        }
        		      }
        		      argumentPointer++;
        		      break;
        		    case 'h':
        		      // Hex string, low nibble first
        		    case 'H':
        		      // Hex string, high nibble first
        		      if (typeof arguments[argumentPointer] === 'undefined') {
        		        throw new Error('Warning: pack() Type ' + instruction + ': not enough arguments');
        		      } else {
        		        argument = arguments[argumentPointer];
        		      }
        		      if (quantifier === '*') {
        		        quantifier = argument.length;
        		      }
        		      if (quantifier > argument.length) {
        		        throw new Error('Warning: pack() Type ' + instruction + ': not enough characters in string');
        		      }
        		      for (i = 0; i < quantifier; i += 2) {
        		        // Always get per 2 bytes...
        		        word = argument[i];
        		        if (((i + 1) >= quantifier) || typeof(argument[i + 1]) === 'undefined') {
        		          word += '0';
        		        } else {
        		          word += argument[i + 1];
        		        }
        		        // The fastest way to reverse?
        		        if (instruction === 'h') {
        		          word = word[1] + word[0];
        		        }
        		        result += String.fromCharCode(parseInt(word, 16));
        		      }
        		      argumentPointer++;
        		      break;

        		    case 'c':
        		      // signed char
        		    case 'C':
        		      // unsigned char
        		      // c and C is the same in pack
        		      if (quantifier === '*') {
        		        quantifier = arguments.length - argumentPointer;
        		      }
        		      if (quantifier > (arguments.length - argumentPointer)) {
        		        throw new Error('Warning:  pack() Type ' + instruction + ': too few arguments');
        		      }

        		      for (i = 0; i < quantifier; i++) {
        		        result += String.fromCharCode(arguments[argumentPointer]);
        		        argumentPointer++;
        		      }
        		      break;

        		    case 's':
        		      // signed short (always 16 bit, machine byte order)
        		    case 'S':
        		      // unsigned short (always 16 bit, machine byte order)
        		    case 'v':
        		      // s and S is the same in pack
        		      if (quantifier === '*') {
        		        quantifier = arguments.length - argumentPointer;
        		      }
        		      if (quantifier > (arguments.length - argumentPointer)) {
        		        throw new Error('Warning:  pack() Type ' + instruction + ': too few arguments');
        		      }

        		      for (i = 0; i < quantifier; i++) {
        		        result += String.fromCharCode(arguments[argumentPointer] & 0xFF);
        		        result += String.fromCharCode(arguments[argumentPointer] >> 8 & 0xFF);
        		        argumentPointer++;
        		      }
        		      break;

        		    case 'n':
        		      // unsigned short (always 16 bit, big endian byte order)
        		      if (quantifier === '*') {
        		        quantifier = arguments.length - argumentPointer;
        		      }
        		      if (quantifier > (arguments.length - argumentPointer)) {
        		        throw new Error('Warning:  pack() Type ' + instruction + ': too few arguments');
        		      }

        		      for (i = 0; i < quantifier; i++) {
        		        result += String.fromCharCode(arguments[argumentPointer] >> 8 & 0xFF);
        		        result += String.fromCharCode(arguments[argumentPointer] & 0xFF);
        		        argumentPointer++;
        		      }
        		      break;

        		    case 'i':
        		      // signed integer (machine dependent size and byte order)
        		    case 'I':
        		      // unsigned integer (machine dependent size and byte order)
        		    case 'l':
        		      // signed long (always 32 bit, machine byte order)
        		    case 'L':
        		      // unsigned long (always 32 bit, machine byte order)
        		    case 'V':
        		      // unsigned long (always 32 bit, little endian byte order)
        		      if (quantifier === '*') {
        		        quantifier = arguments.length - argumentPointer;
        		      }
        		      if (quantifier > (arguments.length - argumentPointer)) {
        		        throw new Error('Warning:  pack() Type ' + instruction + ': too few arguments');
        		      }

        		      for (i = 0; i < quantifier; i++) {
        		        result += String.fromCharCode(arguments[argumentPointer] & 0xFF);
        		        result += String.fromCharCode(arguments[argumentPointer] >> 8 & 0xFF);
        		        result += String.fromCharCode(arguments[argumentPointer] >> 16 & 0xFF);
        		        result += String.fromCharCode(arguments[argumentPointer] >> 24 & 0xFF);
        		        argumentPointer++;
        		      }

        		      break;
        		    case 'N':
        		      // unsigned long (always 32 bit, big endian byte order)
        		      if (quantifier === '*') {
        		        quantifier = arguments.length - argumentPointer;
        		      }
        		      if (quantifier > (arguments.length - argumentPointer)) {
        		        throw new Error('Warning:  pack() Type ' + instruction + ': too few arguments');
        		      }

        		      for (i = 0; i < quantifier; i++) {
        		        result += String.fromCharCode(arguments[argumentPointer] >> 24 & 0xFF);
        		        result += String.fromCharCode(arguments[argumentPointer] >> 16 & 0xFF);
        		        result += String.fromCharCode(arguments[argumentPointer] >> 8 & 0xFF);
        		        result += String.fromCharCode(arguments[argumentPointer] & 0xFF);
        		        argumentPointer++;
        		      }
        		      break;

        		    case 'f':
        		      // float (machine dependent size and representation)
        		    case 'd':
        		      // double (machine dependent size and representation)
        		      // version based on IEEE754
        		      precisionBits = 23;
        		      exponentBits = 8;
        		      if (instruction === 'd') {
        		        precisionBits = 52;
        		        exponentBits = 11;
        		      }

        		      if (quantifier === '*') {
        		        quantifier = arguments.length - argumentPointer;
        		      }
        		      if (quantifier > (arguments.length - argumentPointer)) {
        		        throw new Error('Warning:  pack() Type ' + instruction + ': too few arguments');
        		      }
        		      for (i = 0; i < quantifier; i++) {
        		        argument = arguments[argumentPointer];
        		        bias = Math.pow(2, exponentBits - 1) - 1;
        		        minExp = -bias + 1;
        		        maxExp = bias;
        		        minUnnormExp = minExp - precisionBits;
        		        status = isNaN(n = parseFloat(argument)) || n === -Infinity || n === +Infinity ? n : 0;
        		        exp = 0;
        		        len = 2 * bias + 1 + precisionBits + 3;
        		        bin = new Array(len);
        		        signal = (n = status !== 0 ? 0 : n) < 0;
        		        n = Math.abs(n);
        		        intPart = Math.floor(n);
        		        floatPart = n - intPart;

        		        for (k = len; k;) {
        		          bin[--k] = 0;
        		        }
        		        for (k = bias + 2; intPart && k;) {
        		          bin[--k] = intPart % 2;
        		          intPart = Math.floor(intPart / 2);
        		        }
        		        for (k = bias + 1; floatPart > 0 && k; --floatPart) {
        		          (bin[++k] = ((floatPart *= 2) >= 1) - 0);
        		        }
        		        for (k = -1; ++k < len && !bin[k];) {}

        		        if (bin[(lastBit = precisionBits - 1 + (k = (exp = bias + 1 - k) >= minExp && exp <= maxExp ? k + 1 : bias + 1 - (exp = minExp - 1))) + 1]) {
        		          if (!(rounded = bin[lastBit])) {
        		            for (j = lastBit + 2; !rounded && j < len; rounded = bin[j++]) {}
        		          }
        		          for (j = lastBit + 1; rounded && --j >= 0;
        		          (bin[j] = !bin[j] - 0) && (rounded = 0)) {}
        		        }

        		        for (k = k - 2 < 0 ? -1 : k - 3; ++k < len && !bin[k];) {}

        		        if ((exp = bias + 1 - k) >= minExp && exp <= maxExp) {
        		          ++k;
        		        } else {
        		          if (exp < minExp) {
        		            if (exp !== bias + 1 - len && exp < minUnnormExp) { /*"encodeFloat::float underflow" */
        		            }
        		            k = bias + 1 - (exp = minExp - 1);
        		          }
        		        }

        		        if (intPart || status !== 0) {
        		          exp = maxExp + 1;
        		          k = bias + 2;
        		          if (status === -Infinity) {
        		            signal = 1;
        		          } else if (isNaN(status)) {
        		            bin[k] = 1;
        		          }
        		        }

        		        n = Math.abs(exp + bias);
        		        tmpResult = '';

        		        for (j = exponentBits + 1; --j;) {
        		          tmpResult = (n % 2) + tmpResult;
        		          n = n >>= 1;
        		        }

        		        n = 0;
        		        j = 0;
        		        k = (tmpResult = (signal ? '1' : '0') + tmpResult + bin.slice(k, k + precisionBits).join('')).length;
        		        r = [];

        		        for (; k;) {
        		          n += (1 << j) * tmpResult.charAt(--k);
        		          if (j === 7) {
        		            r[r.length] = String.fromCharCode(n);
        		            n = 0;
        		          }
        		          j = (j + 1) % 8;
        		        }

        		        r[r.length] = n ? String.fromCharCode(n) : '';
        		        result += r.join('');
        		        argumentPointer++;
        		      }
        		      break;

        		    case 'x':
        		      // NUL byte
        		      if (quantifier === '*') {
        		        throw new Error('Warning: pack(): Type x: \'*\' ignored');
        		      }
        		      for (i = 0; i < quantifier; i++) {
        		        result += String.fromCharCode(0);
        		      }
        		      break;

        		    case 'X':
        		      // Back up one byte
        		      if (quantifier === '*') {
        		        throw new Error('Warning: pack(): Type X: \'*\' ignored');
        		      }
        		      for (i = 0; i < quantifier; i++) {
        		        if (result.length === 0) {
        		          throw new Error('Warning: pack(): Type X:' + ' outside of string');
        		        } else {
        		          result = result.substring(0, result.length - 1);
        		        }
        		      }
        		      break;

        		    case '@':
        		      // NUL-fill to absolute position
        		      if (quantifier === '*') {
        		        throw new Error('Warning: pack(): Type X: \'*\' ignored');
        		      }
        		      if (quantifier > result.length) {
        		        extraNullCount = quantifier - result.length;
        		        for (i = 0; i < extraNullCount; i++) {
        		          result += String.fromCharCode(0);
        		        }
        		      }
        		      if (quantifier < result.length) {
        		        result = result.substring(0, quantifier);
        		      }
        		      break;

        		    default:
        		      throw new Error('Warning:  pack() Type ' + instruction + ': unknown format code');
        		    }
        		  }
        		  if (argumentPointer < arguments.length) {
        		    throw new Error('Warning: pack(): ' + (arguments.length - argumentPointer) + ' arguments unused');
        		  }

        		  return result;
        		};
        		
       exports.preg_match = function(pattern, s, matches) {
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
      exports.explode = function(delimiter, s) {
        				return s.split(delimiter);
        				};
        				
      exports.array_reverse = function(array, preserve_keys) {
        					  // http://kevin.vanzonneveld.net
        					  // +   original by: Kevin van Zonneveld (http://kevin.vanzonneveld.net)
        					  // +   improved by: Karol Kowalski
        					  // *     example 1: array_reverse( [ 'php', '4.0', ['green', 'red'] ], true);
        					  // *     returns 1: { 2: ['green', 'red'], 1: 4, 0: 'php'}
        					  var isArray = Object.prototype.toString.call(array) === "[object Array]",
        					    tmp_arr = preserve_keys ? {} : [],
        					    key;

        					  if (isArray && !preserve_keys) {
        					    return array.slice(0).reverse();
        					  }

        					  if (preserve_keys) {
        					    var keys = [];
        					    for(key in array) {
        					      // if (array.hasOwnProperty(key)) {
        					      keys.push(key);
        					      // }
        					    }

        					    var i = keys.length;
        					    while (i--) {
        					      key = keys[i];
        					      // FIXME: don't rely on browsers keeping keys in insertion order
        					      // it's implementation specific
        					      // eg. the result will differ from expected in Google Chrome
        					      tmp_arr[key] = array[key];
        					    }
        					  } else {
        					    for(key in array) {
        					      // if (array.hasOwnProperty(key)) {
        					      tmp_arr.unshift(array[key]);
        					      // }
        					    }
        					  }

        					  return tmp_arr;
        					};
exports.unpack = function(format, data) {
        					    // http://kevin.vanzonneveld.net
        					    // + original by: Tim de Koning (http://www.kingsquare.nl)
        					    // + parts by: Jonas Raoni Soares Silva
        					    // + http://www.jsfromhell.com
        					    // + bugfixed by: marcuswestin
        					    // % note 1: Float decoding by: Jonas Raoni Soares Silva
        					    // % note 2: Home: http://www.kingsquare.nl/blog/22-12-2009/13650536
        					    // % note 3: Feedback: phpjs-unpack@kingsquare.nl
        					    // % note 4: 'machine dependant byte order and size' aren't
        					    // % note 5: applicable for JavaScript unpack works as on a 32bit,
        					    // % note 6: little endian machine
        					    // * example 1: unpack('f2test', 'abcddbca');
        					    // * returns 1: { 'test1': 1.6777999408082E+22.
        					    // * returns 2: 'test2': 2.6100787562286E+20 }

        					    var formatPointer = 0, dataPointer = 0, result = {}, instruction = '',
        					            quantifier = '', label = '', currentData = '', i = 0, j = 0,
        					            word = '', precisionBits = 0, exponentBits = 0, dataByteLength = 0;

        					    // Used by float decoding
        					    var b = [], bias, signal, exponent, significand, divisor, curByte,
        					            byteValue, startBit = 0, mask, currentResult;

        					    var readBits = function(start, length, byteArray){
        					        var offsetLeft, offsetRight, curByte, lastByte, diff, sum;

        					        function shl(a, b){
        					            for(++b; --b;) {
        					                a = ((a %= 0x7fffffff + 1) & 0x40000000) === 0x40000000 ?
        					                    a * 2 :
        					                    (a - 0x40000000) * 2 + 0x7fffffff + 1;
        					            }
        					            return a;
        					        }
        					        if(start < 0 || length <= 0) {
        					            return 0;
        					        }

        					        offsetRight = start % 8;
        					        curByte = byteArray.length - (start >> 3) - 1;
        					        lastByte = byteArray.length + (-(start + length) >> 3);
        					        diff = curByte - lastByte;
        					        sum = (
        					                (byteArray[ curByte ] >> offsetRight) &
        					                ((1 << (diff ? 8 - offsetRight : length)) - 1)
        					            ) + (
        					               diff && (offsetLeft = (start + length) % 8) ?
        					                (byteArray[ lastByte++ ] & ((1 << offsetLeft) - 1)) <<
        					                (diff-- << 3) - offsetRight :
        					                0
        					            );

        					        for(; diff;) {
        					            sum += shl(byteArray[ lastByte++ ], (diff-- << 3) - offsetRight);
        					        }
        					        return sum;
        					    };

        					    while (formatPointer < format.length) {
        					        instruction = format.charAt(formatPointer);

        					        // Start reading 'quantifier'
        					        quantifier = '';
        					        formatPointer++;
        					        while ((formatPointer < format.length) &&
        					              (format.charAt(formatPointer).match(/[\d\*]/) !== null)) {
        					            quantifier += format.charAt(formatPointer);
        					            formatPointer++;
        					        }
        					        if (quantifier === '') {
        					            quantifier = '1';
        					        }


        					        // Start reading label
        					        label = '';
        					        while ((formatPointer < format.length) &&
        					              (format.charAt(formatPointer) !== '/')) {
        					            label += format.charAt(formatPointer);
        					            formatPointer++;
        					        }
        					        if (format.charAt(formatPointer) === '/') {
        					            formatPointer++;
        					        }

        					        // Process given instruction
        					        switch (instruction) {
        					            case 'a': // NUL-padded string
        					            case 'A': // SPACE-padded string
        					                if (quantifier === '*') {
        					                    quantifier = data.length - dataPointer;
        					                } else {
        					                    quantifier = parseInt(quantifier, 10);
        					                }
        					                currentData = data.substr(dataPointer, quantifier);
        					                dataPointer += quantifier;

        					                if (instruction === 'a') {
        					                    currentResult = currentData.replace(/\0+$/, '');
        					                } else {
        					                    currentResult = currentData.replace(/ +$/, '');
        					                }
        					                result[label] = currentResult;
        					                break;

        					            case 'h': // Hex string, low nibble first
        					            case 'H': // Hex string, high nibble first
        					                if (quantifier === '*') {
        					                    quantifier = data.length - dataPointer;
        					                } else {
        					                    quantifier = parseInt(quantifier, 10);
        					                }
        					                currentData = data.substr(dataPointer, quantifier);
        					                dataPointer += quantifier;

        					                if (quantifier>currentData.length) {
        					                    throw new Error('Warning: unpack(): Type ' + instruction +
        					                            ': not enough input, need ' + quantifier);
        					                }

        					                currentResult = '';
        					                for(i=0;i<currentData.length;i++) {
        					                    word = currentData.charCodeAt(i).toString(16);
        					                    if (instruction === 'h') {
        					                        word = word[1]+word[0];
        					                    }
        					                   currentResult += word;
        					                }
        					                result[label] = currentResult;
        					                break;

        					            case 'c': // signed char
        					            case 'C': // unsigned c
        					                if (quantifier === '*') {
        					                    quantifier = data.length - dataPointer;
        					                } else {
        					                    quantifier = parseInt(quantifier, 10);
        					                }

        					                currentData = data.substr(dataPointer, quantifier);
        					                dataPointer += quantifier;

        					                for (i=0;i<currentData.length;i++) {
        					                     currentResult = currentData.charCodeAt(i);
        					                     if ((instruction === 'c') && (currentResult >= 128)) {
        					                        currentResult -= 256;
        					                     }
        					                     result[label+(quantifier>1?
        					                            (i+1):
        					                            '')] = currentResult;
        					                }
        					                break;

        					            case 'S': // unsigned short (always 16 bit, machine byte order)
        					            case 's': // signed short (always 16 bit, machine byte order)
        					            case 'v': // unsigned short (always 16 bit, little endian byte order)
        					                if (quantifier === '*') {
        					                    quantifier = (data.length - dataPointer) / 2;
        					                } else {
        					                    quantifier = parseInt(quantifier, 10);
        					                }

        					                currentData = data.substr(dataPointer, quantifier * 2);
        					                dataPointer += quantifier * 2;

        					                for (i=0;i<currentData.length;i+=2) {
        					                     // sum per word;
        					                    currentResult = ((currentData.charCodeAt(i+1) & 0xFF) << 8) +
        					                            (currentData.charCodeAt(i) & 0xFF);
        					                    if ((instruction === 's') && (currentResult >= 32768)) {
        					                        currentResult -= 65536;
        					                    }
        					                    result[label+(quantifier>1?
        					                            ((i/2)+1):
        					                            '')] = currentResult;
        					                }
        					                break;

        					            case 'n': // unsigned short (always 16 bit, big endian byte order)
        					                if (quantifier === '*') {
        					                    quantifier = (data.length - dataPointer) / 2;
        					                } else {
        					                    quantifier = parseInt(quantifier, 10);
        					                }

        					                currentData = data.substr(dataPointer, quantifier * 2);
        					                dataPointer += quantifier * 2;

        					                for (i=0;i<currentData.length;i+=2) {
        					                     // sum per word;
        					                    currentResult = ((currentData.charCodeAt(i) & 0xFF) << 8) +
        					                            (currentData.charCodeAt(i+1) & 0xFF);
        					                    result[label+(quantifier>1?
        					                            ((i/2)+1):
        					                            '')] = currentResult;
        					                }
        					                break;

        					            case 'i': // signed integer (machine dependent size and byte order)
        					            case 'I': // unsigned integer (machine dependent size & byte order)
        					            case 'l': // signed long (always 32 bit, machine byte order)
        					            case 'L': // unsigned long (always 32 bit, machine byte order)
        					            case 'V': // unsigned long (always 32 bit, little endian byte order)
        					                if (quantifier === '*') {
        					                    quantifier = (data.length - dataPointer) / 4;
        					                } else {
        					                    quantifier = parseInt(quantifier, 10);
        					                }

        					                currentData = data.substr(dataPointer, quantifier * 4);
        					                dataPointer += quantifier * 4;

        					                for (i=0;i<currentData.length;i+=4) {
        					                    currentResult =
        					                            ((currentData.charCodeAt(i+3) & 0xFF) << 24) +
        					                            ((currentData.charCodeAt(i+2) & 0xFF) << 16) +
        					                            ((currentData.charCodeAt(i+1) & 0xFF) << 8) +
        					                            ((currentData.charCodeAt(i) & 0xFF));
        					                    result[label+(quantifier>1?
        					                            ((i/4)+1):
        					                            '')] = currentResult;
        					                }

        					                break;

        					            case 'N': // unsigned long (always 32 bit, little endian byte order)
        					               if (quantifier === '*') {
        					                    quantifier = (data.length - dataPointer) / 4;
        					                } else {
        					                    quantifier = parseInt(quantifier, 10);
        					                }

        					                currentData = data.substr(dataPointer, quantifier * 4);
        					                dataPointer += quantifier * 4;

        					                for (i=0;i<currentData.length;i+=4) {
        					                    currentResult =
        					                            ((currentData.charCodeAt(i) & 0xFF) << 24) +
        					                            ((currentData.charCodeAt(i+1) & 0xFF) << 16) +
        					                            ((currentData.charCodeAt(i+2) & 0xFF) << 8) +
        					                            ((currentData.charCodeAt(i+3) & 0xFF));
        					                    result[label+(quantifier>1?
        					                            ((i/4)+1):
        					                            '')] = currentResult;
        					                }

        					                break;

        					            case 'f':
        					            case 'd':
        					                exponentBits = 8;
        					                dataByteLength = 4;
        					                if (instruction === 'd') {
        					                    exponentBits = 11;
        					                    dataByteLength = 8;
        					                }

        					               if (quantifier === '*') {
        					                    quantifier = (data.length - dataPointer) / dataByteLength;
        					                } else {
        					                    quantifier = parseInt(quantifier, 10);
        					                }

        					                currentData = data.substr(dataPointer,
        					                        quantifier * dataByteLength);
        					                dataPointer += quantifier * dataByteLength;

        					                for (i=0;i<currentData.length;i+=dataByteLength) {
        					                    data = currentData.substr(i, dataByteLength);

        					                    b = [];
        					                    for(j = data.length-1; j >= 0 ; --j) {
        					                        b.push(data.charCodeAt(j));
        					                    }

        					                    precisionBits = (instruction === 'f')?23:52;

        					                    bias = Math.pow(2, exponentBits - 1) - 1;
        					                    signal = readBits(precisionBits + exponentBits, 1, b);
        					                    exponent = readBits(precisionBits, exponentBits, b);
        					                    significand = 0;
        					                    divisor = 2;
        					                    curByte = b.length + (-precisionBits >> 3) - 1;
        					                    startBit = 0;

        					                    do {
        					                        byteValue = b[ ++curByte ];
        					                        startBit = precisionBits % 8 || 8;
        					                        mask = 1 << startBit;
        					                        for(; (mask >>= 1);) {
        					                            if (byteValue & mask) {
        					                                significand += 1 / divisor;
        					                            }
        					                            divisor *= 2;
        					                        }
        					                    } while ((precisionBits -= startBit));

        					                        if (exponent === (bias << 1) + 1) {
        					                            if (significand) {
        					                                currentResult = NaN;
        					                            } else {
        					                                if (signal) {
        					                                    currentResult = -Infinity;
        					                                } else {
        					                                    currentResult = +Infinity;
        					                                }
        					                            }
        					                        } else {
        					                            if ((1 + signal * -2) * (exponent || significand)) {
        					                                if (!exponent) {
        					                                    currentResult = Math.pow(2, -bias + 1) *
        					                                            significand;
        					                                } else {
        					                                    currentResult = Math.pow(2,
        					                                            exponent - bias) *
        					                                            (1 + significand);
        					                                }
        					                            } else {
        					                                currentResult = 0;
        					                            }
        					                        }
        					                        result[label+(quantifier>1?
        					                                ((i/4)+1):
        					                                '')] = currentResult;
        					                }

        					                break;

        					            case 'x': // NUL byte
        					            case 'X': // Back up one byte
        					            case '@': // NUL byte
        					                 if (quantifier === '*') {
        					                    quantifier = data.length - dataPointer;
        					                } else {
        					                    quantifier = parseInt(quantifier, 10);
        					                }

        					                if (quantifier > 0) {
        					                    if (instruction === 'X') {
        					                        dataPointer -= quantifier;
        					                    } else {
        					                        if (instruction === 'x') {
        					                            dataPointer += quantifier;
        					                        } else {
        					                            dataPointer = quantifier;
        					                        }
        					                    }
        					                }
        					                break;

        					            default:
        					            throw new Error('Warning: unpack() Type ' + instruction +
        					                    ': unknown format code');
        					        }
        					    }
        					    return result;
        					};
        					
        				