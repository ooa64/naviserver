/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * The Initial Developer of the Original Code and related documentation
 * is America Online, Inc. Portions created by AOL are Copyright (C) 1999
 * America Online, Inc. All Rights Reserved.
 *
 */

/*
 * urlencode.c --
 *
 *      Encode and decode strings with percent encoding, as covered in
 *      - RFC 3986 (Uniform Resource Identifier (URI): Generic Syntax)
 *      - RFC 6265 (HTTP State Management Mechanism)
 *
 *      When the code is complied with RFC1738 activated, the encoding
 *      of prior versions is used.
 */

#include "nsd.h"

/*
 * The following structure defines the encoding attributes
 * of a byte.
 */

typedef struct ByteKey {
    TCL_SIZE_T  len;   /* Length required to encode string. */
    const char *str;   /* String for multibyte encoded character. */
} ByteKey;

/*
 * Local functions defined in this file.
 */

static char *UrlEncode(Tcl_DString *dsPtr, const char *urlSegment,
                       Tcl_Encoding encoding, char percentScheme, bool upperCase)
    NS_GNUC_NONNULL(1) NS_GNUC_NONNULL(2);
static char *UrlDecode(Tcl_DString *dsPtr, const char *urlSegment,
                       Tcl_Encoding encoding, char percentScheme, Ns_ReturnCode *resultPtr)
    NS_GNUC_NONNULL(1) NS_GNUC_NONNULL(2);

static TCL_SIZE_T PercentDecode(char *dest, const char *source, char percentScheme)
    NS_GNUC_NONNULL(1) NS_GNUC_NONNULL(2);

static int UrlPercentDecode(NsInterp *itPtr, const char *inputStr,
                            char percentScheme, const char *charset,
                            Tcl_Obj *fallbackCharsetObj)
    NS_GNUC_NONNULL(1) NS_GNUC_NONNULL(2);

static Ns_ReturnCode UrlFallbackDecode(NsInterp *itPtr, Tcl_DString *dsPtr,
                                       const char *inputStr, Tcl_Obj *fallbackCharsetObj,
                                       Tcl_Encoding initialEncoding, char percentScheme)
        NS_GNUC_NONNULL(1) NS_GNUC_NONNULL(2)  NS_GNUC_NONNULL(3);
/*
 * Local variables defined in this file.
 */

static Ns_ObjvTable percentSchemes[] = {
    {"query",    UCHAR('q')},
    {"path",     UCHAR('p')},
    {"cookie",   UCHAR('c')},
    {"oauth1",   UCHAR('o')},
    {NULL,       0u}
};

#ifdef RFC1738

/*
 * The following table is used for encoding and decoding the
 * segments of a URI query component according to RFC 1739.
 *
 * All ASCII control characters (00-1f and 7f) and the URI
 * 'delim' and 'unwise' characters are encoded.  In addition, the
 * following URI query component reserved characters are also
 * encoded:
 *
 *      $  &  +  ,  /  :  ;  =  ?  @
 *
 * The ASCII space character receives special treatment and is
 * encoded as +.  This is handled by the encoding/decoding
 * routines and is not represented in the table below.
 *
 */

static const ByteKey query_scheme[] = {
    {3, "00"}, {3, "01"}, {3, "02"}, {3, "03"},
    {3, "04"}, {3, "05"}, {3, "06"}, {3, "07"},
    {3, "08"}, {3, "09"}, {3, "0a"}, {3, "0b"},
    {3, "0c"}, {3, "0d"}, {3, "0e"}, {3, "0f"},
    {3, "10"}, {3, "11"}, {3, "12"}, {3, "13"},
    {3, "14"}, {3, "15"}, {3, "16"}, {3, "17"},
    {3, "18"}, {3, "19"}, {3, "1a"}, {3, "1b"},
    {3, "1c"}, {3, "1d"}, {3, "1e"}, {3, "1f"},
    {3, "20"}, {1, NULL}, {3, "22"}, {3, "23"},
    {3, "24"}, {3, "25"}, {3, "26"}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {3, "2b"},
    {3, "2c"}, {1, NULL}, {1, NULL}, {3, "2f"},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {3, "3a"}, {3, "3b"},
    {3, "3c"}, {3, "3d"}, {3, "3e"}, {3, "3f"},
    {3, "40"}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {3, "5b"},
    {3, "5c"}, {3, "5d"}, {3, "5e"}, {1, NULL},
    {3, "60"}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {3, "7b"},
    {3, "7c"}, {3, "7d"}, {1, NULL}, {3, "7f"},
    {3, "80"}, {3, "81"}, {3, "82"}, {3, "83"},
    {3, "84"}, {3, "85"}, {3, "86"}, {3, "87"},
    {3, "88"}, {3, "89"}, {3, "8a"}, {3, "8b"},
    {3, "8c"}, {3, "8d"}, {3, "8e"}, {3, "8f"},
    {3, "90"}, {3, "91"}, {3, "92"}, {3, "93"},
    {3, "94"}, {3, "95"}, {3, "96"}, {3, "97"},
    {3, "98"}, {3, "99"}, {3, "9a"}, {3, "9b"},
    {3, "9c"}, {3, "9d"}, {3, "9e"}, {3, "9f"},
    {3, "a0"}, {3, "a1"}, {3, "a2"}, {3, "a3"},
    {3, "a4"}, {3, "a5"}, {3, "a6"}, {3, "a7"},
    {3, "a8"}, {3, "a9"}, {3, "aa"}, {3, "ab"},
    {3, "ac"}, {3, "ad"}, {3, "ae"}, {3, "af"},
    {3, "b0"}, {3, "b1"}, {3, "b2"}, {3, "b3"},
    {3, "b4"}, {3, "b5"}, {3, "b6"}, {3, "b7"},
    {3, "b8"}, {3, "b9"}, {3, "ba"}, {3, "bb"},
    {3, "bc"}, {3, "bd"}, {3, "be"}, {3, "bf"},
    {3, "c0"}, {3, "c1"}, {3, "c2"}, {3, "c3"},
    {3, "c4"}, {3, "c5"}, {3, "c6"}, {3, "c7"},
    {3, "c8"}, {3, "c9"}, {3, "ca"}, {3, "cb"},
    {3, "cc"}, {3, "cd"}, {3, "ce"}, {3, "cf"},
    {3, "d0"}, {3, "d1"}, {3, "d2"}, {3, "d3"},
    {3, "d4"}, {3, "d5"}, {3, "d6"}, {3, "d7"},
    {3, "d8"}, {3, "d9"}, {3, "da"}, {3, "db"},
    {3, "dc"}, {3, "dd"}, {3, "de"}, {3, "df"},
    {3, "e0"}, {3, "e1"}, {3, "e2"}, {3, "e3"},
    {3, "e4"}, {3, "e5"}, {3, "e6"}, {3, "e7"},
    {3, "e8"}, {3, "e9"}, {3, "ea"}, {3, "eb"},
    {3, "ec"}, {3, "ed"}, {3, "ee"}, {3, "ef"},
    {3, "f0"}, {3, "f1"}, {3, "f2"}, {3, "f3"},
    {3, "f4"}, {3, "f5"}, {3, "f6"}, {3, "f7"},
    {3, "f8"}, {3, "f9"}, {3, "fa"}, {3, "fb"},
    {3, "fc"}, {3, "fd"}, {3, "fe"}, {3, "ff"}
};

/*
 * The following table is used for encoding and decoding the
 * segments of a URI path component.
 *
 * All ASCII control characters (00-1f and 7f) and the URI 'delim'
 * and 'unwise' characters are encoded.  In addition, the following
 * URI path component reserved characters are also encoded:
 *
 *      /  ;  =  ?
 *
 */

static const ByteKey path_scheme[] = {
    {3, "00"}, {3, "01"}, {3, "02"}, {3, "03"},
    {3, "04"}, {3, "05"}, {3, "06"}, {3, "07"},
    {3, "08"}, {3, "09"}, {3, "0a"}, {3, "0b"},
    {3, "0c"}, {3, "0d"}, {3, "0e"}, {3, "0f"},
    {3, "10"}, {3, "11"}, {3, "12"}, {3, "13"},
    {3, "14"}, {3, "15"}, {3, "16"}, {3, "17"},
    {3, "18"}, {3, "19"}, {3, "1a"}, {3, "1b"},
    {3, "1c"}, {3, "1d"}, {3, "1e"}, {3, "1f"},
    {3, "20"}, {1, NULL}, {3, "22"}, {3, "23"},
    {1, NULL}, {3, "25"}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {3, "2f"},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {3, "3b"},
    {3, "3c"}, {3, "3d"}, {3, "3e"}, {3, "3f"},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {3, "5b"},
    {3, "5c"}, {3, "5d"}, {3, "5e"}, {1, NULL},
    {3, "60"}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    {1, NULL}, {1, NULL}, {1, NULL}, {3, "7b"},
    {3, "7c"}, {3, "7d"}, {1, NULL}, {3, "7f"},
    {3, "80"}, {3, "81"}, {3, "82"}, {3, "83"},
    {3, "84"}, {3, "85"}, {3, "86"}, {3, "87"},
    {3, "88"}, {3, "89"}, {3, "8a"}, {3, "8b"},
    {3, "8c"}, {3, "8d"}, {3, "8e"}, {3, "8f"},
    {3, "90"}, {3, "91"}, {3, "92"}, {3, "93"},
    {3, "94"}, {3, "95"}, {3, "96"}, {3, "97"},
    {3, "98"}, {3, "99"}, {3, "9a"}, {3, "9b"},
    {3, "9c"}, {3, "9d"}, {3, "9e"}, {3, "9f"},
    {3, "a0"}, {3, "a1"}, {3, "a2"}, {3, "a3"},
    {3, "a4"}, {3, "a5"}, {3, "a6"}, {3, "a7"},
    {3, "a8"}, {3, "a9"}, {3, "aa"}, {3, "ab"},
    {3, "ac"}, {3, "ad"}, {3, "ae"}, {3, "af"},
    {3, "b0"}, {3, "b1"}, {3, "b2"}, {3, "b3"},
    {3, "b4"}, {3, "b5"}, {3, "b6"}, {3, "b7"},
    {3, "b8"}, {3, "b9"}, {3, "ba"}, {3, "bb"},
    {3, "bc"}, {3, "bd"}, {3, "be"}, {3, "bf"},
    {3, "c0"}, {3, "c1"}, {3, "c2"}, {3, "c3"},
    {3, "c4"}, {3, "c5"}, {3, "c6"}, {3, "c7"},
    {3, "c8"}, {3, "c9"}, {3, "ca"}, {3, "cb"},
    {3, "cc"}, {3, "cd"}, {3, "ce"}, {3, "cf"},
    {3, "d0"}, {3, "d1"}, {3, "d2"}, {3, "d3"},
    {3, "d4"}, {3, "d5"}, {3, "d6"}, {3, "d7"},
    {3, "d8"}, {3, "d9"}, {3, "da"}, {3, "db"},
    {3, "dc"}, {3, "dd"}, {3, "de"}, {3, "df"},
    {3, "e0"}, {3, "e1"}, {3, "e2"}, {3, "e3"},
    {3, "e4"}, {3, "e5"}, {3, "e6"}, {3, "e7"},
    {3, "e8"}, {3, "e9"}, {3, "ea"}, {3, "eb"},
    {3, "ec"}, {3, "ed"}, {3, "ee"}, {3, "ef"},
    {3, "f0"}, {3, "f1"}, {3, "f2"}, {3, "f3"},
    {3, "f4"}, {3, "f5"}, {3, "f6"}, {3, "f7"},
    {3, "f8"}, {3, "f9"}, {3, "fa"}, {3, "fb"},
    {3, "fc"}, {3, "fd"}, {3, "fe"}, {3, "ff"}
};
#else
/*
 * The following table is used for encoding and decoding the segments of
 * a URI query component based on RFC 3986 (Uniform Resource Identifier
 * (URI): Generic Syntax, 2005)
 *
 * A percent-encoding is used to represent a data octet in a component
 * when that octet's corresponding character is outside the allowed set
 * or is being used as a delimiter of, or within, the component.
 *
 * For all components all ASCII control characters (00-1f and 7f) and
 * characters above 7f are encoded, 'unreserved' characters are never
 * encoded.
 *
 * The query part of a URL is defined as:
 *
 *    query       = *( pchar / "/" / "?" )
 *    pchar       = unreserved / pct-encoded / sub-delims / ":" / "@"
 *    unreserved  = ALPHA / DIGIT / "-" / "." / "_" / "~"
 *    sub-delims  = "!" / "$" / "&" / "'" / "(" / ")"
 *
 * The RFC just defines the "outer" syntax of the query, the content is
 * usually form-urlencoded, where "&", "=" and "+" have special
 * meanings (https://www.w3.org/TR/html401/interact/forms.html#h-17.13.4.1)
 * so only the following sub-delims are allowed literally.
 *
 *   query-sub-delims1 = "!" / "$" / "'" / "(" / ")" / "*" / "," / ";"
 *
 * In order to make query-component usable for encoding/decoding cookies,
 * the characters "," and ";" have to be percent-encoded as well.
 *
 *   query-sub-delims = "!" / "$" / "'" / "(" / ")" / "*"
 *
 * This means a total of 76 characters are allowed unencoded in query
 * parts:
 *    unreserved:       26 + 26 + 10 + 4 = 66
 *    query-sub-delims: 6
 *    pchar:            66 + 6 + 2 = 74
 *    query:            76 + 2 = 76
 *
 * Unprotected characters:
 *
 *   ! $ ' ( ) * + - . / 0 1 2 3 4 5 6 7 8 9 : ? @
 *   A B C D E F G H I J K L M N O P Q R S T U V W X Y Z _
 *   a b c d e f g h i j k l m n o p q r s t u v w x y z ~
 */

static const ByteKey query_scheme[] = {
    /* 0x00 */  {3, "00"}, {3, "01"}, {3, "02"}, {3, "03"},
    /* 0x04 */  {3, "04"}, {3, "05"}, {3, "06"}, {3, "07"},
    /* 0x08 */  {3, "08"}, {3, "09"}, {3, "0a"}, {3, "0b"},
    /* 0x0c */  {3, "0c"}, {3, "0d"}, {3, "0e"}, {3, "0f"},
    /* 0x10 */  {3, "10"}, {3, "11"}, {3, "12"}, {3, "13"},
    /* 0x14 */  {3, "14"}, {3, "15"}, {3, "16"}, {3, "17"},
    /* 0x18 */  {3, "18"}, {3, "19"}, {3, "1a"}, {3, "1b"},
    /* 0x1c */  {3, "1c"}, {3, "1d"}, {3, "1e"}, {3, "1f"},
    /* 0x20 */  {1, NULL}, {1, NULL}, {3, "22"}, {3, "23"},
    /* 0x24 */  {1, NULL}, {3, "25"}, {3, "26"}, {1, NULL},
    /* 0x28 */  {1, NULL}, {1, NULL}, {1, NULL}, {3, "2b"},
    /* 0x2c */  {3, "2c"}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x30 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x34 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x38 */  {1, NULL}, {1, NULL}, {1, NULL}, {3, "3b"},
    /* 0x3c */  {3, "3c"}, {3, "3d"}, {3, "3e"}, {1, NULL},
    /* 0x40 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x44 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x48 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x4c */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x50 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x54 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x58 */  {1, NULL}, {1, NULL}, {1, NULL}, {3, "5b"},
    /* 0x5c */  {3, "5c"}, {3, "5d"}, {3, "5e"}, {1, NULL},
    /* 0x60 */  {3, "60"}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x64 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x68 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x6c */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x70 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x74 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x78 */  {1, NULL}, {1, NULL}, {1, NULL}, {3, "7b"},
    /* 0x7c */  {3, "7c"}, {3, "7d"}, {1, NULL}, {3, "7f"},
    /* 0x80 */  {3, "80"}, {3, "81"}, {3, "82"}, {3, "83"},
    /* 0x84 */  {3, "84"}, {3, "85"}, {3, "86"}, {3, "87"},
    /* 0x88 */  {3, "88"}, {3, "89"}, {3, "8a"}, {3, "8b"},
    /* 0x8c */  {3, "8c"}, {3, "8d"}, {3, "8e"}, {3, "8f"},
    /* 0x90 */  {3, "90"}, {3, "91"}, {3, "92"}, {3, "93"},
    /* 0x94 */  {3, "94"}, {3, "95"}, {3, "96"}, {3, "97"},
    /* 0x98 */  {3, "98"}, {3, "99"}, {3, "9a"}, {3, "9b"},
    /* 0x9c */  {3, "9c"}, {3, "9d"}, {3, "9e"}, {3, "9f"},
    /* 0xa0 */  {3, "a0"}, {3, "a1"}, {3, "a2"}, {3, "a3"},
    /* 0xa4 */  {3, "a4"}, {3, "a5"}, {3, "a6"}, {3, "a7"},
    /* 0xa8 */  {3, "a8"}, {3, "a9"}, {3, "aa"}, {3, "ab"},
    /* 0xac */  {3, "ac"}, {3, "ad"}, {3, "ae"}, {3, "af"},
    /* 0xb0 */  {3, "b0"}, {3, "b1"}, {3, "b2"}, {3, "b3"},
    /* 0xb4 */  {3, "b4"}, {3, "b5"}, {3, "b6"}, {3, "b7"},
    /* 0xb8 */  {3, "b8"}, {3, "b9"}, {3, "ba"}, {3, "bb"},
    /* 0xbc */  {3, "bc"}, {3, "bd"}, {3, "be"}, {3, "bf"},
    /* 0xc0 */  {3, "c0"}, {3, "c1"}, {3, "c2"}, {3, "c3"},
    /* 0xc4 */  {3, "c4"}, {3, "c5"}, {3, "c6"}, {3, "c7"},
    /* 0xc8 */  {3, "c8"}, {3, "c9"}, {3, "ca"}, {3, "cb"},
    /* 0xcc */  {3, "cc"}, {3, "cd"}, {3, "ce"}, {3, "cf"},
    /* 0xd0 */  {3, "d0"}, {3, "d1"}, {3, "d2"}, {3, "d3"},
    /* 0xd4 */  {3, "d4"}, {3, "d5"}, {3, "d6"}, {3, "d7"},
    /* 0xd8 */  {3, "d8"}, {3, "d9"}, {3, "da"}, {3, "db"},
    /* 0xdc */  {3, "dc"}, {3, "dd"}, {3, "de"}, {3, "df"},
    /* 0xe0 */  {3, "e0"}, {3, "e1"}, {3, "e2"}, {3, "e3"},
    /* 0xe4 */  {3, "e4"}, {3, "e5"}, {3, "e6"}, {3, "e7"},
    /* 0xe8 */  {3, "e8"}, {3, "e9"}, {3, "ea"}, {3, "eb"},
    /* 0xec */  {3, "ec"}, {3, "ed"}, {3, "ee"}, {3, "ef"},
    /* 0xf0 */  {3, "f0"}, {3, "f1"}, {3, "f2"}, {3, "f3"},
    /* 0xf4 */  {3, "f4"}, {3, "f5"}, {3, "f6"}, {3, "f7"},
    /* 0xf8 */  {3, "f8"}, {3, "f9"}, {3, "fa"}, {3, "fb"},
    /* 0xfc */  {3, "fc"}, {3, "fd"}, {3, "fe"}, {3, "ff"}
};


/*
 * The following table is used for encoding and decoding the segments of
 * a URI path component based on RFC 3986 (Uniform Resource Identifier
 * (URI): Generic Syntax, 2005)
 *
 * The segment part of a URL is defined as:
 *
 *    segment     = *pchar
 *    pchar       = unreserved / pct-encoded / sub-delims / ":" / "@"
 *    unreserved  = ALPHA / DIGIT / "-" / "." / "_" / "~"
 *    sub-delims  = "!" / "$" / "&" / "'" / "(" / ")"
 *                   / "*" / "+" / "," / ";" / "="
 *
 * The RFC states that semicolon (";") and equals ("=") reserved
 * characters are often used to delimit parameters and parameter values
 * applicable to that segment (whatever "often" means!). To be on the safe
 * side, and to support that characters as part of the segment, these are
 * encoded.
 *
 *    segment-sub-delims  = "!" / "$" / "&" / "'" / "(" / ")"
 *                         / "*" / "+" / ","

 * This means a total of 77 characters are allowed unencoded in segment
 * parts:
 *    unreserved:         26 + 26 + 10 + 4 = 66
 *    segment-sub-delims: 9
 *    segment-chars:      66 + 2 + 9 = 77
 *
 * Unprotected characters:
 *
 *    ! $ & ' ( ) * + , - . 0 1 2 3 4 5 6 7 8 9 : @
 *    A B C D E F G H I J K L M N O P Q R S T U V W X Y Z _
 *    a b c d e f g h i j k l m n o p q r s t u v w x y z ~
 */


static const ByteKey path_scheme[] = {
    /* 0x00 */  {3, "00"}, {3, "01"}, {3, "02"}, {3, "03"},
    /* 0x04 */  {3, "04"}, {3, "05"}, {3, "06"}, {3, "07"},
    /* 0x08 */  {3, "08"}, {3, "09"}, {3, "0a"}, {3, "0b"},
    /* 0x0c */  {3, "0c"}, {3, "0d"}, {3, "0e"}, {3, "0f"},
    /* 0x10 */  {3, "10"}, {3, "11"}, {3, "12"}, {3, "13"},
    /* 0x14 */  {3, "14"}, {3, "15"}, {3, "16"}, {3, "17"},
    /* 0x18 */  {3, "18"}, {3, "19"}, {3, "1a"}, {3, "1b"},
    /* 0x1c */  {3, "1c"}, {3, "1d"}, {3, "1e"}, {3, "1f"},
    /* 0x20 */  {3, "20"}, {1, NULL}, {3, "22"}, {3, "23"},
    /* 0x24 */  {1, NULL}, {3, "25"}, {1, NULL}, {1, NULL},
    /* 0x28 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x2c */  {1, NULL}, {1, NULL}, {1, NULL}, {3, "2f"},
    /* 0x30 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x34 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x38 */  {1, NULL}, {1, NULL}, {1, NULL}, {3, "3b"},
    /* 0x3c */  {3, "3c"}, {3, "3d"}, {3, "3e"}, {3, "3f"},
    /* 0x40 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x44 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x48 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x4c */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x50 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x54 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x58 */  {1, NULL}, {1, NULL}, {1, NULL}, {3, "5b"},
    /* 0x5c */  {3, "5c"}, {3, "5d"}, {3, "5e"}, {1, NULL},
    /* 0x60 */  {3, "60"}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x64 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x68 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x6c */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x70 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x74 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x78 */  {1, NULL}, {1, NULL}, {1, NULL}, {3, "7b"},
    /* 0x7c */  {3, "7c"}, {3, "7d"}, {1, NULL}, {3, "7f"},
    /* 0x80 */  {3, "80"}, {3, "81"}, {3, "82"}, {3, "83"},
    /* 0x84 */  {3, "84"}, {3, "85"}, {3, "86"}, {3, "87"},
    /* 0x88 */  {3, "88"}, {3, "89"}, {3, "8a"}, {3, "8b"},
    /* 0x8c */  {3, "8c"}, {3, "8d"}, {3, "8e"}, {3, "8f"},
    /* 0x90 */  {3, "90"}, {3, "91"}, {3, "92"}, {3, "93"},
    /* 0x94 */  {3, "94"}, {3, "95"}, {3, "96"}, {3, "97"},
    /* 0x98 */  {3, "98"}, {3, "99"}, {3, "9a"}, {3, "9b"},
    /* 0x9c */  {3, "9c"}, {3, "9d"}, {3, "9e"}, {3, "9f"},
    /* 0xa0 */  {3, "a0"}, {3, "a1"}, {3, "a2"}, {3, "a3"},
    /* 0xa4 */  {3, "a4"}, {3, "a5"}, {3, "a6"}, {3, "a7"},
    /* 0xa8 */  {3, "a8"}, {3, "a9"}, {3, "aa"}, {3, "ab"},
    /* 0xac */  {3, "ac"}, {3, "ad"}, {3, "ae"}, {3, "af"},
    /* 0xb0 */  {3, "b0"}, {3, "b1"}, {3, "b2"}, {3, "b3"},
    /* 0xb4 */  {3, "b4"}, {3, "b5"}, {3, "b6"}, {3, "b7"},
    /* 0xb8 */  {3, "b8"}, {3, "b9"}, {3, "ba"}, {3, "bb"},
    /* 0xbc */  {3, "bc"}, {3, "bd"}, {3, "be"}, {3, "bf"},
    /* 0xc0 */  {3, "c0"}, {3, "c1"}, {3, "c2"}, {3, "c3"},
    /* 0xc4 */  {3, "c4"}, {3, "c5"}, {3, "c6"}, {3, "c7"},
    /* 0xc8 */  {3, "c8"}, {3, "c9"}, {3, "ca"}, {3, "cb"},
    /* 0xcc */  {3, "cc"}, {3, "cd"}, {3, "ce"}, {3, "cf"},
    /* 0xd0 */  {3, "d0"}, {3, "d1"}, {3, "d2"}, {3, "d3"},
    /* 0xd4 */  {3, "d4"}, {3, "d5"}, {3, "d6"}, {3, "d7"},
    /* 0xd8 */  {3, "d8"}, {3, "d9"}, {3, "da"}, {3, "db"},
    /* 0xdc */  {3, "dc"}, {3, "dd"}, {3, "de"}, {3, "df"},
    /* 0xe0 */  {3, "e0"}, {3, "e1"}, {3, "e2"}, {3, "e3"},
    /* 0xe4 */  {3, "e4"}, {3, "e5"}, {3, "e6"}, {3, "e7"},
    /* 0xe8 */  {3, "e8"}, {3, "e9"}, {3, "ea"}, {3, "eb"},
    /* 0xec */  {3, "ec"}, {3, "ed"}, {3, "ee"}, {3, "ef"},
    /* 0xf0 */  {3, "f0"}, {3, "f1"}, {3, "f2"}, {3, "f3"},
    /* 0xf4 */  {3, "f4"}, {3, "f5"}, {3, "f6"}, {3, "f7"},
    /* 0xf8 */  {3, "f8"}, {3, "f9"}, {3, "fa"}, {3, "fb"},
    /* 0xfc */  {3, "fc"}, {3, "fd"}, {3, "fe"}, {3, "ff"}
};
#endif

/*
 * The following table is used for encoding and decoding the segments of
 * a cookie based on RFC 6265 (HTTP State Management Mechanism, 2011)
 *
 * A percent-encoding is used to represent a data octet except for
 * characters which are explicitly allowed in the RFC.
 *
 * Allowed cookie characters are defined as:
 *
 *    cookie-octet      = %x21 / %x23-2B / %x2D-3A / %x3C-5B / %x5D-7E
 *                      ; US-ASCII characters excluding CTLs,
 *                      ; whitespace, DQUOTE, comma, semicolon,
 *                      ; and backslash
 *
 * In additions, '%' has to be encoded, because elsewise a raw string
 * "%20" would be encoded as "%20" (no need to encode anything) but
 * decoded as " " (space).
 *
 * This definition implies that a total of 89 characters are allowed
 * unencoded using this scheme:
 *
 *     ! # $ & ' ( ) * + - . / 0 1 2 3 4 5 6 7 8 9 : < = > ? @
 *     A B C D E F G H I J K L M N O P Q R S T U V W X Y Z [ ] ^ _ `
 *     a b c d e f g h i j k l m n o p q r s t u v w x y z { | } ~
 */

static const ByteKey cookie_scheme[] = {
    /* 0x00 */  {3, "00"}, {3, "01"}, {3, "02"}, {3, "03"},
    /* 0x04 */  {3, "04"}, {3, "05"}, {3, "06"}, {3, "07"},
    /* 0x08 */  {3, "08"}, {3, "09"}, {3, "0a"}, {3, "0b"},
    /* 0x0c */  {3, "0c"}, {3, "0d"}, {3, "0e"}, {3, "0f"},
    /* 0x10 */  {3, "10"}, {3, "11"}, {3, "12"}, {3, "13"},
    /* 0x14 */  {3, "14"}, {3, "15"}, {3, "16"}, {3, "17"},
    /* 0x18 */  {3, "18"}, {3, "19"}, {3, "1a"}, {3, "1b"},
    /* 0x1c */  {3, "1c"}, {3, "1d"}, {3, "1e"}, {3, "1f"},
    /* 0x20 */  {3, "20"}, {1, NULL}, {3, "22"}, {1, NULL},
    /* 0x24 */  {1, NULL}, {3, "25"}, {1, NULL}, {1, NULL},
    /* 0x28 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x2c */  {3, "2c"}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x30 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x34 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x38 */  {1, NULL}, {1, NULL}, {1, NULL}, {3, "3b"},
    /* 0x3c */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x40 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x44 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x48 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x4c */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x50 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x54 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x58 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x5c */  {3, "5c"}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x60 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x64 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x68 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x6c */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x70 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x74 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x78 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0x7c */  {1, NULL}, {1, NULL}, {1, NULL}, {3, "7f"},
    /* 0x80 */  {3, "80"}, {3, "81"}, {3, "82"}, {3, "83"},
    /* 0x84 */  {3, "84"}, {3, "85"}, {3, "86"}, {3, "87"},
    /* 0x88 */  {3, "88"}, {3, "89"}, {3, "8a"}, {3, "8b"},
    /* 0x8c */  {3, "8c"}, {3, "8d"}, {3, "8e"}, {3, "8f"},
    /* 0x90 */  {3, "90"}, {3, "91"}, {3, "92"}, {3, "93"},
    /* 0x94 */  {3, "94"}, {3, "95"}, {3, "96"}, {3, "97"},
    /* 0x98 */  {3, "98"}, {3, "99"}, {3, "9a"}, {3, "9b"},
    /* 0x9c */  {3, "9c"}, {3, "9d"}, {3, "9e"}, {3, "9f"},
    /* 0xa0 */  {3, "a0"}, {3, "a1"}, {3, "a2"}, {3, "a3"},
    /* 0xa4 */  {3, "a4"}, {3, "a5"}, {3, "a6"}, {3, "a7"},
    /* 0xa8 */  {3, "a8"}, {3, "a9"}, {3, "aa"}, {3, "ab"},
    /* 0xac */  {3, "ac"}, {3, "ad"}, {3, "ae"}, {3, "af"},
    /* 0xb0 */  {3, "b0"}, {3, "b1"}, {3, "b2"}, {3, "b3"},
    /* 0xb4 */  {3, "b4"}, {3, "b5"}, {3, "b6"}, {3, "b7"},
    /* 0xb8 */  {3, "b8"}, {3, "b9"}, {3, "ba"}, {3, "bb"},
    /* 0xbc */  {3, "bc"}, {3, "bd"}, {3, "be"}, {3, "bf"},
    /* 0xc0 */  {3, "c0"}, {3, "c1"}, {3, "c2"}, {3, "c3"},
    /* 0xc4 */  {3, "c4"}, {3, "c5"}, {3, "c6"}, {3, "c7"},
    /* 0xc8 */  {3, "c8"}, {3, "c9"}, {3, "ca"}, {3, "cb"},
    /* 0xcc */  {3, "cc"}, {3, "cd"}, {3, "ce"}, {3, "cf"},
    /* 0xd0 */  {3, "d0"}, {3, "d1"}, {3, "d2"}, {3, "d3"},
    /* 0xd4 */  {3, "d4"}, {3, "d5"}, {3, "d6"}, {3, "d7"},
    /* 0xd8 */  {3, "d8"}, {3, "d9"}, {3, "da"}, {3, "db"},
    /* 0xdc */  {3, "dc"}, {3, "dd"}, {3, "de"}, {3, "df"},
    /* 0xe0 */  {3, "e0"}, {3, "e1"}, {3, "e2"}, {3, "e3"},
    /* 0xe4 */  {3, "e4"}, {3, "e5"}, {3, "e6"}, {3, "e7"},
    /* 0xe8 */  {3, "e8"}, {3, "e9"}, {3, "ea"}, {3, "eb"},
    /* 0xec */  {3, "ec"}, {3, "ed"}, {3, "ee"}, {3, "ef"},
    /* 0xf0 */  {3, "f0"}, {3, "f1"}, {3, "f2"}, {3, "f3"},
    /* 0xf4 */  {3, "f4"}, {3, "f5"}, {3, "f6"}, {3, "f7"},
    /* 0xf8 */  {3, "f8"}, {3, "f9"}, {3, "fa"}, {3, "fb"},
    /* 0xfc */  {3, "fc"}, {3, "fd"}, {3, "fe"}, {3, "ff"}
};

/*
 * The following table is used for encoding and decoding of oauth tokens
 * as specified in RFC 5849 Section 3.6 for the construction of the
 * signature base string and the "Authorization" header field.
 *
 * A percent-encoding is used to represent a data octet except for
 * characters which are explicitly allowed in the RFC.
 *
 * Allowed oauth1 characters are defined as:
 *
 * Characters in the unreserved character set as defined by
 * [RFC3986], Section 2.3 (ALPHA, DIGIT, "-", ".", "_", "~") MUST
 * NOT be encoded.
 *
 * All other characters MUST be encoded.
 *
 * This definition implies that a total of 66 characters are allowed
 * unencoded using this scheme:
 *
 *     - . 0 1 2 3 4 5 6 7 8 9
 *     A B C D E F G H I J K L M N O P Q R S T U V W X Y Z _
 *     a b c d e f g h i j k l m n o p q r s t u v w x y z ~
 */
static const ByteKey oauth1_scheme[] = {
    /* 0X00 */  {3, "00"}, {3, "01"}, {3, "02"}, {3, "03"},
    /* 0X04 */  {3, "04"}, {3, "05"}, {3, "06"}, {3, "07"},
    /* 0X08 */  {3, "08"}, {3, "09"}, {3, "0A"}, {3, "0B"},
    /* 0X0C */  {3, "0C"}, {3, "0D"}, {3, "0E"}, {3, "0F"},
    /* 0X10 */  {3, "10"}, {3, "11"}, {3, "12"}, {3, "13"},
    /* 0X14 */  {3, "14"}, {3, "15"}, {3, "16"}, {3, "17"},
    /* 0X18 */  {3, "18"}, {3, "19"}, {3, "1A"}, {3, "1B"},
    /* 0X1C */  {3, "1C"}, {3, "1D"}, {3, "1E"}, {3, "1F"},
    /* 0X20 */  {3, "20"}, {3, "21"}, {3, "22"}, {3, "23"},
    /* 0X24 */  {3, "24"}, {3, "25"}, {3, "26"}, {3, "27"},
    /* 0X28 */  {3, "28"}, {3, "29"}, {3, "2A"}, {3, "2B"},
    /* 0X2C */  {3, "2C"}, {1, NULL}, {1, NULL}, {3, "2F"},
    /* 0X30 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0X34 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0X38 */  {1, NULL}, {1, NULL}, {3, "3A"}, {3, "3B"},
    /* 0X3C */  {3, "3C"}, {3, "3D"}, {3, "3E"}, {3, "3F"},
    /* 0X40 */  {3, "40"}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0X44 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0X48 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0X4C */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0X50 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0X54 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0X58 */  {1, NULL}, {1, NULL}, {1, NULL}, {3, "5B"},
    /* 0X5C */  {3, "5C"}, {3, "5D"}, {3, "5E"}, {1, NULL},
    /* 0X60 */  {3, "60"}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0X64 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0X68 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0X6C */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0X70 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0X74 */  {1, NULL}, {1, NULL}, {1, NULL}, {1, NULL},
    /* 0X78 */  {1, NULL}, {1, NULL}, {1, NULL}, {3, "7B"},
    /* 0X7C */  {3, "7C"}, {3, "7D"}, {1, NULL}, {3, "7F"},
    /* 0X80 */  {3, "80"}, {3, "81"}, {3, "82"}, {3, "83"},
    /* 0X84 */  {3, "84"}, {3, "85"}, {3, "86"}, {3, "87"},
    /* 0X88 */  {3, "88"}, {3, "89"}, {3, "8A"}, {3, "8B"},
    /* 0X8C */  {3, "8C"}, {3, "8D"}, {3, "8E"}, {3, "8F"},
    /* 0X90 */  {3, "90"}, {3, "91"}, {3, "92"}, {3, "93"},
    /* 0X94 */  {3, "94"}, {3, "95"}, {3, "96"}, {3, "97"},
    /* 0X98 */  {3, "98"}, {3, "99"}, {3, "9A"}, {3, "9B"},
    /* 0X9C */  {3, "9C"}, {3, "9D"}, {3, "9E"}, {3, "9F"},
    /* 0XA0 */  {3, "A0"}, {3, "A1"}, {3, "A2"}, {3, "A3"},
    /* 0XA4 */  {3, "A4"}, {3, "A5"}, {3, "A6"}, {3, "A7"},
    /* 0XA8 */  {3, "A8"}, {3, "A9"}, {3, "AA"}, {3, "AB"},
    /* 0XAC */  {3, "AC"}, {3, "AD"}, {3, "AE"}, {3, "AF"},
    /* 0XB0 */  {3, "B0"}, {3, "B1"}, {3, "B2"}, {3, "B3"},
    /* 0XB4 */  {3, "B4"}, {3, "B5"}, {3, "B6"}, {3, "B7"},
    /* 0XB8 */  {3, "B8"}, {3, "B9"}, {3, "BA"}, {3, "BB"},
    /* 0XBC */  {3, "BC"}, {3, "BD"}, {3, "BE"}, {3, "BF"},
    /* 0XC0 */  {3, "C0"}, {3, "C1"}, {3, "C2"}, {3, "C3"},
    /* 0XC4 */  {3, "C4"}, {3, "C5"}, {3, "C6"}, {3, "C7"},
    /* 0XC8 */  {3, "C8"}, {3, "C9"}, {3, "CA"}, {3, "CB"},
    /* 0XCC */  {3, "CC"}, {3, "CD"}, {3, "CE"}, {3, "CF"},
    /* 0XD0 */  {3, "D0"}, {3, "D1"}, {3, "D2"}, {3, "D3"},
    /* 0XD4 */  {3, "D4"}, {3, "D5"}, {3, "D6"}, {3, "D7"},
    /* 0XD8 */  {3, "D8"}, {3, "D9"}, {3, "DA"}, {3, "DB"},
    /* 0XDC */  {3, "DC"}, {3, "DD"}, {3, "DE"}, {3, "DF"},
    /* 0XE0 */  {3, "E0"}, {3, "E1"}, {3, "E2"}, {3, "E3"},
    /* 0XE4 */  {3, "E4"}, {3, "E5"}, {3, "E6"}, {3, "E7"},
    /* 0XE8 */  {3, "E8"}, {3, "E9"}, {3, "EA"}, {3, "EB"},
    /* 0XEC */  {3, "EC"}, {3, "ED"}, {3, "EE"}, {3, "EF"},
    /* 0XF0 */  {3, "F0"}, {3, "F1"}, {3, "F2"}, {3, "F3"},
    /* 0XF4 */  {3, "F4"}, {3, "F5"}, {3, "F6"}, {3, "F7"},
    /* 0XF8 */  {3, "F8"}, {3, "F9"}, {3, "FA"}, {3, "FB"},
    /* 0XFC */  {3, "FC"}, {3, "FD"}, {3, "FE"}, {3, "FF"}
};


/*
 *----------------------------------------------------------------------
 *
 * Ns_UrlEncodingWarnUnencoded --
 *
 *      Heuristic to warn about unencoded characters in a URL string.
 *      This function warns only about characters that have to be
 *      encoded always in the path and query component.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Produces potentially warnings in the system log file.
 *
 *----------------------------------------------------------------------
 */

void
Ns_UrlEncodingWarnUnencoded(const char *msg, const char *inputStr)
{
    static bool initialized = NS_FALSE;
    static bool mustBeEncoded[256];
    size_t charLength;
    size_t i;

    NS_NONNULL_ASSERT(msg != NULL);
    NS_NONNULL_ASSERT(inputStr != NULL);

    if (!initialized) {
        /*
         * No need for a fine-grained lock.
         */
        Ns_MasterLock();
        for (i = 0u; i < 256u; i++) {
            mustBeEncoded[i] = NS_TRUE;
        }

        /*
         * Don't try to distinguish for now between percents in
         * pct-encoded input string and literal percents (same with '='
         * and ';').
         */
        mustBeEncoded[UCHAR('%')] = NS_FALSE;
        mustBeEncoded[UCHAR('=')] = NS_FALSE;
        mustBeEncoded[UCHAR(';')] = NS_FALSE;

        /*
         * Don't warn about begin of fragment identifier. We would need
         * a detailed URL parser to detect its usage).
         */
        mustBeEncoded[UCHAR('#')] = NS_FALSE;

        for (i = 0u; i < 256u; i++) {
            if (path_scheme[i].str == NULL) {
                mustBeEncoded[i] = NS_FALSE;
            }
            if (query_scheme[i].str == NULL) {
                mustBeEncoded[i] = NS_FALSE;
            }
        }
        initialized = NS_TRUE;
        Ns_MasterUnlock();
    }

    charLength = strlen(inputStr);
    for (i = 0u; i < charLength; i++) {
        if (mustBeEncoded[UCHAR(inputStr[i])]) {
            Ns_Log(Warning, "%s value '%s': byte with binary value 0x%.2x must be URL-encoded",
                   msg, inputStr, UCHAR(inputStr[i]));
            /*
             * Just warn about the first invalid character
             */
            break;
        }
    }
}



/*
 *----------------------------------------------------------------------
 *
 * Ns_GetUrlEncoding --
 *
 *      Get the encoding to use for Ns_UrlQueryDecode and related
 *      routines.  The encoding is determined by the following sequence:
 *
 *      - charset parameter
 *      - connection->urlEncoding
 *      - config parameter urlEncoding
 *      - static default
 *
 * Results:
 *      A Tcl_Encoding.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Encoding
Ns_GetUrlEncoding(const char *charset)
{
    Tcl_Encoding  encoding = NULL;

    if (charset != NULL) {
        encoding = Ns_GetCharsetEncoding(charset);
        if (encoding == NULL) {
            Ns_Log(Warning, "no encoding found for charset \"%s\"", charset);
        }
    }

    /*
     * The conn urlEncoding field is initialized from the config default
     * URL encoding.  This implements the fallback described above in
     * a single step.
     */

    if (encoding == NULL) {
        const Conn *connPtr = (const Conn *) Ns_GetConn();

        if (connPtr != NULL) {
            encoding = connPtr->urlEncoding;
        } else {
            /* In the current code, the URL is decoded via
               UrlPathDecode() *before* the NsConnThread() is started, so
               connPtr will be normally NULL.

               We need here an appropriate encoding to decode the
               URL. It would be nice to do here:

                 NsServer   *servPtr = NsGetServer(server);
                 return servPtr->encoding.urlEncoding;

               However, "server" is not available here.  Reading
               values from the configuration file would require "server" as
               well.

               Unfortunately, the general default for encoding opens a
               door for a path traversal attack with (invalid)
               UTF-8 characters.  For example, ".." can be encoded via
               UTF-8 in a URL as "%c0%ae%c0%ae" or
               "%e0%80%ae%e0%80%ae" and many more forms, so the
               literal checks against path traversal based on
               character data (here in Ns_NormalizePath()) fail. As a
               consequence, it would be possible to retrieve
               e.g. /etc/passwd from NaviServer. For more details,
               see Section "Canonicalization" in

               http://www.cgisecurity.com/owasp/html/ch11s03.html

               A simple approach to handle this attack is to fall back
               to utf-8 encodings and let Tcl do the UTF-8
               canonicalization.

               -gustaf neumann
            */
            encoding = NS_utf8Encoding;
        }
    }

    return encoding;
}


/*
 *----------------------------------------------------------------------
 *
 * Ns_UrlPathEncode, Ns_UrlPathDecode --
 *
 *      Encode/decode the given segment of URI path component.
 *      If encoding is NULL, UTF8 is assumed.
 *
 * Results:
 *      A pointer to the dstring's value, containing the transformed
 *      path component.
 *
 * Side effects:
 *      Transformed path component will be copied to given dstring.
 *
 *----------------------------------------------------------------------
 */

char *
Ns_UrlPathEncode(Tcl_DString *dsPtr, const char *urlSegment,
                 Tcl_Encoding encoding)
{
    NS_NONNULL_ASSERT(dsPtr != NULL);
    NS_NONNULL_ASSERT(urlSegment != NULL);

    return UrlEncode(dsPtr, urlSegment, encoding, 'p', NS_FALSE);
}

char *
Ns_UrlPathDecode(Tcl_DString *dsPtr, const char *urlSegment,
                 Tcl_Encoding encoding)
{
    NS_NONNULL_ASSERT(dsPtr != NULL);
    NS_NONNULL_ASSERT(urlSegment != NULL);

    return UrlDecode(dsPtr, urlSegment, encoding, 'p', NULL);
}


/*
 *----------------------------------------------------------------------
 *
 * Ns_UrlQueryEncode, Ns_UrlQueryDecode --
 *
 *      Encode/decode the given segment of URI query component.
 *      If encoding is NULL, UTF8 is assumed.
 *
 * Results:
 *      A pointer to the Tcl_DString's value, containing the transformed
 *      query string component.
 *
 * Side effects:
 *      Transformed query string component will be copied to given
 *      dstring.
 *
 *----------------------------------------------------------------------
 */

char *
Ns_UrlQueryEncode(Tcl_DString *dsPtr, const char *urlSegment,
                  Tcl_Encoding encoding)
{
    NS_NONNULL_ASSERT(dsPtr != NULL);
    NS_NONNULL_ASSERT(urlSegment != NULL);

    return UrlEncode(dsPtr, urlSegment, encoding, 'q', NS_FALSE);
}

char *
Ns_UrlQueryDecode(Tcl_DString *dsPtr, const char *urlSegment,
                  Tcl_Encoding encoding, Ns_ReturnCode *resultPtr)
{
    NS_NONNULL_ASSERT(dsPtr != NULL);
    NS_NONNULL_ASSERT(urlSegment != NULL);

    return UrlDecode(dsPtr, urlSegment, encoding, 'q', resultPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Ns_CookieEncode, Ns_CookieDecode --
 *
 *      Encode/decode the given string in cookie encoding.
 *      If encoding is NULL, UTF8 is assumed.
 *
 * Results:
 *      A pointer to the dstring's value, containing the encoded/decoded
 *      string component.
 *
 * Side effects:
 *      Transformed query string component will be copied to given
 *      dstring.
 *
 *----------------------------------------------------------------------
 */

char *
Ns_CookieEncode(Tcl_DString *dsPtr, const char *cookie, Tcl_Encoding encoding)
{
    NS_NONNULL_ASSERT(dsPtr != NULL);
    NS_NONNULL_ASSERT(cookie != NULL);

#ifdef RFC1738
    return UrlEncode(dsPtr, cookie, encoding, 'q', NS_FALSE);
#else
    return UrlEncode(dsPtr, cookie, encoding, 'c', NS_FALSE);
#endif
}

char *
Ns_CookieDecode(Tcl_DString *dsPtr, const char *cookie, Tcl_Encoding encoding)
{
    NS_NONNULL_ASSERT(dsPtr != NULL);
    NS_NONNULL_ASSERT(cookie != NULL);

#ifdef RFC1738
    return UrlDecode(dsPtr, cookie, encoding, 'q', NULL);
#else
    return UrlDecode(dsPtr, cookie, encoding, 'c', NULL);
#endif
}

char *
Ns_Oauth1Encode(Tcl_DString *dsPtr, const char *cookie, Tcl_Encoding encoding)
{
    NS_NONNULL_ASSERT(dsPtr != NULL);
    NS_NONNULL_ASSERT(cookie != NULL);

    return UrlEncode(dsPtr, cookie, encoding, 'o', NS_FALSE);
}

char *
Ns_Oauth1Decode(Tcl_DString *dsPtr, const char *cookie, Tcl_Encoding encoding)
{
    NS_NONNULL_ASSERT(dsPtr != NULL);
    NS_NONNULL_ASSERT(cookie != NULL);

    return UrlDecode(dsPtr, cookie, encoding, 'o', NULL);

}

#ifdef NS_WITH_DEPRECATED

/*
 *----------------------------------------------------------------------
 *
 * Ns_EncodeUrlWithEncoding, Ns_EncodeUrlCharset,
 * Ns_DecodeUrlWithEncoding, Ns_DecodeUrlCharset --
 *
 *      Deprecated.
 *
 * Results:
 *      A pointer to the transformed urlSegment (which is part of the
 *      passed-in DString's memory)
 *
 * Side effects:
 *      transformed input will be copied to given dstring.
 *
 *----------------------------------------------------------------------
 */

char *
Ns_EncodeUrlWithEncoding(Tcl_DString *dsPtr, const char *urlSegment,
                         Tcl_Encoding encoding)
{
    NS_NONNULL_ASSERT(dsPtr != NULL);
    NS_NONNULL_ASSERT(urlSegment != NULL);

    return Ns_UrlQueryEncode(dsPtr, urlSegment, encoding);
}

char *
Ns_EncodeUrlCharset(Tcl_DString *dsPtr, const char *urlSegment,
                    const char *charset)
{
    Tcl_Encoding encoding = Ns_GetUrlEncoding(charset);

    NS_NONNULL_ASSERT(dsPtr != NULL);
    NS_NONNULL_ASSERT(urlSegment != NULL);

    return Ns_UrlQueryEncode(dsPtr, urlSegment, encoding);
}

char *
Ns_DecodeUrlWithEncoding(Tcl_DString *dsPtr, const char *urlSegment,
                         Tcl_Encoding encoding)
{
    NS_NONNULL_ASSERT(dsPtr != NULL);
    NS_NONNULL_ASSERT(urlSegment != NULL);

    return Ns_UrlQueryDecode(dsPtr, urlSegment, encoding, NULL);
}

char *
Ns_DecodeUrlCharset(Tcl_DString *dsPtr, const char *urlSegment,
                    const char *charset)
{
    Tcl_Encoding encoding = Ns_GetUrlEncoding(charset);

    NS_NONNULL_ASSERT(dsPtr != NULL);
    NS_NONNULL_ASSERT(urlSegment != NULL);

    return Ns_UrlQueryDecode(dsPtr, urlSegment, encoding, NULL);
}
#endif


/*
 *----------------------------------------------------------------------
 *
 * NsTclUrlEncodeObjCmd --
 *
 *      Implements "ns_urlencode".
 *
 *      Encodes one or more segments of a either a URI path or query
 *      component part.  If the part is not specified, query is assumed.
 *      Segments are joined with a separator according to part.
 *
 *      NB: Path component param sections are not supported -- the ';'
 *      and '=' characters are encoded.  This is a relatively little used
 *      feature.
 *
 * Results:
 *      Tcl result.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */


int
NsTclUrlEncodeObjCmd(ClientData UNUSED(clientData), Tcl_Interp *interp,
                     TCL_SIZE_T objc, Tcl_Obj *const* objv)
{
    int          upperCase = 0, result = TCL_OK, percentScheme = INTCHAR('q');
    TCL_SIZE_T   nargs = 0;
    char        *charset = NULL;
    Ns_ObjvSpec lopts[] = {
        {"-charset",   Ns_ObjvString, &charset,       NULL},
        {"-part",      Ns_ObjvIndex,  &percentScheme, percentSchemes},
        {"-uppercase", Ns_ObjvBool,   &upperCase,     INT2PTR(NS_TRUE)},
        {"--",         Ns_ObjvBreak,  NULL,           NULL},
        {NULL, NULL, NULL, NULL}
    };
    Ns_ObjvSpec args[] = {
        {"component",  Ns_ObjvArgs, &nargs, NULL},
        {NULL, NULL, NULL, NULL}
    };

    if (Ns_ParseObjv(lopts, args, interp, 1, objc, objv) != NS_OK) {
        result = TCL_ERROR;
    } else {
        Tcl_DString  ds;
        Tcl_Encoding encoding = NULL;
        TCL_SIZE_T   i;

        if (charset != NULL) {
            encoding = Ns_GetCharsetEncoding(charset);
        }

        Tcl_DStringInit(&ds);
        for (i = (TCL_SIZE_T)objc - nargs; i < (TCL_SIZE_T)objc; ++i) {
            (void)UrlEncode(&ds, Tcl_GetString(objv[i]), encoding, (char)percentScheme, (upperCase == 1));

            if (i + 1 < (TCL_SIZE_T)objc) {
                if (percentScheme == 'q') {
                    Tcl_DStringAppend(&ds, "&", 1);
                } else {
                    Tcl_DStringAppend(&ds, "/", 1);
                }
            }
        }
        Tcl_DStringResult(interp, &ds);
    }

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * UrlFallbackDecode --
 *
 *      Attempt to decode a percent-encoded URL string using an initial
 *      character encoding, and if that fails due to an invalid
 *      encoding, retry with a fallback charset (if provided).
 *
 * Results:
 *      Returns NS_OK if decoding succeeds (either on the first try or after
 *      falling back); returns an error code (e.g. NS_ERROR) if both attempts
 *      fail or if no valid fallback encoding is found.
 *
 * Side effects:
 *      May free and re‑initialize dsPtr during the fallback attempt.
 *
 *----------------------------------------------------------------------
 */
static Ns_ReturnCode
UrlFallbackDecode(NsInterp *itPtr, Tcl_DString *dsPtr, const char *inputStr,
                  Tcl_Obj *fallbackCharsetObj, Tcl_Encoding initialEncoding, char percentScheme)
{
    Tcl_Encoding  fallbackEncoding = NULL;
    Ns_ReturnCode status, result = NS_ERROR;

    assert(itPtr != NULL);
    if (itPtr->servPtr == NULL) {
        Ns_Log(Debug, "percent decode: no servPtr for fallbackEncoding available");
    }
    status = NsGetFallbackEncoding(itPtr->interp, itPtr->servPtr,
                                   fallbackCharsetObj, NS_FALSE, &fallbackEncoding);

    if (status == NS_OK
        && fallbackEncoding != NULL
        && fallbackEncoding != initialEncoding
        ) {
        Tcl_DStringFree(dsPtr);
        Tcl_DStringInit(dsPtr);

        Ns_Log(Debug, "percent decode: retry decoding with encoding %s",
               Ns_GetEncodingCharset(fallbackEncoding));
        (void)UrlDecode(dsPtr, inputStr, fallbackEncoding, percentScheme, &result);
        Ns_Log(Debug, "percent decode: retry decoding ends with status %s '%s'",
               Ns_ReturnCodeString(status), dsPtr->string);
    }

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * UrlPercentDecode --
 *
 *      Decode a percent‑encoded URL string into UTF‑8 using a primary
 *      charset (if provided) or the server’s default URL encoding. If the
 *      decode fails due to an invalid encoding and no explicit charset was
 *      given, attempt to retry with a fallback charset.
 *
 * Results:
 *      Returns TCL_OK and sets the Tcl result to the decoded UTF‑8 string
 *      if successful. On failure, returns TCL_ERROR, sets an error message
 *      and error code "NS_INVALID_UTF8" in the interpreter.
 *
 * Side effects:
 *      - Allocates and frees a Tcl_DString during decoding.
 *      - May invoke UrlFallbackDecode for a second decode attempt.
 *
 *----------------------------------------------------------------------
 */
static int
UrlPercentDecode(NsInterp *itPtr, const char *inputStr, char percentScheme, const char *charset,
                 Tcl_Obj *fallbackCharsetObj)
{
    Tcl_DString   ds;
    Tcl_Encoding  encoding;
    int           result = TCL_OK;
    Ns_ReturnCode status;

    Tcl_DStringInit(&ds);
    if (charset != NULL) {
        encoding = Ns_GetCharsetEncoding(charset);
    } else {
        encoding = Ns_GetUrlEncoding(NULL);
    }

    (void)UrlDecode(&ds, inputStr, encoding, percentScheme, &status);
    if (status == NS_OK) {
        Tcl_DStringResult(itPtr->interp, &ds);
    } else if (charset == NULL) {
        /*
         * Decoding failed. UrlDecode() returns NS_ERROR only on
         * invalid encodings. Retry with fallbackCharset if
         * specified.
         */
        status = UrlFallbackDecode(itPtr, &ds, inputStr,
                                   fallbackCharsetObj, encoding, percentScheme);
        if (status == NS_OK) {
            Tcl_DStringResult(itPtr->interp, &ds);
        }
    }
    if (status != NS_OK) {
        Ns_TclPrintfResult(itPtr->interp, "input string '%s' cannot be converted to UTF-8",
                           inputStr);
        Tcl_SetErrorCode(itPtr->interp, "NS_INVALID_UTF8", NULL);
        result = TCL_ERROR;
        Tcl_DStringFree(&ds);
    }
    return result;
}


/*
 *----------------------------------------------------------------------
 *
 * NsTclUrlDecodeObjCmd --
 *
 *      Implements "ns_urldecode".
 *
 *      Decode a component of either a URL path or query.  If the percentScheme
 *      is not specified, "query" is assumed.
 *
 * Results:
 *      Tcl result.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
int
NsTclUrlDecodeObjCmd(ClientData clientData, Tcl_Interp *interp,
                     TCL_SIZE_T objc, Tcl_Obj *const* objv)
{
    NsInterp    *itPtr = clientData;
    int          result = TCL_OK, percentScheme = INTCHAR('q');
    char        *charset = NULL, *inputStr = (char *)NS_EMPTY_STRING;
    Tcl_Obj     *fallbackCharsetObj = NULL;
    Ns_ObjvSpec  lopts[] = {
        {"-charset", Ns_ObjvString, &charset, NULL},
        {"-fallbackcharset", Ns_ObjvObj, &fallbackCharsetObj, NULL},
        {"-part",    Ns_ObjvIndex,  &percentScheme, percentSchemes},
        {"--",       Ns_ObjvBreak,  NULL,     NULL},
        {NULL, NULL, NULL, NULL}
    };
    Ns_ObjvSpec  args[] = {
        {"string", Ns_ObjvString, &inputStr, NULL},
        {NULL, NULL, NULL, NULL}
    };

    if (Ns_ParseObjv(lopts, args, interp, 1, objc, objv) != NS_OK) {
        result = TCL_ERROR;
    } else {
        result = UrlPercentDecode(itPtr, inputStr, (char)percentScheme, charset, fallbackCharsetObj);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * NsTclPercentEncodeObjCmd --
 *
 *      Implements the "ns_percentencode" Tcl command.
 *
 *      Percent‑encodes the given input string and returns the result.
 *      By default, the query‑style encoding scheme is used:
 *          - Spaces become '+'.
 *          - Unsafe characters are replaced with "%xx" escapes.
 *
 *      Options:
 *        -charset <name>    : interpret the input string using the named
 *                             character set before encoding (defaults to UTF‑8)
 *        -scheme  <char>    : choose the percent‑encoding scheme
 *                             ('q' for query, 'p' for path, etc.)
 *        -uppercase         : emit hexadecimal digits in uppercase (e.g. "%2F" instead of "%2f")
 *
 * Results:
 *      Tcl result.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
int
NsTclPercentEncodeObjCmd(ClientData UNUSED(clientData), Tcl_Interp *interp,
                         TCL_SIZE_T objc, Tcl_Obj *const* objv)
{
    int          upperCase = 0, result = TCL_OK, percentScheme = INTCHAR('q');
    char        *charset = NULL, *string;
    Ns_ObjvSpec lopts[] = {
        {"-charset",   Ns_ObjvString, &charset,       NULL},
        {"-scheme",    Ns_ObjvIndex,  &percentScheme, percentSchemes},
        {"-uppercase", Ns_ObjvBool,   &upperCase,     INT2PTR(NS_TRUE)},
        {"--",         Ns_ObjvBreak,  NULL,           NULL},
        {NULL, NULL, NULL, NULL}
    };
    Ns_ObjvSpec args[] = {
        {"string",  Ns_ObjvString, &string, NULL},
        {NULL, NULL, NULL, NULL}
    };

    if (Ns_ParseObjv(lopts, args, interp, 1, objc, objv) != NS_OK) {
        result = TCL_ERROR;
    } else {
        Tcl_DString  ds;
        Tcl_Encoding encoding = NULL;

        if (charset != NULL) {
            encoding = Ns_GetCharsetEncoding(charset);
        }

        Tcl_DStringInit(&ds);
        (void)UrlEncode(&ds, string, encoding, (char)percentScheme, (upperCase == 1));
        Tcl_DStringResult(interp, &ds);
    }

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * NsTclPercentDecodeObjCmd --
 *
 *      Implements "ns_percentdecode".
 *
 *      Decode a percent-encoded input string.  If the percent scheme
 *      is not specified, "query" is assumed.
 *
 * Results:
 *      Tcl result.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
int
NsTclPercentDecodeObjCmd(ClientData clientData, Tcl_Interp *interp,
                         TCL_SIZE_T objc, Tcl_Obj *const* objv)
{
    NsInterp    *itPtr = clientData;
    int          result = TCL_OK, percentScheme = INTCHAR('q');
    char        *charset = NULL, *inputStr = (char *)NS_EMPTY_STRING;
    Tcl_Obj     *fallbackCharsetObj = NULL;
    Ns_ObjvSpec  lopts[] = {
        {"-charset",         Ns_ObjvString, &charset,            NULL},
        {"-fallbackcharset", Ns_ObjvObj,    &fallbackCharsetObj, NULL},
        {"-scheme",          Ns_ObjvIndex,  &percentScheme,      percentSchemes},
        {"--",               Ns_ObjvBreak,  NULL,                NULL},
        {NULL, NULL, NULL, NULL}
    };
    Ns_ObjvSpec  args[] = {
        {"string", Ns_ObjvString, &inputStr, NULL},
        {NULL, NULL, NULL, NULL}
    };

    if (Ns_ParseObjv(lopts, args, interp, 1, objc, objv) != NS_OK) {
        result = TCL_ERROR;
    } else {
        result = UrlPercentDecode(itPtr, inputStr, (char)percentScheme, charset, fallbackCharsetObj);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * UrlEncode --
 *
 *      Encode the given URL component according to percentScheme.
 *
 * Results:
 *      A pointer to the encoded string (which is part of the
 *      passed-in DString's memory)
 *
 * Side effects:
 *      Encoded URL component will be copied to given dstring.
 *
 *----------------------------------------------------------------------
 */

static char *
UrlEncode(Tcl_DString *dsPtr, const char *urlSegment, Tcl_Encoding encoding,
          char percentScheme, bool upperCase)
{
    TCL_SIZE_T     i, n;
    register char *q;
    const char    *p;
    Tcl_DString    ds;
    const ByteKey *enc;

    NS_NONNULL_ASSERT(dsPtr != NULL);
    NS_NONNULL_ASSERT(urlSegment != NULL);


    if (encoding != NULL) {
        urlSegment = Tcl_UtfToExternalDString(encoding, urlSegment, TCL_INDEX_NONE, &ds);
    }

    /*
     * Get the encoding table
     */
    switch (percentScheme) {
    case 'q': enc = query_scheme; break;
    case 'p': enc = path_scheme; break;
    case 'c': enc = cookie_scheme; break;
    case 'o': enc = oauth1_scheme; break;
    default:  enc = query_scheme; break;
    }

    /*
     * Save old dstring length, determine and set the full required
     * dstring length.
     */
    i = dsPtr->length;
    n = 0;
    for (p = urlSegment; *p != '\0'; p++) {
        n += enc[UCHAR(*p)].len;
    }
    Tcl_DStringSetLength(dsPtr, dsPtr->length + n);

    /*
     * Copy the result directly to the pre-sized dstring.
     */

    q = dsPtr->string + i;
    for (p = urlSegment; *p != '\0'; p++) {
        if ((unlikely (*p == ' ' && percentScheme == 'q'))) {
            *q++ = '+';
        } else if (enc[UCHAR(*p)].str == NULL) {
            *q++ = *p;
        } else {
            char c1 = enc[UCHAR(*p)].str[0];
            char c2 = enc[UCHAR(*p)].str[1];

            if (upperCase) {
                if (c1 >= 'a' && c1 <= 'f') {
                    c1 = CHARCONV(upper, c1);
                }
                if (c2 >= 'a' && c2 <= 'f') {
                    c2 = CHARCONV(upper, c2);
                }
            }
            *q++ = '%';
            *q++ = c1;
            *q++ = c2;
        }
    }

    if (encoding != NULL) {
        Tcl_DStringFree(&ds);
    }

    return dsPtr->string;
}


/*
 *----------------------------------------------------------------------
 *
 * PercentDecode --
 *
 *      Helper function of UrlDecode(), which performs the actual
 *      decoding. It assumes a large enough buffer in 'dest' and returns
 *      the number of decoded characters.
 *
 * Results:
 *      Number of decoded characters.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
static TCL_SIZE_T
PercentDecode(char *dest, const char *source, char percentScheme)
{
    register char       *q = dest;
    register const char *p = source;
    register TCL_SIZE_T  n = 0;
    static const int hex_code[] = {
        /* 0x00 */  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        /* 0x10 */  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        /* 0x20 */  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        /* 0x30 */   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
        /* 0x40 */  -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        /* 0x50 */  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        /* 0x60 */  -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        /* 0x70 */  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        /* 0x80 */  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        /* 0x90 */  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        /* 0xa0 */  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        /* 0xb0 */  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        /* 0xc0 */  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        /* 0xd0 */  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        /* 0xe0 */  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        /* 0xf0 */  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
    };

    while (likely(*p != '\0')) {
        int  i, j;
        char c1, c2 = '\0';

        if (unlikely(p[0] == '%')) {
            /*
             * Decode percent code and make sure not to read date after
             * the NUL character.
             */
            c1 = p[1];
            if (c1 != '\0') {
                c2 = p[2];
            }
        } else {
            c1 = '\0';
        }

        /*
         * When c2 != '\0', hex conversion is possible
         */
        if (c2 != '\0'
            && (i = hex_code[UCHAR(c1)]) >= 0
            && (j = hex_code[UCHAR(c2)]) >= 0) {
            *q++ = (char)(UCHAR(UCHAR(i) << 4u) + UCHAR(j));
            p += 3;
        } else if (unlikely(p[0] == '+') && percentScheme == 'q') {
            *q++ = ' ';
            p++;
        } else {
            *q++ = *p++;
        }
        n++;
    }
    /*
     * Ensure the resulting string is null-terminated.
     */
    *q = '\0';

    return n;
}


/*
 *----------------------------------------------------------------------
 *
 * UrlDecode --
 *
 *      Decode the given URL component according to percentScheme.
 *
 * Results:
 *      A pointer to the Tcl_DString's value, containing the decoded
 *      URL. In addition, an Ns_ReturnCode is placed into resultPtr.
 *      NS_ERROR is returned on encoding failures (invalid UTF-8).
 *
 * Side effects:
 *      Decoded URL will be copied to given Tcl_DString.
 *
 *----------------------------------------------------------------------
 */

static char *
UrlDecode(Tcl_DString *dsPtr, const char *urlSegment, Tcl_Encoding encoding,
          char percentScheme, Ns_ReturnCode *resultPtr)
{
    const char      *firstCode;
    size_t           inputLength;
    Ns_ReturnCode    result = NS_OK;

    NS_NONNULL_ASSERT(dsPtr != NULL);
    NS_NONNULL_ASSERT(urlSegment != NULL);

    /*
     * The size of the hex-decoded string is less or equal the size of
     * the encoded string.  Get the size of the input string.
     */
    inputLength = strlen(urlSegment);

    /*
     * Check, if character decoding is necessary (i.e. input string
     * contains '%' or '+'.
     */
    firstCode = strpbrk(urlSegment, "%+");

    if (likely(firstCode == NULL)
        && (likely(encoding == NS_utf8Encoding) || (encoding == NULL) )
        ) {
        /*
         * There is no decoding character in the input string (this is
         * quite common for e.g. paths) and there is no change in the
         * encoding required (the required encoded == utf-8 == system
         * encoding). This optimization improves this function roughly
         * 2x.
         */
        Tcl_DStringAppend(dsPtr, urlSegment, (TCL_SIZE_T)inputLength);
        //Ns_Log(Notice, "### UrlDecode plain append <%s> len %ld", dsPtr->string, inputLength);
    } else {
        TCL_SIZE_T oldLength, decodedLength;
        char *decoded;

        oldLength = dsPtr->length;

        /*
         * Expand the Tcl_DString by the length of the input
         * string which will be the largest size required.
         */
        Tcl_DStringSetLength(dsPtr, oldLength + (TCL_SIZE_T)inputLength);
        decoded = dsPtr->string + oldLength;

        if (firstCode != NULL) {
            ptrdiff_t offset = firstCode - urlSegment;

            memcpy(decoded, urlSegment, (size_t)offset);
            decodedLength = (TCL_SIZE_T)offset;
            dsPtr->length += decodedLength;
            decodedLength += PercentDecode(decoded+offset, urlSegment+offset, percentScheme);
        } else {
            memcpy(decoded, urlSegment, inputLength);
            decodedLength = (TCL_SIZE_T)inputLength;
        }

        if (likely(encoding != NULL)) {
            bool        validByteSequence = NS_TRUE;
            Tcl_DString messageDs;

            if (encoding == NS_utf8Encoding) {
                validByteSequence = Ns_Valid_UTF8((const unsigned char *)decoded, (size_t)decodedLength, &messageDs);
            }
            Ns_Log(Debug, "### UrlDecode external '%s' encoding %s valid %d", decoded,
                   Tcl_GetEncodingName(encoding), validByteSequence);

            if (validByteSequence) {
                Tcl_DString encodedDs;

                (void)Tcl_ExternalToUtfDString(encoding, decoded, decodedLength, &encodedDs);
                Tcl_DStringSetLength(dsPtr, oldLength);
                Tcl_DStringAppend(dsPtr, encodedDs.string, encodedDs.length);
                Tcl_DStringFree(&encodedDs);
            } else {
                /*
                 * The input byte sequence is not valid. Here, we have
                 * the invalid UTF-8 sequence at hand. We do not want to
                 * loose this information about the bad byte
                 * sequence. Since we have no interp in the API, we
                 * cannot return it in the error message. Therefore, we
                 * provide a warning message.
                 */

                Ns_Log(Warning, "decoded string contains invalid UTF-8: '%s'", messageDs.string);
                Tcl_DStringFree(&messageDs);
                Tcl_DStringSetLength(dsPtr, oldLength);

                result = NS_ERROR;
            }
            Ns_Log(Debug, "### UrlDecode utf8     '%s'", dsPtr->string);

        } else {
            /*
             * The length of dsPtr is now (oldLength + inputLength);
             * adjust the length to (oldLength + decodedLength), which
             * might be less.
             */
            Tcl_DStringSetLength(dsPtr, (oldLength + decodedLength));
        }
    }
    if (resultPtr != NULL) {
        *resultPtr = result;
    }
    return dsPtr->string;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 72
 * indent-tabs-mode: nil
 * End:
 */
