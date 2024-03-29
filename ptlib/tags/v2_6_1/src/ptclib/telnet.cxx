/*
 * telnet.cxx
 *
 * TELNET socket I/O channel class.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-2002 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifdef __GNUC__
#pragma implementation "telnet.h"
#endif

#include <ptlib.h>
#include <ptlib/sockets.h>
#include <ptclib/telnet.h>


//////////////////////////////////////////////////////////////////////////////
// PTelnetSocket

PTelnetSocket::PTelnetSocket()
  : PTCPSocket("telnet")
{
  Construct();
}


PTelnetSocket::PTelnetSocket(const PString & address)
  : PTCPSocket("telnet")
{
  Construct();
  Connect(address);
}


void PTelnetSocket::Construct()
{
  synchronising = 0;
  terminalType = "UNKNOWN";
  windowWidth = windowHeight = 0;
  state = StateNormal;

  memset(option, 0, sizeof(option));
  SetOurOption(TransmitBinary);
  SetOurOption(SuppressGoAhead);
  SetOurOption(StatusOption);
  SetOurOption(TimingMark);
  SetOurOption(TerminalSpeed);
  SetOurOption(TerminalType);
  SetTheirOption(TransmitBinary);
  SetTheirOption(SuppressGoAhead);
  SetTheirOption(StatusOption);
  SetTheirOption(TimingMark);
  SetTheirOption(EchoOption);

#ifdef _DEBUG
  debug = PTrue;
#else
  debug = PFalse;
#endif
}


#define PTelnetError if (debug) PError << "PTelnetSocket: "
#define PDebugError if (debug) PError

PBoolean PTelnetSocket::Connect(const PString & host)
{
  PTelnetError << "Connect" << endl;

  if (!PTCPSocket::Connect(host))
    return PFalse;

  SendDo(SuppressGoAhead);
  SendDo(StatusOption);
  SendWill(TerminalSpeed);
  return PTrue;
}


PBoolean PTelnetSocket::Accept(PSocket & sock)
{
  if (!PTCPSocket::Accept(sock))
    return PFalse;

  SendDo(SuppressGoAhead);
  SendWill(StatusOption);
  return PTrue;
}


PBoolean PTelnetSocket::Write(void const * buffer, PINDEX length)
{
  const BYTE * base = (const BYTE *)buffer;
  const BYTE * next = base;
  int count = 0;

  while (length > 0) {
    if (*next == '\r' &&
            !(length > 1 && next[1] == '\n') && !IsOurOption(TransmitBinary)) {
      // send the characters
      if (!PTCPSocket::Write(base, (next - base) + 1))
        return PFalse;
      count += lastWriteCount;

      char null = '\0';
      if (!PTCPSocket::Write(&null, 1))
        return PFalse;
      count += lastWriteCount;

      base = next+1;
    }

    if (*next == IAC) {
      // send the characters
      if (!PTCPSocket::Write(base, (next - base) + 1))
        return PFalse;
      count += lastWriteCount;
      base = next;
    }

    next++;
    length--;
  }

  if (next > base) {
    if (!PTCPSocket::Write(base, next - base))
      return PFalse;
    count += lastWriteCount;
  }

  lastWriteCount = count;
  return PTrue;
}


PBoolean PTelnetSocket::SendCommand(Command cmd, int opt)
{
  BYTE buffer[3];
  buffer[0] = IAC;
  buffer[1] = (BYTE)cmd;

  switch (cmd) {
    case DO :
    case DONT :
    case WILL :
    case WONT :
      buffer[2] = (BYTE)opt;
      return PTCPSocket::Write(buffer, 3);

    case InterruptProcess :
    case Break :
    case AbortProcess :
    case SuspendProcess :
    case AbortOutput :
      if (opt) {
        // Send the command
        if (!PTCPSocket::Write(buffer, 2))
          return PFalse;
        // Send a TimingMark for output flush.
        buffer[1] = TimingMark;
        if (!PTCPSocket::Write(buffer, 2))
          return PFalse;
        // Send a DataMark for synchronisation.
        if (cmd != AbortOutput) {
          buffer[1] = DataMark;
          if (!PTCPSocket::Write(buffer, 2))
            return PFalse;
          // Send the datamark character as the only out of band data byte.
          if (!WriteOutOfBand(&buffer[1], 1))
            return PFalse;
        }
        // Then flush any waiting input data.
        PTimeInterval oldTimeout = readTimeout;
        readTimeout = 0;
        while (PTCPSocket::Read(buffer, sizeof(buffer)))
          ;
        readTimeout = oldTimeout;
      }
      break;

    default :
      return PTCPSocket::Write(buffer, 2);
  }

  return PTrue;
}


static PString GetTELNETOptionName(PINDEX code)
{
  static const char * const name[] = {
    "TransmitBinary",
    "EchoOption",
    "ReconnectOption",
    "SuppressGoAhead",
    "MessageSizeOption",
    "StatusOption",
    "TimingMark",
    "RCTEOption",
    "OutputLineWidth",
    "OutputPageSize",
    "CRDisposition",
    "HorizontalTabsStops",
    "HorizTabDisposition",
    "FormFeedDisposition",
    "VerticalTabStops",
    "VertTabDisposition",
    "LineFeedDisposition",
    "ExtendedASCII",
    "ForceLogout",
    "ByteMacroOption",
    "DataEntryTerminal",
    "SupDupProtocol",
    "SupDupOutput",
    "SendLocation",
    "TerminalType",
    "EndOfRecordOption",
    "TACACSUID",
    "OutputMark",
    "TerminalLocation",
    "Use3270RegimeOption",
    "UseX3PADOption",
    "WindowSize",
    "TerminalSpeed",
    "FlowControl",
    "LineMode",
    "XDisplayLocation",
    "EnvironmentOption",
    "AuthenticateOption",
    "EncriptionOption"
  };

  if (code < PARRAYSIZE(name))
    return name[code];
  if (code == PTelnetSocket::ExtendedOptionsList)
    return "ExtendedOptionsList";
  return PString(PString::Printf, "Option #%u", code);
}


PBoolean PTelnetSocket::StartSend(const char * which, BYTE code)
{
  PTelnetError << which << ' ' << GetTELNETOptionName(code) << ' ';
  if (IsOpen())
    return PTrue;

  PDebugError << "not open yet." << endl;
  return SetErrorValues(NotOpen, EBADF);
}


PBoolean PTelnetSocket::SendDo(BYTE code)
{
  if (!StartSend("SendDo", code))
    return PFalse;

  OptionInfo & opt = option[code];

  switch (opt.theirState) {
    case OptionInfo::IsNo :
      PDebugError << "initiated.";
      SendCommand(DO, code);
      opt.theirState = OptionInfo::WantYes;
      break;

    case OptionInfo::IsYes :
      PDebugError << "already enabled." << endl;
      return PFalse;

    case OptionInfo::WantNo :
      PDebugError << "queued.";
      opt.theirState = OptionInfo::WantNoQueued;
      break;

    case OptionInfo::WantNoQueued :
      PDebugError << "already queued." << endl;
      opt.theirState = OptionInfo::IsNo;
      return PFalse;

    case OptionInfo::WantYes :
      PDebugError << "already negotiating." << endl;
      opt.theirState = OptionInfo::IsNo;
      return PFalse;

    case OptionInfo::WantYesQueued :
      PDebugError << "dequeued.";
      opt.theirState = OptionInfo::WantYes;
      break;
  }

  PDebugError << endl;
  return PTrue;
}


PBoolean PTelnetSocket::SendDont(BYTE code)
{
  if (!StartSend("SendDont", code))
    return PFalse;

  OptionInfo & opt = option[code];

  switch (opt.theirState) {
    case OptionInfo::IsNo :
      PDebugError << "already disabled." << endl;
      return PFalse;

    case OptionInfo::IsYes :
      PDebugError << "initiated.";
      SendCommand(DONT, code);
      opt.theirState = OptionInfo::WantNo;
      break;

    case OptionInfo::WantNo :
      PDebugError << "already negotiating." << endl;
      opt.theirState = OptionInfo::IsNo;
      return PFalse;

    case OptionInfo::WantNoQueued :
      PDebugError << "dequeued.";
      opt.theirState = OptionInfo::WantNo;
      break;

    case OptionInfo::WantYes :
      PDebugError << "queued.";
      opt.theirState = OptionInfo::WantYesQueued;
      break;

    case OptionInfo::WantYesQueued :
      PDebugError << "already queued." << endl;
      opt.theirState = OptionInfo::IsYes;
      return PFalse;
  }

  PDebugError << endl;
  return PTrue;
}


PBoolean PTelnetSocket::SendWill(BYTE code)
{
  if (!StartSend("SendWill", code))
    return PFalse;

  if (!IsOpen())
    return PFalse;

  OptionInfo & opt = option[code];

  switch (opt.ourState) {
    case OptionInfo::IsNo :
      PDebugError << "initiated.";
      SendCommand(WILL, code);
      opt.ourState = OptionInfo::WantYes;
      break;

    case OptionInfo::IsYes :
      PDebugError << "already enabled." << endl;
      return PFalse;

    case OptionInfo::WantNo :
      PDebugError << "queued.";
      opt.ourState = OptionInfo::WantNoQueued;
      break;

    case OptionInfo::WantNoQueued :
      PDebugError << "already queued." << endl;
      opt.ourState = OptionInfo::IsNo;
      return PFalse;

    case OptionInfo::WantYes :
      PDebugError << "already negotiating." << endl;
      opt.ourState = OptionInfo::IsNo;
      return PFalse;

    case OptionInfo::WantYesQueued :
      PDebugError << "dequeued.";
      opt.ourState = OptionInfo::WantYes;
      break;
  }

  PDebugError << endl;
  return PTrue;
}


PBoolean PTelnetSocket::SendWont(BYTE code)
{
  if (!StartSend("SendWont", code))
    return PFalse;

  OptionInfo & opt = option[code];

  switch (opt.ourState) {
    case OptionInfo::IsNo :
      PDebugError << "already disabled." << endl;
      return PFalse;

    case OptionInfo::IsYes :
      PDebugError << "initiated.";
      SendCommand(WONT, code);
      opt.ourState = OptionInfo::WantNo;
      break;

    case OptionInfo::WantNo :
      PDebugError << "already negotiating." << endl;
      opt.ourState = OptionInfo::IsNo;
      return PFalse;

    case OptionInfo::WantNoQueued :
      PDebugError << "dequeued.";
      opt.ourState = OptionInfo::WantNo;
      break;

    case OptionInfo::WantYes :
      PDebugError << "queued.";
      opt.ourState = OptionInfo::WantYesQueued;
      break;

    case OptionInfo::WantYesQueued :
      PDebugError << "already queued." << endl;
      opt.ourState = OptionInfo::IsYes;
      return PFalse;
  }

  PDebugError << endl;
  return PTrue;
}


PBoolean PTelnetSocket::SendSubOption(BYTE code,
                                    const BYTE * info, PINDEX len, int subCode)
{
  if (!StartSend("SendSubOption", code))
    return PFalse;

  PDebugError << "with " << len << " bytes." << endl;

  PBYTEArray buffer(len + 6);
  buffer[0] = IAC;
  buffer[1] = SB;
  buffer[2] = code;
  PINDEX i = 3;
  if (subCode >= 0)
    buffer[i++] = (BYTE)subCode;
  while (len-- > 0) {
    if (*info == IAC)
      buffer[i++] = IAC;
    buffer[i++] = *info++;
  }
  buffer[i++] = IAC;
  buffer[i++] = SE;

  return PTCPSocket::Write((const BYTE *)buffer, i);
}


void PTelnetSocket::SetTerminalType(const PString & newType)
{
  terminalType = newType;
}


void PTelnetSocket::SetWindowSize(WORD width, WORD height)
{
  windowWidth = width;
  windowHeight = height;
  if (IsOurOption(WindowSize)) {
    BYTE buffer[4];
    buffer[0] = (BYTE)(width >> 8);
    buffer[1] = (BYTE)width;
    buffer[2] = (BYTE)(height >> 8);
    buffer[3] = (BYTE)height;
    SendSubOption(WindowSize, buffer, 4);
  }
  else {
    SetOurOption(WindowSize);
    SendWill(WindowSize);
  }
}


void PTelnetSocket::GetWindowSize(WORD & width, WORD & height) const
{
  width = windowWidth;
  height = windowHeight;
}


PBoolean PTelnetSocket::Read(void * data, PINDEX bytesToRead)
{
  PBYTEArray buffer(bytesToRead);
  PINDEX charsLeft = bytesToRead;
  BYTE * dst = (BYTE *)data;

  while (charsLeft > 0) {
    BYTE * src = buffer.GetPointer(charsLeft);
    if (!PTCPSocket::Read(src, charsLeft)) {
      lastReadCount = bytesToRead - charsLeft;
      return lastReadCount > 0;
    }

    while (lastReadCount > 0) {
      BYTE currentByte = *src++;
      lastReadCount--;
      switch (state) {
        case StateCarriageReturn :
          state = StateNormal;
          if (currentByte == '\0')
            break; // Ignore \0 after CR
          // Else, fall through for normal processing

        case StateNormal :
          if (currentByte == IAC)
            state = StateIAC;
          else {
            if (currentByte == '\r' && !IsTheirOption(TransmitBinary))
              state = StateCarriageReturn;
            *dst++ = currentByte;
            charsLeft--;
          }
          break;

        case StateIAC :
          switch (currentByte) {
            case IAC :
              state = StateNormal;
              *dst++ = IAC;
              charsLeft--;
              break;

            case DO :
              state = StateDo;
              break;

            case DONT :
              state = StateDont;
              break;

            case WILL :
              state = StateWill;
              break;

            case WONT :
              state = StateWont;
              break;

            case DataMark :    // data stream portion of a Synch
              /* We may have missed an urgent notification, so make sure we
                 flush whatever is in the buffer currently.
               */
              PTelnetError << "received DataMark" << endl;
              if (synchronising > 0)
                synchronising--;
              break;

            case SB :          // subnegotiation start
              state = StateSubNegotiations;
              subOption.SetSize(0);
              break;

            default:
              if (OnCommand(currentByte))
                state = StateNormal;
              break;
          }
          break;

        case StateDo :
          OnDo(currentByte);
          state = StateNormal;
          break;

        case StateDont :
          OnDont(currentByte);
          state = StateNormal;
          break;

        case StateWill :
          OnWill(currentByte);
          state = StateNormal;
          break;

        case StateWont :
          OnWont(currentByte);
          state = StateNormal;
          break;

        case StateSubNegotiations :
          if (currentByte == IAC)
            state = StateEndNegotiations;
          else
            subOption[subOption.GetSize()] = currentByte;
          break;

        case StateEndNegotiations :
          if (currentByte == SE)
            state = StateNormal;
          else if (currentByte != IAC) {
            /* This is an error.  We only expect to get "IAC IAC" or "IAC SE".
               Several things may have happend.  An IAC was not doubled, the
               IAC SE was left off, or another option got inserted into the
               suboption are all possibilities. If we assume that the IAC was
               not doubled, and really the IAC SE was left off, we could get
               into an infinate loop here.  So, instead, we terminate the
               suboption, and process the partial suboption if we can.
             */
            state = StateIAC;
            src--;  // Go back to character for IAC ccommand
          }
          else {
            subOption[subOption.GetSize()] = currentByte;
            state = StateSubNegotiations;
            break;  // Was IAC IAC, subnegotiation not over yet.
          }
          if (subOption.GetSize() > 1 && IsOurOption(subOption[0]))
            OnSubOption(subOption[0],
                            ((const BYTE*)subOption)+1, subOption.GetSize()-1);
          break;

        default :
          PTelnetError << "illegal state: " << (int)state << endl;
          state = StateNormal;
      }
      if (synchronising > 0) {
        charsLeft = bytesToRead;    // Flush data being received.
        dst = (BYTE *)data;
      }
    }
  }
  lastReadCount = bytesToRead;
  return PTrue;
}


void PTelnetSocket::OnDo(BYTE code)
{
  PTelnetError << "OnDo " << GetTELNETOptionName(code) << ' ';

  OptionInfo & opt = option[code];

  switch (opt.ourState) {
    case OptionInfo::IsNo :
      if (opt.weCan) {
        PDebugError << "WILL.";
        SendCommand(WILL, code);
        opt.ourState = OptionInfo::IsYes;
      }
      else {
        PDebugError << "WONT.";
        SendCommand(WONT, code);
      }
      break;

    case OptionInfo::IsYes :
      PDebugError << "ignored.";
      break;

    case OptionInfo::WantNo :
      PDebugError << "is answer to WONT.";
      opt.ourState = OptionInfo::IsNo;
      break;

    case OptionInfo::WantNoQueued :
      PDebugError << "impossible answer.";
      opt.ourState = OptionInfo::IsYes;
      break;

    case OptionInfo::WantYes :
      PDebugError << "accepted.";
      opt.ourState = OptionInfo::IsYes;
      break;

    case OptionInfo::WantYesQueued :
      PDebugError << "refused.";
      opt.ourState = OptionInfo::WantNo;
      SendCommand(WONT, code);
      break;
  }

  PDebugError << endl;

  if (IsOurOption(code)) {
    switch (code) {
      case TerminalSpeed : {
          static BYTE defSpeed[] = "38400,38400";
          SendSubOption(TerminalSpeed,defSpeed,sizeof(defSpeed)-1,SubOptionIs);
        }
        break;

      case TerminalType :
        SendSubOption(TerminalType,
                          terminalType, terminalType.GetLength(), SubOptionIs);
        break;

      case WindowSize :
        SetWindowSize(windowWidth, windowHeight);
        break;
    }
  }
}


void PTelnetSocket::OnDont(BYTE code)
{
  PTelnetError << "OnDont " << GetTELNETOptionName(code) << ' ';

  OptionInfo & opt = option[code];

  switch (opt.ourState) {
    case OptionInfo::IsNo :
      PDebugError << "ignored.";
      break;

    case OptionInfo::IsYes :
      PDebugError << "WONT.";
      opt.ourState = OptionInfo::IsNo;
      SendCommand(WONT, code);
      break;

    case OptionInfo::WantNo :
      PDebugError << "disabled.";
      opt.ourState = OptionInfo::IsNo;
      break;

    case OptionInfo::WantNoQueued :
      PDebugError << "accepting.";
      opt.ourState = OptionInfo::WantYes;
      SendCommand(DO, code);
      break;

    case OptionInfo::WantYes :
      PDebugError << "queued disable.";
      opt.ourState = OptionInfo::IsNo;
      break;

    case OptionInfo::WantYesQueued :
      PDebugError << "refused.";
      opt.ourState = OptionInfo::IsNo;
      break;
  }

  PDebugError << endl;
}


void PTelnetSocket::OnWill(BYTE code)
{
  PTelnetError << "OnWill " << GetTELNETOptionName(code) << ' ';

  OptionInfo & opt = option[code];

  switch (opt.theirState) {
    case OptionInfo::IsNo :
      if (opt.theyShould) {
        PDebugError << "DO.";
        SendCommand(DO, code);
        opt.theirState = OptionInfo::IsYes;
      }
      else {
        PDebugError << "DONT.";
        SendCommand(DONT, code);
      }
      break;

    case OptionInfo::IsYes :
      PDebugError << "ignored.";
      break;

    case OptionInfo::WantNo :
      PDebugError << "is answer to DONT.";
      opt.theirState = OptionInfo::IsNo;
      break;

    case OptionInfo::WantNoQueued :
      PDebugError << "impossible answer.";
      opt.theirState = OptionInfo::IsYes;
      break;

    case OptionInfo::WantYes :
      PDebugError << "accepted.";
      opt.theirState = OptionInfo::IsYes;
      break;

    case OptionInfo::WantYesQueued :
      PDebugError << "refused.";
      opt.theirState = OptionInfo::WantNo;
      SendCommand(DONT, code);
      break;
  }

  PDebugError << endl;
}


void PTelnetSocket::OnWont(BYTE code)
{
  PTelnetError << "OnWont " << GetTELNETOptionName(code) << ' ';

  OptionInfo & opt = option[code];

  switch (opt.theirState) {
    case OptionInfo::IsNo :
      PDebugError << "ignored.";
      break;

    case OptionInfo::IsYes :
      PDebugError << "DONT.";
      opt.theirState = OptionInfo::IsNo;
      SendCommand(DONT, code);
      break;

    case OptionInfo::WantNo :
      PDebugError << "disabled.";
      opt.theirState = OptionInfo::IsNo;
      break;

    case OptionInfo::WantNoQueued :
      PDebugError << "accepting.";
      opt.theirState = OptionInfo::WantYes;
      SendCommand(DO, code);
      break;

    case OptionInfo::WantYes :
      PDebugError << "refused.";
      opt.theirState = OptionInfo::IsNo;
      break;

    case OptionInfo::WantYesQueued :
      PDebugError << "queued refusal.";
      opt.theirState = OptionInfo::IsNo;
      break;
  }

  PDebugError << endl;
}


void PTelnetSocket::OnSubOption(BYTE code, const BYTE * info, PINDEX len)
{
  PTelnetError << "OnSubOption " << GetTELNETOptionName(code)
               << " of " << len << " bytes." << endl;
  switch (code) {
    case TerminalType :
      if (*info == SubOptionSend)
        SendSubOption(TerminalType,
                          terminalType, terminalType.GetLength(), SubOptionIs);
      break;

    case TerminalSpeed :
      if (*info == SubOptionSend) {
        static BYTE defSpeed[] = "38400,38400";
        SendSubOption(TerminalSpeed,defSpeed,sizeof(defSpeed)-1,SubOptionIs);
      }
      break;
  }
}


PBoolean PTelnetSocket::OnCommand(BYTE code)
{
  if (code == NOP)
    return PTrue;
  PTelnetError << "unknown command " << (int)code << endl;
  return PTrue;
}

void PTelnetSocket::OnOutOfBand(const void *, PINDEX length)
{
  PTelnetError << "out of band data received of length " << length << endl;
  synchronising++;
}


// End Of File ///////////////////////////////////////////////////////////////
