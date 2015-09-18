/******
Copyright (C) 2014 Justin Adams

    This file is part of Unicoder.

    Unicoder is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License
    (version 2.1 only) as published by the Free Software Foundation.

    Unicoder is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Unicoder.  If not, see <http://www.gnu.org/licenses/>.
****/


#ifndef  UNICODER_C
#define  UNICODER_C  1


#include <stdio.h>
#include <string.h>

#include "unicoder.h"






/* tests for little endian straight or big endian straight, returns UNICODER_ENDIANNESS_UNRECOGNIZED otherwise */
int unicoder_getMachineEndianness()
{
	unsigned int x= 0x01234567;
	char *b0, *b1, *b2, *b3;

	b0= (char*) (&x);
	b1= b0 + 1;
	b2= b0 + 2;
	b3= b0 + 3;

	if((*b0 == 0x67) && (*b1 == 0x45) && (*b2 == 0x23) && (*b3 == 0x01))
		return UNICODER_LES;

	if((*b0 == 0x01) && (*b1 == 0x23) && (*b2 == 0x45) && (*b3 == 0x67))
		return UNICODER_BES;

	else return UNICODER_ENDIANNESS_UNRECOGNIZED;
}


int unicoder_reverseEndianness(unsigned char* src, unsigned char* dest, unsigned int length)
{
	unsigned int i;

	if((src == NULL) || (dest == NULL))
		return UNICODER_NULL_POINTER;

	if(length < 2)
		return UNICODER_BAD_LENGTH;

	for(i= 0; i < length; i++)
		*(dest + i)= *(src + length - i - 1);

	return 1;
}


/* uses byte order mark to determine encoding type */
int unicoder_decodeBom(unsigned char* p)
{
	int i;
	unsigned int len;
	unsigned char newp[4];

	if(p == NULL)
		return UNICODER_NULL_POINTER;

	for(i= 0; i < 4; i++)
		newp[i]= 0;

	/* this is weird, but it's the best way I can think of to do it portably */
	/* ATTN, in the future this could be a portability concern */
	strncpy(newp, p, 4);
	strncpy(newp + 1, p + 1, 3);
	strncpy(newp + 2, p + 2, 2);
	strncpy(newp + 3, p + 3, 1);

	/* this is for debugging
	for(i= 0; i < 4; i++)
		printf("<%d>\n", newp[i]);
	printf("\n\n");
	*/

	/* 00 00 FE FF*/
	if((newp[0] == 0x00) && (newp[1] == 0x00) && (newp[2] == 0xfe) && (newp[3] == 0xff))
		return UNICODER_UTF32BE;

	/* FF FE 00 00*/
	if((newp[0] == 0xff) && (newp[1] == 0xfe) && (newp[2] == 0x00) && (newp[3] == 0x00))
		return UNICODER_UTF32LE;

	/* EF BB  BF */
	if((newp[0] == 0xef) && (newp[1] == 0xbb) && (newp[2] == 0xbf))
		return UNICODER_UTF8;

	/* FE FF */
	if((newp[0] == 0xfe) && (newp[1] == 0xff))
		return UNICODER_UTF16BE;

	/* FF FE*/
	if((newp[0] == 0xff) && (newp[1] == 0xfe))
		return UNICODER_UTF16LE;

	/* if all else failes, assume ASCII */
	return UNICODER_ASCII;
}


/* uses byte order mark to determine encoding type */
int unicoder_decodeBomFromFile(FILE* f)
{
	int i, currChar;
	long originalPosition;
	unsigned char bytes[4];

	if(f == NULL)
		return UNICODER_FILE_IO_ERROR;

	originalPosition= ftell(f);
	fseek(f, 0, SEEK_SET);

	for(i= 0; i < 4; i++)
		bytes[i]= 0;

	for(i= 0; i < 4; i++)
	{
		currChar= fgetc(f);
		if(currChar == EOF)
			break;
		else
			bytes[i]= (unsigned char) currChar;
	}

	fseek(f, originalPosition, SEEK_SET);

	return unicoder_decodeBom(bytes);
}


/* Remember, this is multi-byte endianness we are referring to, not bit-level endianness.
   Bit-level endianness is handled by hardware. */
unsigned int unicoder_uint32_reverseByteEndian(unsigned int x)
{
	unsigned int ret= 0;

	ret |= x & 0x000000ff;
	ret <<= 8;
	ret |= (x >>  8) & 0x000000ff;
	ret <<= 8;
	ret |= (x >> 16) & 0x000000ff;
	ret <<= 8;
	ret |= (x >> 24) & 0x000000ff;


	return ret;
}






/* decodes utf-8 code point at p, stores uint32 in result, returns error code or number of bytes read */
int unicoder_utf8_decode(unsigned int* result, unsigned char* p)
{
	int i;
	int numBytes= 0;
	unsigned int decoded= 0;
	unsigned char currByte;
	const int debug_utf8_decode= 0;

	if(result == NULL)
		return UNICODER_NULL_POINTER;

	if(p == NULL)
		return UNICODER_NULL_POINTER;

	/*
	pasted from rfc 3629
	Char. number range  |        UTF-8 octet sequence
	   (hexadecimal)    |              (binary)
	--------------------+---------------------------------------------
	0000 0000-0000 007F | 0xxxxxxx
	0000 0080-0000 07FF | 110xxxxx 10xxxxxx
	0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
	0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
	*/

	currByte= *p;

	if((numBytes == 0) && ((currByte & 0x80) == 0x00))
		numBytes= 1;
	if((numBytes == 0) && ((currByte & 0xe0) == 0xc0))
		numBytes= 2;
	if((numBytes == 0) && ((currByte & 0xf0) == 0xe0))
		numBytes= 3;
	if((numBytes == 0) && ((currByte & 0xf8) == 0xf0))
		numBytes= 4;
	if(numBytes == 0)
		return UNICODER_INVALID_BYTE_SEQUENCE;

	for(i= 0; i < numBytes; i++)
	{
		currByte= *(p + i);

		if(i == 0)
		{
			switch(numBytes)
			{
				case 1: decoded= (0x7f & currByte);
					break;
				case 2: decoded= (0x1f & currByte);
					break;
				case 3: decoded= (0x0f & currByte);
					break;
				case 4: decoded= (0x07 & currByte);
					break;
				default:
					return UNICODER_UNPOSSIBLE;
			};
		}

		else
		{
			/* should have 10xx xxxx for each byte after first */
			if((currByte & 0xc0) != 0x80)
				return UNICODER_INVALID_BYTE_SEQUENCE;

			if(debug_utf8_decode == 1) printf("\t%08x\n", decoded);
			decoded <<= 6;
			if(debug_utf8_decode == 1) printf("\t%08x\n", decoded);
			decoded |= (currByte & 0x3f);
			if(debug_utf8_decode == 1) printf("\t%08x\n", decoded);
		}

		if(debug_utf8_decode == 1) printf("%08x\n", decoded);
	}

	/* max unicode code point is 0x10ffff */
	if(decoded > 0x10ffff)
		return UNICODER_INVALID_CODE_POINT;

	/* 0xd800 through 0xdfff are reserved for utf-16 surrogates */
	/* they are not valid unicode code points */
	if(0xd800 <= decoded  &&  decoded <= 0xdfff)
		return UNICODER_INVALID_CODE_POINT;

	*result= decoded;

	return numBytes;
}


/* encodes x to pointer p, return number of bytes written or error code */
int unicoder_utf8_encode(unsigned char* p, unsigned int x)
{
	unsigned int bytesRequired, i, temp;
	unsigned char currByte;

	if(p == NULL)
		return 0;

	/* max code point */
	if(x > 0x10ffff)
		return UNICODER_INVALID_CODE_POINT;

	/* reserved for utf-16 surrogates */
	if(0xd800 <= x  &&  x <= 0xdfff)
		return UNICODER_INVALID_CODE_POINT;

	if(x <= 0x7f)
		bytesRequired= 1;

	else if(x <= 0x07ff)
		bytesRequired= 2;

	else if(x <= 0xffff)
		bytesRequired= 3;

	else
		bytesRequired= 4;

	switch(bytesRequired)
	{
		case 1:
			temp= 0x00000000;
			temp |= ((x << 24) & 0x7f000000);
			break;

		case 2:
			temp= 0xc0800000;
			temp |= ((x << 18) & 0x1f000000);
			temp |= ((x << 16) & 0x003f0000);
			break;

		case 3:
			temp= 0xe0808000;
			temp |= ((x << 12) & 0x0f000000);
			temp |= ((x << 10) & 0x003f0000);
			temp |= ((x <<  8) & 0x00003f00);
			break;

		case 4:
			temp= 0xf0808080;
			temp |= ((x <<  6) & 0x07000000);
			temp |= ((x <<  4) & 0x003f0000);
			temp |= ((x <<  2) & 0x00003f00);
			temp |= ((x <<  0) & 0x0000003f);
			break;
	};

	for(i= 0; i < bytesRequired; i++)
	{
		*(p + i)= (unsigned char) (0x000000ff & (temp >> ((3 - i) * 8)));
	}

	return bytesRequired;
}


/* decode utf16 code point at p, store in result, endianness is UNICODER_(LES|BES), returns error code or number of bytes read */
int unicoder_utf16_decode(unsigned int* result, unsigned char* p, unsigned int endianness)
{
	unsigned int dbyteA, dbyteB, highSurrogate, lowSurrogate;

	if(endianness != UNICODER_BES  &&  endianness != UNICODER_LES)
		return UNICODER_ENDIANNESS_UNRECOGNIZED;

	if(p == NULL)
		return UNICODER_NULL_POINTER;

	if(result == NULL)
		return UNICODER_NULL_POINTER;

	/* left shift / right shift operators are endianness-independent */
	/* that is handled by the compiler on any given system */
	if(endianness == UNICODER_LES)
	{
		dbyteA= *(p + 1);
		dbyteA <<= 8;
		dbyteA |= *(p + 0);


		dbyteB= *(p + 3);
		dbyteB <<= 8;
		dbyteB |= *(p + 2);
	}

	else
	{
		dbyteA= *(p + 0);
		dbyteA <<= 8;
		dbyteA |= *(p + 1);

		dbyteB= *(p + 2);
		dbyteB <<= 8;
		dbyteB |= *(p + 3);
	}

	if(dbyteA < 0xd800  ||  0xdfff < dbyteA)
	{
		if(dbyteA > 0x10ffff)
			return UNICODER_INVALID_CODE_POINT;
		*result= dbyteA;

		/* success, we read a utf-16 code point in 2 bytes */
		return 2;
	}

	/*
	copied from rfc2781
	U' = yyyyyyyyyyxxxxxxxxxx
	W1 = 110110yyyyyyyyyy
	W2 = 110111xxxxxxxxxx
	Add 0x10000 to U' to obtain the character value U. Terminate.
	*/

	if(0xd800 <= dbyteA  &&  dbyteA <= 0xdbff)
	{
		if(0xdc00 <= dbyteB  &&  dbyteB <= 0xdfff)
		{
			highSurrogate= dbyteA & 0x03ff;
			lowSurrogate= dbyteB & 0x03ff;

			*result= 0;
			*result |= highSurrogate;
			*result <<= 10;
			*result |= lowSurrogate;
			*result += 0x010000;

			/* success, we read a utf-16 code point in 4 bytes */
			return 4;
		}

		else
			return UNICODER_INVALID_BYTE_SEQUENCE; /* received only one surrogate but not the other */
	}

	else
		return UNICODER_INVALID_BYTE_SEQUENCE; /* first surrogate was within [0xdc00, 0xdfff]; not valid */

	return UNICODER_UNPOSSIBLE;
}


/* encodes x to pointer p, return number of bytes written or error code, allows different endianness */
int unicoder_utf16_encode(unsigned char* p, unsigned int x, unsigned int endianness)
{
	unsigned int dbyteA, dbyteB, highSurrogate, lowSurrogate, requiredBytes, uPrime;
	unsigned char temp;
	const int debug_utf16_encode= 0;

	if(endianness != UNICODER_BES  &&  endianness != UNICODER_LES)
		return UNICODER_ENDIANNESS_UNRECOGNIZED;

	if(p == NULL)
		return UNICODER_NULL_POINTER;

	/* max code point */
	if(x > 0x10ffff)
		return UNICODER_INVALID_CODE_POINT;

	/* reserved for utf-16 surrogates */
	if(0xd800 <= x  &&  x <= 0xdfff)
		return UNICODER_INVALID_CODE_POINT;

	if(x < 0x010000)
	{
		if(endianness == UNICODER_LES)
		{
			temp= (unsigned char) (x & 0x000000ff);
			*p= temp;
			temp= (unsigned char) ((x & 0x0000ff00) >> 8);
			*(p + 1)= temp;
		}

		else
		{
			temp= (unsigned char) ((x & 0x0000ff00) >> 8);
			*p= temp;
			temp= (unsigned char) (x & 0x000000ff);
			*(p + 1)= temp;
		}

		return 2; /* wrote 2 bytes */
	}

	else
	{
		/*
		copied (sort of) from rfc2781
		U' = yyyy yyyy yyxx xxxx xxxx
		W1 =      1101 10yy yyyy yyyy
		W2 =      1101 11xx xxxx xxxx
		*/

		uPrime= x - 0x010000;
		highSurrogate= 0x0000d800 | ((uPrime >> 10) & 0x000003ff);
		lowSurrogate=  0x0000dc00 | (uPrime & 0x000003ff);

		if(debug_utf16_encode == 1) printf("\t\ths <%08x>     ls <%08x>     up <%08x>\n", highSurrogate, lowSurrogate, uPrime);

		/* high surrogate always appears first */
		/* low surrogate always appears second */
		/* endianness effects how each surrogate is written */
		if(endianness == UNICODER_LES)
		{
			temp= (unsigned char) (highSurrogate & 0x000000ff);
			*p= temp;
			temp= (unsigned char) ((highSurrogate & 0x0000ff00) >> 8);
			*(p + 1)= temp;
			temp= (unsigned char) (lowSurrogate & 0x000000ff);
			*(p + 2)= temp;
			temp= (unsigned char) ((lowSurrogate & 0x0000ff00) >> 8);
			*(p + 3)= temp;
		}

		else
		{
			temp= (unsigned char) ((highSurrogate & 0x0000ff00) >> 8);
			*p= temp;
			temp= (unsigned char) (highSurrogate & 0x000000ff);
			*(p + 1)= temp;
			temp= (unsigned char) ((lowSurrogate & 0x0000ff00) >> 8);
			*(p + 2)= temp;
			temp= (unsigned char) (lowSurrogate & 0x000000ff);
			*(p + 3)= temp;
		}

		return 4; /* wrote 4 bytes */
	}

	return UNICODER_UNPOSSIBLE;
}


/* decode utf32 code point at p, store in result, endianness is UNICODER_(LES|BES), returns error code or number of bytes read */
int unicoder_utf32_decode(unsigned int* result, unsigned char* p, unsigned int endianness)
{
	int i;
	unsigned int decoded;

	if(endianness != UNICODER_BES  &&  endianness != UNICODER_LES)
		return UNICODER_ENDIANNESS_UNRECOGNIZED;

	if(p == NULL)
		return UNICODER_NULL_POINTER;

	if(result == NULL)
		return UNICODER_NULL_POINTER;

	decoded= 0;

	for(i= 0; i < 4; i++)
	{
		if(endianness == UNICODER_BES)
		{
			if(i > 0) decoded <<= 8;
			decoded |= *(p + i);
		}

		else
		{
			if(i > 0) decoded >>= 8;
			decoded |= (((unsigned int) *(p + i)) << 24);
		}
	}

	/* max code point */
	if(decoded > 0x10ffff)
		return UNICODER_INVALID_CODE_POINT;

	/* reserved for utf-16 surrogates */
	if(0xd800 <= decoded  &&  decoded <= 0xdfff)
		return UNICODER_INVALID_CODE_POINT;

	*result= decoded;
	return 4; /* success, we read 4 bytes */
}

/* encodes x to pointer p, return number of bytes written or error code, allows different endianness */
int unicoder_utf32_encode(unsigned char* p, unsigned int x, unsigned int endianness)
{
	int i, index;
	unsigned int temp;
	const int debug_utf32_encode= 1;

	if(endianness != UNICODER_BES  &&  endianness != UNICODER_LES)
		return UNICODER_ENDIANNESS_UNRECOGNIZED;

	if(p == NULL)
		return UNICODER_NULL_POINTER;

	/* max code point */
	if(x > 0x10ffff)
		return UNICODER_INVALID_CODE_POINT;

	/* reserved for utf-16 surrogates */
	if(0xd800 <= x  &&  x <= 0xdfff)
		return UNICODER_INVALID_CODE_POINT;

	for(i= 0; i < 4; i++)
	{
		temp= (x << (i * 8)) & 0xff000000;

		if(endianness == UNICODER_BES)
			index= i;

		else
			index= 3 - i;

		*(p + index)= (unsigned char) ((temp >> 24) & 0x000000ff);
	}

	return 4;
}





/* reads single code point from p, store in result, returns error code or number of bytes read */
int unicoder_readCodePoint(unsigned char* p, unsigned int* result, unsigned int encoding)
{
	unsigned char temp[4];
	int i;
	unsigned int originalPosition, currentChar, bytesRead;

	if(p == NULL)
		return UNICODER_NULL_POINTER;

	if(result == NULL)
		return UNICODER_NULL_POINTER;

	switch(encoding)
	{
		case UNICODER_ASCII:
			(*result)= (unsigned int) (*p);
			bytesRead= 1;
			break;

		case UNICODER_UTF8:
			bytesRead= unicoder_utf8_decode(result, p);
			break;

		case UNICODER_UTF16BE:
			bytesRead= unicoder_utf16_decode(result, p, UNICODER_BES);
			break;

		case UNICODER_UTF16LE:
			bytesRead= unicoder_utf16_decode(result, p, UNICODER_LES);
			break;

		case UNICODER_UTF32BE:
			bytesRead= unicoder_utf32_decode(result, p, UNICODER_BES);
			break;

		case UNICODER_UTF32LE:
			bytesRead= unicoder_utf32_decode(result, p, UNICODER_LES);
			break;

		default:
			return UNICODER_ENCODING_UNRECOGNIZED;
			break;
	};

	return bytesRead;
}


/* reads single code point from file f, store in result, returns error code or number of bytes read */
int unicoder_readCodePointFromFile(FILE* f, unsigned int* result, unsigned int encoding)
{
	int i, currChar, bytesRead;
	long originalPosition;
	unsigned char bytes[6];

	if(f == NULL)
		return UNICODER_FILE_IO_ERROR;

	originalPosition= ftell(f);

	/* max unicode code point width is 6 bytes */
	for(i= 0; i < 6; i++)
		bytes[i]= 0;

	for(i= 0; i < 6; i++)
	{
		currChar= fgetc(f);
		if(currChar == EOF)
			break;
		else
			bytes[i]= (unsigned char) currChar;
	}

	bytesRead= unicoder_readCodePoint(bytes, result, encoding);

	/* seek to new position if successful, to original position otherwise */
	if(bytesRead >= 0)
		fseek(f, originalPosition + bytesRead, SEEK_SET);
	else
		fseek(f, originalPosition, SEEK_SET);

	/* we want to skip the BOM */
	if((*result) == 0x0000feff)
		return unicoder_readCodePointFromFile(f, result, encoding);

	return bytesRead;
}


/* writes single code point to p, returns number of bytes written or error code */
int unicoder_writeCodePoint(unsigned char* p, unsigned int x, unsigned int encoding)
{
	if(p == NULL)
		return UNICODER_NULL_POINTER;

	switch(encoding)

	{
		case UNICODER_ASCII:
			if(x > 0x0000007f)
				return UNICODER_OUT_OF_ASCII_RANGE;
			(*p)= (unsigned char) x;
			return 1;
			break;

		case UNICODER_UTF8:
			return unicoder_utf8_encode(p, x);
			break;

		case UNICODER_UTF16BE:
			return unicoder_utf16_encode(p, x, UNICODER_BES);
			break;

		case UNICODER_UTF16LE:
			return unicoder_utf16_encode(p, x, UNICODER_LES);
			break;


		case UNICODER_UTF32BE:
			return unicoder_utf32_encode(p, x, UNICODER_BES);
			break;


		case UNICODER_UTF32LE:
			return unicoder_utf32_encode(p, x, UNICODER_LES);
			break;

		default:
			return UNICODER_ENCODING_UNRECOGNIZED;
			break;
	};

	/* we should have returned by now, we should never get here */
	return UNICODER_UNPOSSIBLE;
}


/* writes single code point to file, returns number of bytes written or error code */
int unicoder_writeCodePointToFile(FILE* file, unsigned int x, unsigned int encoding)
{
	unsigned char p[4];
	int i, numBytes, fputcRet;

	if(file == NULL)
		return UNICODER_FILE_IO_ERROR;

	numBytes= 0;

	for(i= 0; i < 4; i++)
		p[i]= 0;

	numBytes= unicoder_writeCodePoint(p, x, encoding);

	if(numBytes < 1)
		return numBytes;

	for(i= 0; i < numBytes; i++)
	{
		fputcRet= fputc(p[i], file);
		if(fputcRet == EOF)
			return UNICODER_FILE_IO_ERROR;
	}

	return numBytes;
}


/* writes byte order mark to file f, returns number of bytes written or error code */
unsigned int unicoder_writeBomToFile(FILE* f, unsigned int encoding)
{
	if(f == NULL)
		return UNICODER_FILE_IO_ERROR;

	if(encoding == UNICODER_ASCII)
		return 0;

	return unicoder_writeCodePointToFile(f, 0x0000feff, encoding);
}


/* writes cstring to f using encoding, returns number bytes written or error code */
int unicoder_writeCStringToFile(FILE* f, const char* cstr, unsigned int encoding)
{
	unsigned int len, i, j, numBytes, returnValue;

	if(f == NULL)
		return UNICODER_NULL_POINTER;

	if(cstr == NULL)
		return UNICODER_NULL_POINTER;

	if(encoding < UNICODER_ASCII  ||  UNICODER_UTF32LE < encoding)
		return UNICODER_ENCODING_UNRECOGNIZED;

	len= strnlen(cstr, 1000005);
	if(len > 1000000)
		return UNICODER_TOO_LONG_CSTRING;

	for(i= 0, numBytes= 0; i < len; i++)
		numBytes += unicoder_writeCodePointToFile(f, *(cstr + i), encoding);

	return numBytes;
}





#endif
