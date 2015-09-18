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


#ifndef  UNICODER_H
#define  UNICODER_H  1

#include <stdio.h>

/* error codes for function returns */
#define  UNICODER_NULL_POINTER            -1
#define  UNICODER_ENCODING_UNRECOGNIZED   -2
#define  UNICODER_ENDIANNESS_UNRECOGNIZED -4
#define  UNICODER_INVALID_BYTE_SEQUENCE   -8 /* tried to decode something that was encoded incorrectly */
#define  UNICODER_INVALID_CODE_POINT     -16
#define  UNICODER_TOO_LONG_CSTRING       -32
#define  UNICODER_FILE_IO_ERROR          -64
#define  UNICODER_EOF                   -128
#define  UNICODER_OUT_OF_ASCII_RANGE    -256
#define  UNICODER_BAD_LENGTH            -512 /* from unicoder_reverseEndianness() */
#define  UNICODER_UNPOSSIBLE           -1024 /* should never happen, indicates bug in this library */


/* Codes for endianness types. */
/* We will only deal with straight endiannes. */
/* IOW, no odd/even bytes or crap like that (0x01234567 > 01 45 23 67), this is right out. */
/* Must go straight from most significant byte to least significant byte or vice versa. */
#define  UNICODER_LES  1 /* 0x01234567 > 67 45 23 01, AKA Little Endian Straight */
#define  UNICODER_BES  2 /* 0x01234567 > 01 23 45 67, AKA Big Endian Straight */


/* different encoding types */
#define  UNICODER_ASCII    1
#define  UNICODER_UTF8     2
#define  UNICODER_UTF16BE  3
#define  UNICODER_UTF16LE  4
#define  UNICODER_UTF32BE  5
#define  UNICODER_UTF32LE  6



/* tests for little endian straight or big endian straight, returns UNICODER_ENDIANNESS_UNRECOGNIZED otherwise */
int unicoder_getMachineEndianness();


int unicoder_reverseEndianness(unsigned char* src, unsigned char* dest, unsigned int length);


/* uses byte order mark to determine encoding type */
int unicoder_decodeBom(unsigned char* p);


/* uses byte order mark to determine encoding type */
int unicoder_decodeBomFromFile(FILE* f);


/* Remember, this is multi-byte endianness we are referring to, not bit-level endianness.
   Bit-level endianness is handled by hardware. */
unsigned int unicoder_uint32_reverseByteEndian(unsigned int x);






/* decodes utf-8 code point at p, stores uint32 in result, returns error code or number of bytes read */
int unicoder_utf8_decode(unsigned int* result, unsigned char* p);


/* encodes x to pointer p, return number of bytes written or error code */
int unicoder_utf8_encode(unsigned char* p, unsigned int x);


/* decode utf16 code point at p, store in result, endianness is UNICODER_(LES|BES), returns error code or number of bytes read */
int unicoder_utf16_decode(unsigned int* result, unsigned char* p, unsigned int endianness);


/* encodes x to pointer p, return number of bytes written or error code, allows different endianness */
int unicoder_utf16_encode(unsigned char* p, unsigned int x, unsigned int endianness);


/* decode utf32 code point at p, store in result, endianness is UNICODER_(LES|BES), returns error code or number of bytes read */
int unicoder_utf32_decode(unsigned int* result, unsigned char* p, unsigned int endianness);

/* encodes x to pointer p, return number of bytes written or error code, allows different endianness */
int unicoder_utf32_encode(unsigned char* p, unsigned int x, unsigned int endianness);





/* reads single code point from p, store in result, returns error code or number of bytes read */
int unicoder_readCodePoint(unsigned char* p, unsigned int* result, unsigned int encoding);


/* reads single code point from file f, store in result, returns error code or number of bytes read */
int unicoder_readCodePointFromFile(FILE* f, unsigned int* result, unsigned int encoding);


/* writes single code point to p, returns number of bytes written or error code */
int unicoder_writeCodePoint(unsigned char* p, unsigned int x, unsigned int encoding);


/* writes single code point to file, returns number of bytes written or error code */
int unicoder_writeCodePointToFile(FILE* file, unsigned int x, unsigned int encoding);


/* writes byte order mark to file f, returns number of bytes written or error code */
unsigned int unicoder_writeBomToFile(FILE* f, unsigned int encoding);


/* writes cstring to f using encoding, returns number bytes written or error code */
int unicoder_writeCStringToFile(FILE* f, const char* cstr, unsigned int encoding);





#endif
