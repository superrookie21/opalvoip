/*
 * $Id: cypher.cxx,v 1.23 1998/02/16 00:14:36 robertj Exp $
 *
 * Portable Windows Library
 *
 * User Interface Classes Interface Declarations
 *
 * Copyright 1993 Equivalence
 *
 * $Log: cypher.cxx,v $
 * Revision 1.23  1998/02/16 00:14:36  robertj
 * Fixed ability to register in one stage instead of always having to use 2.
 *
 * Revision 1.22  1998/01/26 02:49:14  robertj
 * GNU support.
 *
 * Revision 1.21  1997/10/30 10:19:19  robertj
 * Fixed bug with having empty string in encrypted text.
 *
 * Revision 1.20  1997/10/10 10:43:41  robertj
 * Fixed bug in password encryption, missing string terminator.
 *
 * Revision 1.19  1997/08/04 10:39:53  robertj
 * Fixed bug for decoding empty string.
 *
 * Revision 1.18  1997/07/26 11:35:38  robertj
 * Fixed bug where illegal data errors were not propagated.
 *
 * Revision 1.17  1996/11/16 10:50:26  robertj
 * ??
 *
 * Revision 1.16  1996/08/17 09:56:02  robertj
 * Fixed big endian processor platform conformance.
 *
 * Revision 1.15  1996/07/15 10:33:42  robertj
 * Changed memory block base64 conversion functions to be void *.
 * Changed memory block cypher conversion functions to be void *.
 * Changed endian classes to be memory mapped.
 *
 * Revision 1.14  1996/06/18 12:35:49  robertj
 * Fixed bug in registration when language is not English.
 *
 * Revision 1.13  1996/06/10 10:01:23  robertj
 * Fixed bug in getting cypher key, not copying all the bytes.
 *
 * Revision 1.12  1996/05/26 03:46:31  robertj
 * Compatibility to GNU 2.7.x
 *
 * Revision 1.11  1996/04/09 03:32:45  robertj
 * Fixed bug in registration so now works in time zones other than Eastern Australia.
 *
 * Revision 1.11  1996/04/08 05:18:38  robertj
 * Fixed bug in registering programs in a different time zone.
 *
 * Revision 1.10  1996/03/17 05:47:19  robertj
 * Changed secured config to allow for expiry dates.
 *
 * Revision 1.9  1996/03/16 04:37:20  robertj
 * Redesign of secure config to accommodate expiry dates and option values passed in security key code.
 *
 * Revision 1.8  1996/03/11 10:28:53  robertj
 * Fixed bug in C++ optimising compiler.
 *
 * Revision 1.7  1996/03/02 03:20:52  robertj
 * Fixed secured config parameters so leading/trailing blanks not significant.
 *
 * Revision 1.6  1996/02/25 11:22:42  robertj
 * Added assertion if try and SetValidation when not pending.
 *
 * Revision 1.5  1996/02/25 02:53:05  robertj
 * Further secure config development.
 *
 * Revision 1.4  1996/02/15 14:43:28  robertj
 * Allowed no secured config data at all to be "valid". All vars will then be guarenteed to default.
 *
 * Revision 1.3  1996/01/28 14:14:12  robertj
 * Further implementation of secure config.
 *
 * Revision 1.2  1996/01/28 02:49:00  robertj
 * Removal of MemoryPointer classes as usage didn't work for GNU.
 * Added the secure configuration mechanism for protecting applications.
 *
 * Revision 1.1  1996/01/23 13:05:58  robertj
 * Initial revision
 *
 */

#ifdef __GNUC__
#pragma implementation "cypher.h"
#endif

#include <ptlib.h>
#include <cypher.h>
#include <mime.h>


///////////////////////////////////////////////////////////////////////////////
// PMessageDigest5

PMessageDigest5::PMessageDigest5()
{
  Start();
}


// Constants for MD5Transform routine.
#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

// F, G, H and I are basic MD5 functions.
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

// ROTATE_LEFT rotates x left n bits.
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

// FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
// Rotation is separate from addition to prevent recomputation.
#define FF(a, b, c, d, x, s, ac) \
 (a) += F ((b), (c), (d)) + (x) + (DWORD)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \

#define GG(a, b, c, d, x, s, ac) \
 (a) += G ((b), (c), (d)) + (x) + (DWORD)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \

#define HH(a, b, c, d, x, s, ac) \
 (a) += H ((b), (c), (d)) + (x) + (DWORD)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \

#define II(a, b, c, d, x, s, ac) \
 (a) += I ((b), (c), (d)) + (x) + (DWORD)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \


void PMessageDigest5::Transform(const BYTE * block)
{
  DWORD a = state[0];
  DWORD b = state[1];
  DWORD c = state[2];
  DWORD d = state[3];

  DWORD x[16];
  for (PINDEX i = 0; i < 16; i++)
    x[i] = ((PUInt32l*)block)[i];

  /* Round 1 */
  FF(a, b, c, d, x[ 0], S11, 0xd76aa478); /* 1 */
  FF(d, a, b, c, x[ 1], S12, 0xe8c7b756); /* 2 */
  FF(c, d, a, b, x[ 2], S13, 0x242070db); /* 3 */
  FF(b, c, d, a, x[ 3], S14, 0xc1bdceee); /* 4 */
  FF(a, b, c, d, x[ 4], S11, 0xf57c0faf); /* 5 */
  FF(d, a, b, c, x[ 5], S12, 0x4787c62a); /* 6 */
  FF(c, d, a, b, x[ 6], S13, 0xa8304613); /* 7 */
  FF(b, c, d, a, x[ 7], S14, 0xfd469501); /* 8 */
  FF(a, b, c, d, x[ 8], S11, 0x698098d8); /* 9 */
  FF(d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
  FF(c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
  FF(b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
  FF(a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
  FF(d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
  FF(c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
  FF(b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

 /* Round 2 */
  GG(a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
  GG(d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
  GG(c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
  GG(b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
  GG(a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
  GG(d, a, b, c, x[10], S22,  0x2441453); /* 22 */
  GG(c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
  GG(b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
  GG(a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
  GG(d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
  GG(c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
  GG(b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
  GG(a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
  GG(d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
  GG(c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
  GG(b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

  /* Round 3 */
  HH(a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
  HH(d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
  HH(c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
  HH(b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
  HH(a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
  HH(d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
  HH(c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
  HH(b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
  HH(a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
  HH(d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
  HH(c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
  HH(b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */
  HH(a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
  HH(d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
  HH(c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
  HH(b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */

  /* Round 4 */
  II(a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
  II(d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
  II(c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
  II(b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
  II(a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
  II(d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
  II(c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
  II(b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
  II(a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
  II(d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
  II(c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
  II(b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
  II(a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
  II(d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
  II(c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
  II(b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */

  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;

  // Zeroize sensitive information.
  memset(x, 0, sizeof(x));
}


void PMessageDigest5::Start()
{
  // Load magic initialization constants.
  state[0] = 0x67452301;
  state[1] = 0xefcdab89;
  state[2] = 0x98badcfe;
  state[3] = 0x10325476;

  count = 0;
}


void PMessageDigest5::Process(const PString & str)
{
  Process((const char *)str);
}


void PMessageDigest5::Process(const char * cstr)
{
  Process(cstr, strlen(cstr));
}


void PMessageDigest5::Process(const PBYTEArray & data)
{
  Process(data, data.GetSize());
}


void PMessageDigest5::Process(const void * dataPtr, PINDEX length)
{
  const BYTE * data = (const BYTE *)dataPtr;

  // Compute number of bytes mod 64
  PINDEX index = (PINDEX)((count >> 3) & 0x3F);
  PINDEX partLen = 64 - index;

  // Update number of bits
  count += (PUInt64)length << 3;

  // See if have a buffer full
  PINDEX i;
  if (length < partLen)
    i = 0;
  else {
    // Transform as many times as possible.
    memcpy(&buffer[index], data, partLen);
    Transform(buffer);
    for (i = partLen; i + 63 < length; i += 64)
      Transform(&data[i]);
    index = 0;
  }

  // Buffer remaining input
  memcpy(&buffer[index], &data[i], length-i);
}


PString PMessageDigest5::Complete()
{
  Code result;
  Complete(result);
  return PBase64::Encode(&result, sizeof(result));
}


void PMessageDigest5::Complete(Code & result)
{
  // Put the count into bytes platform independently
  PUInt64l countBytes = count;

  // Pad out to 56 mod 64.
  PINDEX index = (PINDEX)((count >> 3) & 0x3f);
  PINDEX padLen = (index < 56) ? (56 - index) : (120 - index);
  static BYTE const padding[64] = {
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };
  Process(padding, padLen);

  // Append length
  Process(&countBytes, sizeof(countBytes));

  // Store state in digest
  for (PINDEX i = 0; i < PARRAYSIZE(state); i++)
    result.value[i] = state[i];

  // Zeroize sensitive information.
  memset(this, 0, sizeof(*this));
}


PString PMessageDigest5::Encode(const PString & str)
{
  return Encode((const char *)str);
}


void PMessageDigest5::Encode(const PString & str, Code & result)
{
  Encode((const char *)str, result);
}


PString PMessageDigest5::Encode(const char * cstr)
{
  return Encode((const BYTE *)cstr, strlen(cstr));
}


void PMessageDigest5::Encode(const char * cstr, Code & result)
{
  Encode((const BYTE *)cstr, strlen(cstr), result);
}


PString PMessageDigest5::Encode(const PBYTEArray & data)
{
  return Encode(data, data.GetSize());
}


void PMessageDigest5::Encode(const PBYTEArray & data, Code & result)
{
  Encode(data, data.GetSize(), result);
}


PString PMessageDigest5::Encode(const void * data, PINDEX length)
{
  Code result;
  Encode(data, length, result);
  return PBase64::Encode(&result, sizeof(result));
}


void PMessageDigest5::Encode(const void * data, PINDEX len, Code & result)
{
  PMessageDigest5 stomach;
  stomach.Process(data, len);
  stomach.Complete(result);
}



///////////////////////////////////////////////////////////////////////////////
// PCypher

PCypher::PCypher(PINDEX blkSize, BlockChainMode mode)
  : blockSize(blkSize),
    chainMode(mode)
{
}


PCypher::PCypher(const void * keyData, PINDEX keyLength,
                 PINDEX blkSize, BlockChainMode mode)
  : key((const BYTE *)keyData, keyLength),
    blockSize(blkSize),
    chainMode(mode)
{
}


PString PCypher::Encode(const PString & str)
{
  return Encode((const char *)str, str.GetLength());
}


PString PCypher::Encode(const PBYTEArray & clear)
{
  return Encode((const BYTE *)clear, clear.GetSize());
}


PString PCypher::Encode(const void * data, PINDEX length)
{
  PBYTEArray coded;
  Encode(data, length, coded);
  return PBase64::Encode(coded);
}


void PCypher::Encode(const PBYTEArray & clear, PBYTEArray & coded)
{
  Encode((const BYTE *)clear, clear.GetSize(), coded);
}


void PCypher::Encode(const void * data, PINDEX length, PBYTEArray & coded)
{
  PAssert((blockSize%8) == 0, PUnsupportedFeature);

  Initialise(TRUE);

  const BYTE * in = (const BYTE *)data;
  BYTE * out = coded.GetPointer(
                      blockSize > 1 ? (length/blockSize+1)*blockSize : length);

  while (length >= blockSize) {
    EncodeBlock(in, out);
    in += blockSize;
    out += blockSize;
    length -= blockSize;
  }

  if (blockSize > 1) {
    PBYTEArray extra(blockSize);
    extra[blockSize-1] = (BYTE)length;
    for (PINDEX i = 0; i < length; i++)
      extra[i] = *in++;
    EncodeBlock(extra, out);
  }
}


PString PCypher::Decode(const PString & cypher)
{
  PString clear;
  if (Decode(cypher, clear))
    return clear;
  return PString();
}


BOOL PCypher::Decode(const PString & cypher, PString & clear)
{
  clear = PString();

  PBYTEArray clearText;
  if (!Decode(cypher, clearText))
    return FALSE;

  if (clearText.IsEmpty())
    return TRUE;

  PINDEX sz = clearText.GetSize();
  memcpy(clear.GetPointer(sz+1), (const BYTE *)clearText, sz);
  return TRUE;
}


BOOL PCypher::Decode(const PString & cypher, PBYTEArray & clear)
{
  PBYTEArray coded;
  if (!PBase64::Decode(cypher, coded))
    return FALSE;
  return Decode(coded, clear);
}


PINDEX PCypher::Decode(const PString & cypher, void * data, PINDEX length)
{
  PBYTEArray coded;
  PBase64::Decode(cypher, coded);
  PBYTEArray clear;
  if (!Decode(coded, clear))
    return 0;
  memcpy(data, clear, PMIN(length, clear.GetSize()));
  return clear.GetSize();
}


PINDEX PCypher::Decode(const PBYTEArray & coded, void * data, PINDEX length)
{
  PBYTEArray clear;
  if (!Decode(coded, clear))
    return 0;
  memcpy(data, coded, PMIN(length, clear.GetSize()));
  return clear.GetSize();
}


BOOL PCypher::Decode(const PBYTEArray & coded, PBYTEArray & clear)
{
  PAssert((blockSize%8) == 0, PUnsupportedFeature);
  if (coded.IsEmpty() || (coded.GetSize()%blockSize) != 0)
    return FALSE;

  Initialise(FALSE);

  const BYTE * in = coded;
  PINDEX length = coded.GetSize();
  BYTE * out = clear.GetPointer(length);

  while (length > 0) {
    DecodeBlock(in, out);
    in += blockSize;
    out += blockSize;
    length -= blockSize;
  }

  if (blockSize != 1)
    clear.SetSize(clear.GetSize() - blockSize + *--out);

  return TRUE;
}



///////////////////////////////////////////////////////////////////////////////
// PTEACypher

PTEACypher::PTEACypher(BlockChainMode chainMode)
  : PCypher(8, chainMode)
{
  GenerateKey(*(Key*)key.GetPointer(sizeof(Key)));
}


PTEACypher::PTEACypher(const Key & keyData, BlockChainMode chainMode)
  : PCypher(&keyData, sizeof(Key), 8, chainMode)
{
}


void PTEACypher::SetKey(const Key & newKey)
{
  memcpy(key.GetPointer(sizeof(Key)), &newKey, sizeof(Key));
}


void PTEACypher::GetKey(Key & newKey) const
{
  memcpy(&newKey, key, sizeof(Key));
}


void PTEACypher::GenerateKey(Key & newKey)
{
#ifdef _DEBUG
  srand(0);
#else
  srand((unsigned)(time(NULL)+clock()));
#endif
  for (PINDEX i = 0; i < sizeof(Key); i++)
    newKey.value[i] = (BYTE)rand();
}


static const DWORD TEADelta = 0x9e3779b9;    // Magic number for key schedule

void PTEACypher::Initialise(BOOL)
{
  k0 = ((const PUInt32l *)(const BYTE *)key)[0];
  k1 = ((const PUInt32l *)(const BYTE *)key)[1];
  k2 = ((const PUInt32l *)(const BYTE *)key)[2];
  k3 = ((const PUInt32l *)(const BYTE *)key)[3];
}


void PTEACypher::EncodeBlock(const void * in, void * out)
{
  DWORD y = ((PUInt32b*)in)[0];
  DWORD z = ((PUInt32b*)in)[1];
  DWORD sum = 0;
  for (PINDEX count = 32; count > 0; count--) {
    sum += TEADelta;    // Magic number for key schedule
    y += (z<<4)+k0 ^ z+sum ^ (z>>5)+k1;
    z += (y<<4)+k2 ^ y+sum ^ (y>>5)+k3;   /* end cycle */
  }
  ((PUInt32b*)out)[0] = y;
  ((PUInt32b*)out)[1] = z;
}


void PTEACypher::DecodeBlock(const void * in, void * out)
{
  DWORD y = ((PUInt32b*)in)[0];
  DWORD z = ((PUInt32b*)in)[1];
  DWORD sum = TEADelta<<5;
  for (PINDEX count = 32; count > 0; count--) {
    z -= (y<<4)+k2 ^ y+sum ^ (y>>5)+k3; 
    y -= (z<<4)+k0 ^ z+sum ^ (z>>5)+k1;
    sum -= TEADelta;    // Magic number for key schedule
  }
  ((PUInt32b*)out)[0] = y;
  ((PUInt32b*)out)[1] = z;
}


///////////////////////////////////////////////////////////////////////////////
// PSecureConfig

static const char DefaultSecuredOptions[] = "Secured Options";
static const char DefaultSecurityKey[] = "Validation";
static const char DefaultExpiryDateKey[] = "Expiry Date";
static const char DefaultOptionBitsKey[] = "Option Bits";
static const char DefaultPendingPrefix[] = "Pending:";

PSecureConfig::PSecureConfig(const PTEACypher::Key & prodKey,
                             const PStringArray & secKeys,
                             Source src)
  : PConfig(DefaultSecuredOptions, src),
    securedKeys(secKeys),
    securityKey(DefaultSecurityKey),
    expiryDateKey(DefaultExpiryDateKey),
    optionBitsKey(DefaultOptionBitsKey),
    pendingPrefix(DefaultPendingPrefix)
{
  productKey = prodKey;
}


PSecureConfig::PSecureConfig(const PTEACypher::Key & prodKey,
                             const char * const * secKeys,
                             PINDEX count,
                             Source src)
  : PConfig(DefaultSecuredOptions, src),
    securedKeys(count, secKeys),
    securityKey(DefaultSecurityKey),
    expiryDateKey(DefaultExpiryDateKey),
    optionBitsKey(DefaultOptionBitsKey),
    pendingPrefix(DefaultPendingPrefix)
{
  productKey = prodKey;
}


void PSecureConfig::GetProductKey(PTEACypher::Key & prodKey) const
{
  prodKey = productKey;
}


PSecureConfig::ValidationState PSecureConfig::GetValidation() const
{
  PString str;
  BOOL allEmpty = TRUE;
  PMessageDigest5 digestor;
  for (PINDEX i = 0; i < securedKeys.GetSize(); i++) {
    str = GetString(securedKeys[i]);
    if (!str.IsEmpty()) {
      digestor.Process(str.Trim());
      allEmpty = FALSE;
    }
  }
  str = GetString(expiryDateKey);
  if (!str.IsEmpty()) {
    digestor.Process(str);
    allEmpty = FALSE;
  }
  str = GetString(optionBitsKey);
  if (!str.IsEmpty()) {
    digestor.Process(str);
    allEmpty = FALSE;
  }

  PString vkey = GetString(securityKey);
  if (allEmpty)
    return (!vkey || GetBoolean(pendingPrefix + securityKey)) ? Pending : Defaults;

  PMessageDigest5::Code code;
  digestor.Complete(code);

  if (vkey.IsEmpty())
    return Invalid;

  BYTE info[sizeof(code)+1+sizeof(DWORD)];
  PTEACypher crypt(productKey);
  if (crypt.Decode(vkey, info, sizeof(info)) != sizeof(info))
    return Invalid;

  if (memcmp(info, &code, sizeof(code)) != 0)
    return Invalid;

  PTime now;
  if (now > GetTime(expiryDateKey))
    return Expired;

  return IsValid;
}


BOOL PSecureConfig::ValidatePending()
{
  if (GetValidation() != Pending)
    return FALSE;

  PString vkey = GetString(securityKey);
  if (vkey.IsEmpty())
    return TRUE;

  PMessageDigest5::Code code;
  BYTE info[sizeof(code)+1+sizeof(DWORD)];
  PTEACypher crypt(productKey);
  if (crypt.Decode(vkey, info, sizeof(info)) != sizeof(info))
    return FALSE;

  PTime expiryDate(0, 0, 0,
            1, info[sizeof(code)]&15, (info[sizeof(code)]>>4)+1996, PTime::GMT);
  PString expiry = expiryDate.AsString("d MMME yyyy", PTime::GMT);

  // This is for alignment problems on processors that care about such things
  PUInt32b opt;
  void * dst = &opt;
  void * src = &info[sizeof(code)+1];
  memcpy(dst, src, sizeof(opt));
  PString options(PString::Unsigned, (DWORD)opt);

  PMessageDigest5 digestor;
  PINDEX i;
  for (i = 0; i < securedKeys.GetSize(); i++)
    digestor.Process(GetString(pendingPrefix + securedKeys[i]).Trim());
  digestor.Process(expiry);
  digestor.Process(options);
  digestor.Complete(code);

  if (memcmp(info, &code, sizeof(code)) != 0)
    return FALSE;

  SetString(expiryDateKey, expiry);
  SetString(optionBitsKey, options);

  for (i = 0; i < securedKeys.GetSize(); i++) {
    PString str = GetString(pendingPrefix + securedKeys[i]);
    if (!str.IsEmpty())
      SetString(securedKeys[i], str);
    DeleteKey(pendingPrefix + securedKeys[i]);
  }
  DeleteKey(pendingPrefix + securityKey);

  return TRUE;
}


void PSecureConfig::ResetPending()
{
  if (GetBoolean(pendingPrefix + securityKey)) {
    for (PINDEX i = 0; i < securedKeys.GetSize(); i++)
      DeleteKey(securedKeys[i]);
  }
  else {
    SetBoolean(pendingPrefix + securityKey, TRUE);

    for (PINDEX i = 0; i < securedKeys.GetSize(); i++) {
      PString str = GetString(securedKeys[i]);
      if (!str.IsEmpty())
        SetString(pendingPrefix + securedKeys[i], str);
      DeleteKey(securedKeys[i]);
    }
  }
  DeleteKey(expiryDateKey);
  DeleteKey(optionBitsKey);
}


///////////////////////////////////////////////////////////////////////////////
