/*
 * $Id: inetprot.cxx,v 1.20 1996/05/26 03:46:22 robertj Exp $
 *
 * Portable Windows Library
 *
 * Operating System Classes Implementation
 *
 * Copyright 1994 Equivalence
 *
 * $Log: inetprot.cxx,v $
 * Revision 1.20  1996/05/26 03:46:22  robertj
 * Compatibility to GNU 2.7.x
 *
 * Revision 1.19  1996/05/15 10:15:15  robertj
 * Added access function to set intercharacter line read timeout.
 *
 * Revision 1.18  1996/05/09 12:14:04  robertj
 * Rewrote the "unread" buffer usage and then used it to improve ReadLine() performance.
 *
 * Revision 1.17  1996/03/31 08:57:34  robertj
 * Changed MIME type for no extension from binary to text.
 * Added flush of data before sending a command.
 * Added version of WriteCommand() and ExecteCommand() without argument string.
 *
 * Revision 1.15  1996/03/18 13:33:13  robertj
 * Fixed incompatibilities to GNU compiler where PINDEX != int.
 *
 * Revision 1.14  1996/03/16 04:53:07  robertj
 * Changed all the get host name and get host address functions to be more consistent.
 * Added ParseReponse() for splitting reponse line into code and info.
 * Changed lastResponseCode to an integer.
 * Fixed bug in MIME write function, should be const.
 * Added PString parameter version of UnRead().
 *
 * Revision 1.13  1996/03/04 12:20:41  robertj
 * Split file into mailsock.cxx
 *
 * Revision 1.12  1996/02/25 11:16:07  robertj
 * Fixed bug in ReadResponse() for multi-line responses under FTP..
 *
 * Revision 1.11  1996/02/25 03:05:12  robertj
 * Added decoding of Base64 to a block of memory instead of PBYTEArray.
 *
 * Revision 1.10  1996/02/19 13:31:26  robertj
 * Changed stuff to use new & operator..
 *
 * Revision 1.9  1996/02/15 14:42:41  robertj
 * Fixed warning for long to int conversion.
 *
 * Revision 1.8  1996/02/13 12:57:49  robertj
 * Added access to the last response in an application socket.
 *
 * Revision 1.7  1996/02/03 11:33:17  robertj
 * Changed RadCmd() so can distinguish between I/O error and unknown command.
 *
 * Revision 1.6  1996/01/28 14:11:11  robertj
 * Fixed bug in MIME content types for non caseless strings.
 * Added default value in string for service name.
 *
 * Revision 1.5  1996/01/28 02:48:27  robertj
 * Removal of MemoryPointer classes as usage didn't work for GNU.
 *
 * Revision 1.4  1996/01/26 02:24:29  robertj
 * Further implemetation.
 *
 * Revision 1.3  1996/01/23 13:18:43  robertj
 * Major rewrite for HTTP support.
 *
 * Revision 1.2  1995/11/09 12:19:29  robertj
 * Fixed missing state assertion in state machine.
 *
 * Revision 1.1  1995/06/17 00:50:37  robertj
 * Initial revision
 *
 */

#include <ptlib.h>
#include <mailsock.h>


static const PString CRLF = "\r\n";
static const PString CRLFdotCRLF = "\r\n.\r\n";


//////////////////////////////////////////////////////////////////////////////
// PApplicationSocket

PApplicationSocket::PApplicationSocket(PINDEX cmdCount,
                                       char const * const * cmdNames,
                                       WORD port)
  : PTCPSocket(port), commandNames(cmdCount, cmdNames, TRUE)
{
  Construct();
}


PApplicationSocket::PApplicationSocket(PINDEX cmdCount,
                                       char const * const * cmdNames,
                                       const PString & service)
  : PTCPSocket(service), commandNames(cmdCount, cmdNames, TRUE)
{
  Construct();
}


PApplicationSocket::PApplicationSocket(PINDEX cmdCount,
                                       char const * const * cmdNames,
                                       const PString & address,
                                       WORD port)
  : PTCPSocket(address, port), commandNames(cmdCount, cmdNames, TRUE)
{
  Construct();
}


PApplicationSocket::PApplicationSocket(PINDEX cmdCount,
                                       char const * const * cmdNames,
                                       const PString & address,
                                       const PString & service)
  : PTCPSocket(address, service), commandNames(cmdCount, cmdNames, TRUE)
{
  Construct();
}


PApplicationSocket::PApplicationSocket(PINDEX cmdCount,
                                       char const * const * cmdNames,
                                       PSocket & socket)
  : PTCPSocket(socket), commandNames(cmdCount, cmdNames, TRUE)
{
  Construct();
}


void PApplicationSocket::Construct()
{
  SetReadTimeout(PTimeInterval(0, 0, 10));  // 10 minutes
  readLineTimeout = PTimeInterval(0, 10);   // 10 seconds
  stuffingState = DontStuff;
  newLineToCRLF = TRUE;
  unReadCount = 0;
}

void PApplicationSocket::SetReadLineTimeout(const PTimeInterval & t)
{
  readLineTimeout = t;
}

BOOL PApplicationSocket::Read(void * buf, PINDEX len)
{
  lastReadCount = PMIN(unReadCount, len);
  const char * unReadPtr = ((const char *)unReadBuffer)+unReadCount;
  char * bufptr = (char *)buf;
  while (unReadCount > 0 && len > 0) {
    *bufptr++ = *--unReadPtr;
    unReadCount--;
    len--;
  }

  if (unReadCount == 0)
    unReadBuffer.SetSize(0);

  if (len > 0) {
    PINDEX saveCount = lastReadCount;
    PTCPSocket::Read(bufptr, len);
    lastReadCount += saveCount;
  }

  return lastReadCount > 0;
}


BOOL PApplicationSocket::Write(const void * buf, PINDEX len)
{
  if (len == 0 || stuffingState == DontStuff)
    return PTCPSocket::Write(buf, len);

  PINDEX totalWritten = 0;
  const char * base = (const char *)buf;
  const char * current = base;
  while (len-- > 0) {
    switch (stuffingState) {
      case StuffIdle :
        switch (*current) {
          case '\r' :
            stuffingState = StuffCR;
            break;

          case '\n' :
            if (newLineToCRLF) {
              if (current > base) {
                if (!PTCPSocket::Write(base, current - base))
                  return FALSE;
                totalWritten += lastWriteCount;
              }
              if (!PTCPSocket::Write("\r", 1))
                return FALSE;
              totalWritten += lastWriteCount;
              base = current;
            }
        }
        break;

      case StuffCR :
        stuffingState = *current != '\n' ? StuffIdle : StuffCRLF;
        break;

      case StuffCRLF :
        if (*current == '.') {
          if (current > base) {
            if (!PTCPSocket::Write(base, current - base))
              return FALSE;
            totalWritten += lastWriteCount;
          }
          if (!PTCPSocket::Write(".", 1))
            return FALSE;
          totalWritten += lastWriteCount;
          base = current;
        }
        // Then do default state

      default :
        stuffingState = StuffIdle;
        break;
    }
    current++;
  }

  if (current > base)
    if (!PTCPSocket::Write(base, current - base))
      return FALSE;

  lastWriteCount += totalWritten;
  return lastWriteCount > 0;
}


BOOL PApplicationSocket::WriteLine(const PString & line)
{
  if (line.FindOneOf(CRLF) == P_MAX_INDEX)
    return WriteString(line + CRLF);

  PStringArray lines = line.Lines();
  for (PINDEX i = 0; i < lines.GetSize(); i++)
    if (!WriteString(lines[i] + CRLF))
      return FALSE;

  return TRUE;
}


BOOL PApplicationSocket::ReadLine(PString & str, BOOL allowContinuation)
{
  str = PString();

  PCharArray line(100);
  PINDEX count = 0;
  BOOL gotEndOfLine = FALSE;

  int c = ReadChar();
  if (c < 0)
    return FALSE;

  PTimeInterval oldTimeout = GetReadTimeout();
  SetReadTimeout(readLineTimeout);

  while (c >= 0 && !gotEndOfLine) {
    if (unReadCount == 0) {
      char readAhead[1000];
      SetReadTimeout(0);
      if (PTCPSocket::Read(readAhead, sizeof(readAhead)))
        UnRead(readAhead, GetLastReadCount());
      SetReadTimeout(readLineTimeout);
    }
    switch (c) {
      case '\b' :
      case '\177' :
        if (count > 0)
          count--;
        c = ReadChar();
        break;

      case '\r' :
        c = ReadChar();
        if (c >= 0 && c != '\n')
          UnRead(c);
        // Then do line feed case

      case '\n' :
        if (count == 0 || !allowContinuation || (c = ReadChar()) < 0)
          gotEndOfLine = TRUE;
        else if (c != ' ' && c != '\t') {
          UnRead(c);
          gotEndOfLine = TRUE;
        }
        break;

      default :
        if (count >= line.GetSize())
          line.SetSize(count + 100);
        line[count++] = (char)c;
        c = ReadChar();
    }
  }

  SetReadTimeout(oldTimeout);

  if (count > 0)
    str = PString(line, count);
  return gotEndOfLine;
}


void PApplicationSocket::UnRead(int ch)
{
  unReadBuffer.SetSize((unReadCount+256)&-256);
  unReadBuffer[unReadCount++] = (char)ch;
}


void PApplicationSocket::UnRead(const PString & str)
{
  UnRead((const char *)str, str.GetLength());
}


void PApplicationSocket::UnRead(const void * buffer, PINDEX len)
{
  char * unreadptr = unReadBuffer.GetPointer((unReadCount+len+255)&-256);
  const char * bufptr = ((const char *)buffer)+len;
  unReadCount += len;
  while (len-- > 0)
    *unreadptr++ = *--bufptr;
}


BOOL PApplicationSocket::WriteCommand(PINDEX cmdNumber)
{
  if (cmdNumber >= commandNames.GetSize())
    return FALSE;
  return WriteLine(commandNames[cmdNumber]);
}

BOOL PApplicationSocket::WriteCommand(PINDEX cmdNumber, const PString & param)
{
  if (cmdNumber >= commandNames.GetSize())
    return FALSE;
  if (param.IsEmpty())
    return WriteLine(commandNames[cmdNumber]);
  else
    return WriteLine(commandNames[cmdNumber] & param);
}


BOOL PApplicationSocket::ReadCommand(PINDEX & num, PString & args)
{
  do {
    if (!ReadLine(args))
      return FALSE;
  } while (args.IsEmpty());

  PINDEX endCommand = args.Find(' ');
  if (endCommand == P_MAX_INDEX)
    endCommand = args.GetLength();
  PCaselessString cmd = args.Left(endCommand);

  num = commandNames.GetValuesIndex(cmd);
  if (num != P_MAX_INDEX)
    args = args.Mid(endCommand+1);

  return TRUE;
}


BOOL PApplicationSocket::WriteResponse(unsigned code, const PString & info)
{
  return WriteResponse(psprintf("%03u", code), info);
}


BOOL PApplicationSocket::WriteResponse(const PString & code,
                                       const PString & info)
{
  if (info.FindOneOf(CRLF) == P_MAX_INDEX)
    return WriteString(code & info + CRLF);

  PStringArray lines = info.Lines();
  PINDEX i;
  for (i = 0; i < lines.GetSize()-1; i++)
    if (!WriteString(code + '-' + lines[i] + CRLF))
      return FALSE;

  return WriteString(code & lines[i] + CRLF);
}


BOOL PApplicationSocket::ReadResponse()
{
  PString line;
  if (!ReadLine(line))
    return FALSE;

  PINDEX continuePos = ParseResponse(line);
  if (continuePos == 0)
    return TRUE;

  char continueChar = line[continuePos];
  while (!isdigit(line[0]) || line[continuePos] == continueChar) {
    lastResponseInfo += '\n';
    if (!ReadLine(line))
      return FALSE;
    lastResponseInfo += line.Mid(continuePos+1);
  }

  return TRUE;
}


BOOL PApplicationSocket::ReadResponse(int & code, PString & info)
{
  BOOL retval = ReadResponse();

  code = lastResponseCode;
  info = lastResponseInfo;

  return retval;
}


PINDEX PApplicationSocket::ParseResponse(const PString & line)
{
  PINDEX endCode = line.FindOneOf(" -");
  if (endCode == P_MAX_INDEX) {
    lastResponseCode = -1;
    lastResponseInfo = line;
    return 0;
  }

  lastResponseCode = line.Left(endCode).AsInteger();
  lastResponseInfo = line.Mid(endCode+1);
  return line[endCode] != ' ' ? endCode : 0;
}


int PApplicationSocket::ExecuteCommand(PINDEX cmd)
{
  return ExecuteCommand(cmd, PString());
}


int PApplicationSocket::ExecuteCommand(PINDEX cmd,
                                       const PString & param)
{
  PTimeInterval oldTimeout = GetReadTimeout();
  SetReadTimeout(0);
  while (ReadChar() >= 0)
    ;
  SetReadTimeout(oldTimeout);
  return WriteCommand(cmd, param) && ReadResponse() ? lastResponseCode : -1;
}


int PApplicationSocket::GetLastResponseCode() const
{
  return lastResponseCode;
}


PString PApplicationSocket::GetLastResponseInfo() const
{
  return lastResponseInfo;
}


//////////////////////////////////////////////////////////////////////////////
// PMIMEInfo

PMIMEInfo::PMIMEInfo(istream & strm)
{
  ReadFrom(strm);
}


PMIMEInfo::PMIMEInfo(PApplicationSocket & socket)
{
  Read(socket);
}


void PMIMEInfo::PrintOn(ostream &strm) const
{
  for (PINDEX i = 0; i < GetSize(); i++)
    strm << GetKeyAt(i) << ": " << GetDataAt(i) << CRLF;
  strm << CRLF;
}


void PMIMEInfo::ReadFrom(istream &strm)
{
  PString line;
  while (strm.good()) {
    strm >> line;
    if (line.IsEmpty())
      break;

    PINDEX colonPos = line.Find(':');
    if (colonPos != P_MAX_INDEX) {
      PCaselessString fieldName  = line.Left(colonPos).Trim();
      PString fieldValue = line(colonPos+1, P_MAX_INDEX).Trim();
      SetAt(fieldName, fieldValue);
    }
  }
}


BOOL PMIMEInfo::Read(PApplicationSocket & socket)
{
  PString line;
  while (socket.ReadLine(line, TRUE)) {
    if (line.IsEmpty())
      return TRUE;

    PINDEX colonPos = line.Find(':');
    if (colonPos != P_MAX_INDEX) {
      PCaselessString fieldName  = line.Left(colonPos).Trim();
      PString fieldValue = line(colonPos+1, P_MAX_INDEX).Trim();
      SetAt(fieldName, fieldValue);
    }
  }

  return FALSE;
}


BOOL PMIMEInfo::Write(PApplicationSocket & socket) const
{
  for (PINDEX i = 0; i < GetSize(); i++) {
    if (!socket.WriteLine(GetKeyAt(i) + ": " + GetDataAt(i)))
      return FALSE;
  }

  return socket.WriteString(CRLF);
}


PString PMIMEInfo::GetString(const PString & key, const PString & dflt) const
{
  if (GetAt(PCaselessString(key)) == NULL)
    return dflt;
  return operator[](key);
}


long PMIMEInfo::GetInteger(const PString & key, long dflt) const
{
  if (GetAt(PCaselessString(key)) == NULL)
    return dflt;
  return operator[](key).AsInteger();
}


static const PStringToString::Initialiser DefaultContentTypes[] = {
  { ".txt", "text/plain" },
  { ".text", "text/plain" },
  { ".html", "text/html" },
  { ".htm", "text/html" },
  { ".aif", "audio/aiff" },
  { ".aiff", "audio/aiff" },
  { ".au", "audio/basic" },
  { ".snd", "audio/basic" },
  { ".wav", "audio/wav" },
  { ".gif", "image/gif" },
  { ".xbm", "image/x-bitmap" },
  { ".tif", "image/tiff" },
  { ".tiff", "image/tiff" },
  { ".jpg", "image/jpeg" },
  { ".jpe", "image/jpeg" },
  { ".jpeg", "image/jpeg" },
  { ".avi", "video/avi" },
  { ".mpg", "video/mpeg" },
  { ".mpeg", "video/mpeg" },
  { ".qt", "video/quicktime" },
  { ".mov", "video/quicktime" }
};

PStringToString PMIMEInfo::contentTypes(PARRAYSIZE(DefaultContentTypes),
                                        DefaultContentTypes,
                                        TRUE);


void PMIMEInfo::SetAssociation(const PStringToString & allTypes, BOOL merge)
{
  if (!merge)
    contentTypes.RemoveAll();
  for (PINDEX i = 0; i < allTypes.GetSize(); i++)
    contentTypes.SetAt(allTypes.GetKeyAt(i), allTypes.GetDataAt(i));
}


PString PMIMEInfo::GetContentType(const PString & fType)
{
  if (fType.IsEmpty())
    return "text/plain";

  if (contentTypes.Contains(fType))
    return contentTypes[fType];

  return "application/octet-stream";
}


///////////////////////////////////////////////////////////////////////////////
// PBase64

PBase64::PBase64()
{
  StartEncoding();
  StartDecoding();
}


void PBase64::StartEncoding(BOOL useCRLF)
{
  encodedString = "";
  encodeLength = nextLine = saveCount = 0;
  useCRLFs = useCRLF;
}


void PBase64::ProcessEncoding(const PString & str)
{
  ProcessEncoding((const char *)str);
}


void PBase64::ProcessEncoding(const char * cstr)
{
  ProcessEncoding((const BYTE *)cstr, strlen(cstr));
}


void PBase64::ProcessEncoding(const PBYTEArray & data)
{
  ProcessEncoding(data, data.GetSize());
}


static const char Binary2Base64[65] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void PBase64::OutputBase64(const BYTE * data)
{
  char * out = encodedString.GetPointer((encodeLength&-256) + 256);

  out[encodeLength++] = Binary2Base64[data[0] >> 2];
  out[encodeLength++] = Binary2Base64[((data[0]&3)<<4) | (data[1]>>4)];
  out[encodeLength++] = Binary2Base64[((data[1]&15)<<2) | (data[2]>>6)];
  out[encodeLength++] = Binary2Base64[data[2]&0x3f];

  if (++nextLine > 76) {
    if (useCRLFs)
      out[encodeLength++] = '\r';
    out[encodeLength++] = '\n';
    nextLine = 0;
  }
}


void PBase64::ProcessEncoding(const BYTE * data, PINDEX length)
{
  while (saveCount < 3) {
    saveTriple[saveCount++] = *data++;
    if (--length <= 0)
      return;
  }

  OutputBase64(saveTriple);

  PINDEX i;
  for (i = 0; i+2 < length; i += 3)
    OutputBase64(data+i);

  saveCount = length - i;
  switch (saveCount) {
    case 2 :
      saveTriple[0] = data[i++];
      saveTriple[1] = data[i];
      break;
    case 1 :
      saveTriple[0] = data[i];
  }
}


PString PBase64::GetEncodedString()
{
  PString retval = encodedString;
  encodedString = "";
  encodeLength = 0;
  return retval;
}


PString PBase64::CompleteEncoding()
{
  char * out = encodedString.GetPointer(encodeLength + 5)+encodeLength;

  switch (saveCount) {
    case 1 :
      *out++ = Binary2Base64[saveTriple[0] >> 2];
      *out++ = Binary2Base64[(saveTriple[0]&3)<<4];
      *out++ = '=';
      *out   = '=';
      break;

    case 2 :
      *out++ = Binary2Base64[saveTriple[0] >> 2];
      *out++ = Binary2Base64[((saveTriple[0]&3)<<4) | (saveTriple[1]>>4)];
      *out++ = Binary2Base64[((saveTriple[1]&15)<<2)];
      *out   = '=';
  }

  return encodedString;
}


PString PBase64::Encode(const PString & str)
{
  return Encode((const char *)str);
}


PString PBase64::Encode(const char * cstr)
{
  return Encode((const BYTE *)cstr, strlen(cstr));
}


PString PBase64::Encode(const PBYTEArray & data)
{
  return Encode(data, data.GetSize());
}


PString PBase64::Encode(const BYTE * data, PINDEX length)
{
  PBase64 encoder;
  encoder.ProcessEncoding(data, length);
  return encoder.CompleteEncoding();
}


void PBase64::StartDecoding()
{
  perfectDecode = TRUE;
  quadPosition = 0;
  decodedData.SetSize(0);
  decodeSize = 0;
}


BOOL PBase64::ProcessDecoding(const PString & str)
{
  return ProcessDecoding((const char *)str);
}


BOOL PBase64::ProcessDecoding(const char * cstr)
{
  static const BYTE Base642Binary[256] = {
    96, 99, 99, 99, 99, 99, 99, 99, 99, 99, 98, 99, 99, 98, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 62, 99, 99, 99, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 99, 99, 99, 97, 99, 99,
    99,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 99, 99, 99, 99, 99,
    99, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99
  };

  for (;;) {
    BYTE value = Base642Binary[(BYTE)*cstr++];
    switch (value) {
      case 96 : // end of string
        return FALSE;

      case 97 : // '=' sign
        if (quadPosition == 3 || (quadPosition == 2 && *cstr == '='))
          return TRUE; // Stop decoding now as must be at end of data
        perfectDecode = FALSE;  // Ignore '=' sign but flag decode as suspect
        break;

      case 98 : // CRLFs
        break;  // Ignore totally

      case 99 :  // Illegal characters
        perfectDecode = FALSE;  // Ignore rubbish but flag decode as suspect
        break;

      default : // legal value from 0 to 63
        BYTE * out = decodedData.GetPointer(((decodeSize+1)&-256) + 256);
        switch (quadPosition) {
          case 0 :
            out[decodeSize] = (BYTE)(value << 2);
            break;
          case 1 :
            out[decodeSize++] |= (BYTE)(value >> 4);
            out[decodeSize] = (BYTE)((value&15) << 4);
            break;
          case 2 :
            out[decodeSize++] |= (BYTE)(value >> 2);
            out[decodeSize] = (BYTE)((value&3) << 6);
            break;
          case 3 :
            out[decodeSize++] |= (BYTE)value;
            break;
        }
        quadPosition = (quadPosition+1)&3;
    }
  }
}


PBYTEArray PBase64::GetDecodedData()
{
  decodedData.SetSize(decodeSize);
  PBYTEArray retval = decodedData;
  retval.MakeUnique();
  decodedData.SetSize(0);
  decodeSize = 0;
  return retval;
}


BOOL PBase64::GetDecodedData(BYTE * dataBlock, PINDEX length)
{
  if (decodeSize > length) {
    decodeSize = length;
    perfectDecode = FALSE;
  }
  memcpy(dataBlock, decodedData, decodeSize);
  decodedData.SetSize(0);
  decodeSize = 0;
  return perfectDecode;
}


PString PBase64::Decode(const PString & str)
{
  PBYTEArray data;
  Decode(str, data);
  return PString((const char *)(const BYTE *)data, data.GetSize());
}


BOOL PBase64::Decode(const PString & str, PBYTEArray & data)
{
  PBase64 decoder;
  decoder.ProcessDecoding(str);
  data = decoder.GetDecodedData();
  return decoder.IsDecodeOK();
}


BOOL PBase64::Decode(const PString & str, BYTE * dataBlock, PINDEX length)
{
  PBase64 decoder;
  decoder.ProcessDecoding(str);
  return decoder.GetDecodedData(dataBlock, length);
}


// End Of File ///////////////////////////////////////////////////////////////
