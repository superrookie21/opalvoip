/*
 * pcss.cxx
 *
 * PC Sound system support.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2000 Equivalence Pty. Ltd.
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: pcss.cxx,v $
 * Revision 1.2030.2.1  2006/12/08 06:27:20  csoutheren
 * Fix compilation problem caused by bad patch backports
 * Allow compilation with latest PWLib
 *
 * Revision 2.29  2005/12/29 16:23:09  dsandras
 * Use the correct clock rate for the canceller.
 *
 * Revision 2.28  2005/12/28 20:03:00  dsandras
 * Attach the silence detector in OnPatchMediaStream so that it can be attached
 * before the echo cancellation filter.
 *
 * Revision 2.27  2005/12/27 22:25:55  dsandras
 * Added propagation of the callback to the pcss endpoint.
 *
 * Revision 2.26  2005/12/27 20:48:43  dsandras
 * Added media format parameter when opening the sound channel so that its
 * parameters can be used in the body of the method.
 * Added wideband support when opening the channel.
 *
 * Revision 2.25  2005/11/24 20:31:55  dsandras
 * Added support for echo cancelation using Speex.
 * Added possibility to add a filter to an OpalMediaPatch for all patches of a connection.
 *
 * Revision 2.24  2005/10/12 21:11:21  dsandras
 * Control if the video streams are started or not from this class.
 *
 * Revision 2.23  2005/07/24 07:28:29  rjongbloed
 * Fixed inclusion of silence detecter in media stream if is audio only.
 *
 * Revision 2.22  2005/02/19 22:44:41  dsandras
 * Changed pointer to PSafeReadOnly to avoid deadlock on incoming calls.
 *
 * Revision 2.21  2004/12/12 13:36:22  dsandras
 * Propagate the remote application and remote party address to the PCSS endpoint.
 *
 * Revision 2.20  2004/08/14 07:56:43  rjongbloed
 * Major revision to utilise the PSafeCollection classes for the connections and calls.
 *
 * Revision 2.19  2004/07/14 13:26:14  rjongbloed
 * Fixed issues with the propagation of the "established" phase of a call. Now
 *   calling an OnEstablished() chain like OnAlerting() and OnConnected() to
 *   finally arrive at OnEstablishedCall() on OpalManager
 *
 * Revision 2.18  2004/07/11 12:42:13  rjongbloed
 * Added function on endpoints to get the list of all media formats any
 *   connection the endpoint may create can support.
 *
 * Revision 2.17  2004/05/24 13:52:38  rjongbloed
 * Added a separate structure for the silence suppression paramters to make
 *   it easier to set from global defaults in the manager class.
 *
 * Revision 2.16  2004/05/17 13:24:18  rjongbloed
 * Added silence suppression.
 *
 * Revision 2.15  2004/04/18 13:35:28  rjongbloed
 * Fixed ability to make calls where both endpoints are specified a priori. In particular
 *   fixing the VXML support for an outgoing sip/h323 call.
 *
 * Revision 2.14  2004/02/03 12:21:30  rjongbloed
 * Fixed random destination alias from being interpreted as a sound card device name, unless it is a valid sound card device name.
 *
 * Revision 2.13  2003/03/17 22:27:36  robertj
 * Fixed multi-byte character that should have been string.
 *
 * Revision 2.12  2003/03/17 10:13:18  robertj
 * Added call back functions for creating sound channel.
 * Added video support.
 *
 * Revision 2.11  2003/03/07 08:14:50  robertj
 * Fixed support of dual address SetUpCall()
 * Fixed use of new call set up sequence, calling OnSetUp() in outgoing call.
 *
 * Revision 2.10  2003/03/06 03:57:47  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 * Revision 2.9  2002/07/04 07:54:53  robertj
 * Fixed GNU warning
 *
 * Revision 2.8  2002/06/16 02:19:55  robertj
 * Fixed and clarified function for initiating call, thanks Ted Szoczei
 *
 * Revision 2.7  2002/01/22 05:13:47  robertj
 * Revamp of user input API triggered by RFC2833 support
 *
 * Revision 2.6  2001/11/13 06:25:56  robertj
 * Changed SetUpConnection() so returns BOOL as returning
 *   pointer to connection is not useful.
 *
 * Revision 2.5  2001/10/15 04:33:17  robertj
 * Removed answerCall signal and replaced with state based functions.
 *
 * Revision 2.4  2001/08/23 03:15:51  robertj
 * Added missing Lock() calls in SetUpConnection
 *
 * Revision 2.3  2001/08/21 01:13:22  robertj
 * Fixed propagation of sound channel buffers through media stream.
 * Allowed setting of sound card by number using #1 etc.
 * Fixed calling OnAlerting when alerting.
 *
 * Revision 2.2  2001/08/17 08:33:50  robertj
 * More implementation.
 *
 * Revision 2.1  2001/08/01 05:45:01  robertj
 * Moved media formats list from endpoint to connection.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.1  2000/02/19 15:34:52  robertj
 * Major upgrade to OPAL (Open Phone Abstraction Library). HUGE CHANGES!!!!
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "pcss.h"
#endif

#include <opal/pcss.h>

#include <ptlib/videoio.h>
#include <ptlib/sound.h>

#include <codec/silencedetect.h>
#include <codec/echocancel.h>
#include <codec/vidcodec.h>
#include <opal/call.h>
#include <opal/patch.h>
#include <opal/manager.h>


#define new PNEW


static PString MakeToken(const PString & playDevice, const PString & recordDevice)
{
  if (playDevice == recordDevice)
    return recordDevice;
  else
    return playDevice + '\\' + recordDevice;
}


/////////////////////////////////////////////////////////////////////////////

OpalPCSSEndPoint::OpalPCSSEndPoint(OpalManager & mgr, const char * prefix)
  : OpalEndPoint(mgr, prefix, CanTerminateCall),
    soundChannelPlayDevice(PSoundChannel::GetDefaultDevice(PSoundChannel::Player)),
    soundChannelRecordDevice(PSoundChannel::GetDefaultDevice(PSoundChannel::Recorder))
{
#ifdef _WIN32
  // Windows MultMedia stuff seems to need greater depth due to enormous
  // latencies in its operation, need to use DirectSound maybe?
  soundChannelBuffers = 3;
#else
  // Should only need double buffering for Unix platforms
  soundChannelBuffers = 2;
#endif

  PTRACE(3, "PCSS\tCreated PC sound system endpoint.");
}


OpalPCSSEndPoint::~OpalPCSSEndPoint()
{
  PTRACE(3, "PCSS\tDeleted PC sound system endpoint.");
}


static BOOL SetDeviceName(const PString & name,
                          PSoundChannel::Directions dir,
                          PString & result)
{
  PStringArray devices = PSoundChannel::GetDeviceNames(dir);

  if (name[0] == '#') {
    PINDEX id = name.Mid(1).AsUnsigned();
    if (id == 0 || id > devices.GetSize())
      return FALSE;
    result = devices[id-1];
  }
  else {
    if (devices.GetValuesIndex(name) == P_MAX_INDEX)
      return FALSE;
    result = name;
  }

  return TRUE;
}


BOOL OpalPCSSEndPoint::MakeConnection(OpalCall & call,
                                      const PString & remoteParty,
                                      void * userData)
{
  // First strip of the prefix if present
  PINDEX prefixLength = 0;
  if (remoteParty.Find(GetPrefixName()+":") == 0)
    prefixLength = GetPrefixName().GetLength()+1;

  PString playDevice;
  PString recordDevice;
  PINDEX separator = remoteParty.FindOneOf("\n\t\\", prefixLength);
  if (separator == P_MAX_INDEX)
    playDevice = recordDevice = remoteParty.Mid(prefixLength);
  else {
    playDevice = remoteParty(prefixLength+1, separator-1);
    recordDevice = remoteParty.Mid(separator+1);
  }

  if (!SetDeviceName(playDevice, PSoundChannel::Player, playDevice))
    playDevice = soundChannelPlayDevice;
  if (!SetDeviceName(recordDevice, PSoundChannel::Recorder, recordDevice))
    recordDevice = soundChannelRecordDevice;

  PSafePtr<OpalPCSSConnection> connection = GetPCSSConnectionWithLock(MakeToken(playDevice, recordDevice));
  if (connection != NULL)
    return FALSE;

  connection = CreateConnection(call, playDevice, recordDevice, userData);
  if (connection == NULL)
    return FALSE;

  connectionsActive.SetAt(connection->GetToken(), connection);

  // If we are the A-party then need to initiate a call now in this thread and
  // go through the routing engine via OnIncomingConnection. If we are the
  // B-Party then SetUpConnection() gets called in the context of the A-party
  // thread.
  if (call.GetConnection(0) == connection)
    connection->InitiateCall();

  return TRUE;
}


OpalMediaFormatList OpalPCSSEndPoint::GetMediaFormats() const
{
  OpalMediaFormatList formats;

  formats += OpalPCM16; // Sound card can only do 16 bit PCM

  AddVideoMediaFormats(formats);

  return formats;
}


OpalPCSSConnection * OpalPCSSEndPoint::CreateConnection(OpalCall & call,
                                                        const PString & playDevice,
                                                        const PString & recordDevice,
                                                        void * /*userData*/)
{
  return new OpalPCSSConnection(call, *this, playDevice, recordDevice);
}


PSoundChannel * OpalPCSSEndPoint::CreateSoundChannel(const OpalPCSSConnection & connection,
						     const OpalMediaFormat & mediaFormat,
                                                     BOOL isSource)
{
  PString deviceName;
  if (isSource)
    deviceName = connection.GetSoundChannelRecordDevice();
  else
    deviceName = connection.GetSoundChannelPlayDevice();

  PSoundChannel * soundChannel = new PSoundChannel;

  if (soundChannel->Open(deviceName,
                         isSource ? PSoundChannel::Recorder
                                  : PSoundChannel::Player,
                         1, mediaFormat.GetClockRate(), 16)) {
    PTRACE(3, "PCSS\tOpened sound channel \"" << deviceName
           << "\" for " << (isSource ? "record" : "play") << "ing.");
    return soundChannel;
  }

  PTRACE(1, "PCSS\tCould not open sound channel \"" << deviceName
         << "\" for " << (isSource ? "record" : "play")
         << "ing: " << soundChannel->GetErrorText());

  delete soundChannel;
  return NULL;
}


void OpalPCSSEndPoint::AcceptIncomingConnection(const PString & token)
{
  PSafePtr<OpalPCSSConnection> connection = GetPCSSConnectionWithLock(token, PSafeReadOnly);
  if (connection != NULL)
    connection->AcceptIncoming();
}


BOOL OpalPCSSEndPoint::OnShowUserInput(const OpalPCSSConnection &, const PString &)
{
  return TRUE;
}


void OpalPCSSEndPoint::OnPatchMediaStream(const OpalPCSSConnection & /*connection*/, BOOL /*isSource*/, OpalMediaPatch & /*patch*/)
{
}


BOOL OpalPCSSEndPoint::SetSoundChannelPlayDevice(const PString & name)
{
  return SetDeviceName(name, PSoundChannel::Player, soundChannelPlayDevice);
}


BOOL OpalPCSSEndPoint::SetSoundChannelRecordDevice(const PString & name)
{
  return SetDeviceName(name, PSoundChannel::Recorder, soundChannelRecordDevice);
}


void OpalPCSSEndPoint::SetSoundChannelBufferDepth(unsigned depth)
{
  PAssert(depth > 1, PInvalidParameter);
  soundChannelBuffers = depth;
}


/////////////////////////////////////////////////////////////////////////////

OpalPCSSConnection::OpalPCSSConnection(OpalCall & call,
                                       OpalPCSSEndPoint & ep,
                                       const PString & playDevice,
                                       const PString & recordDevice)
  : OpalConnection(call, ep, MakeToken(playDevice, recordDevice)),
    endpoint(ep),
    soundChannelPlayDevice(playDevice),
    soundChannelRecordDevice(recordDevice),
    soundChannelBuffers(ep.GetSoundChannelBufferDepth())
{
  silenceDetector = new OpalPCM16SilenceDetector;
  echoCanceler = new OpalEchoCanceler;

  PTRACE(3, "PCSS\tCreated PC sound system connection.");
}


OpalPCSSConnection::~OpalPCSSConnection()
{
  PTRACE(3, "PCSS\tDeleted PC sound system connection.");
}


BOOL OpalPCSSConnection::SetUpConnection()
{
  remotePartyName = ownerCall.GetOtherPartyConnection(*this)->GetRemotePartyName();
  remotePartyAddress = ownerCall.GetOtherPartyConnection(*this)->GetRemotePartyAddress();
  remoteApplication = ownerCall.GetOtherPartyConnection(*this)->GetRemoteApplication();

  PTRACE(3, "PCSS\tSetUpConnection(" << remotePartyName << ')');
  phase = AlertingPhase;
  endpoint.OnShowIncoming(*this);
  OnAlerting();

  return TRUE;
}


BOOL OpalPCSSConnection::SetAlerting(const PString & calleeName, BOOL)
{
  PTRACE(3, "PCSS\tSetAlerting(" << calleeName << ')');
  phase = AlertingPhase;
  remotePartyName = calleeName;
  return endpoint.OnShowOutgoing(*this);
}


BOOL OpalPCSSConnection::SetConnected()
{
  PTRACE(3, "PCSS\tSetConnected()");

  if (mediaStreams.IsEmpty())
    phase = ConnectedPhase;
  else {
    phase = EstablishedPhase;
    OnEstablished();
  }

  return TRUE;
}


PString OpalPCSSConnection::GetDestinationAddress()
{
  return endpoint.OnGetDestination(*this);
}


OpalMediaFormatList OpalPCSSConnection::GetMediaFormats() const
{
 
  OpalMediaFormatList formats;

  formats += OpalPCM16; // Sound card can only do 16 bit PCM

  AddVideoMediaFormats(formats);

  return formats;
}


OpalMediaStream * OpalPCSSConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                        unsigned sessionID,
                                                        BOOL isSource)
{
  if (sessionID != OpalMediaFormat::DefaultAudioSessionID)
    return OpalConnection::CreateMediaStream(mediaFormat, sessionID, isSource);

  PSoundChannel * soundChannel = CreateSoundChannel(mediaFormat, isSource);
  if (soundChannel == NULL)
    return NULL;

  return new OpalAudioMediaStream(mediaFormat, sessionID, isSource, soundChannelBuffers, soundChannel);
}


void OpalPCSSConnection::OnPatchMediaStream(BOOL isSource,
					    OpalMediaPatch & patch)
{
  PTRACE(3, "OpalCon\tNew patch created");
  int clockRate;
  if (patch.GetSource().GetSessionID() == OpalMediaFormat::DefaultAudioSessionID) {
    if (isSource) {
      silenceDetector->SetParameters(endpoint.GetManager().GetSilenceDetectParams());
      patch.AddFilter(silenceDetector->GetReceiveHandler(), OpalPCM16);
    }
    clockRate = patch.GetSource().GetMediaFormat().GetClockRate();
    echoCanceler->SetParameters(endpoint.GetManager().GetEchoCancelParams());
    echoCanceler->SetClockRate(clockRate);
    patch.AddFilter(isSource?echoCanceler->GetReceiveHandler():echoCanceler->GetSendHandler(), OpalPCM16);
  }

  endpoint.OnPatchMediaStream(*this, isSource, patch);
}


BOOL OpalPCSSConnection::OpenSourceMediaStream(const OpalMediaFormatList & mediaFormats,
					       unsigned sessionID)
{
  if (sessionID == OpalMediaFormat::DefaultVideoSessionID && !endpoint.GetManager().CanAutoStartTransmitVideo())
    return FALSE;

  return OpalConnection::OpenSourceMediaStream(mediaFormats, sessionID);
}


OpalMediaStream * OpalPCSSConnection::OpenSinkMediaStream(OpalMediaStream & source)
{
  if (source.GetSessionID() == OpalMediaFormat::DefaultVideoSessionID && !endpoint.GetManager().CanAutoStartReceiveVideo())
    return NULL;

  return OpalConnection::OpenSinkMediaStream(source);
}


PSoundChannel * OpalPCSSConnection::CreateSoundChannel(const OpalMediaFormat & mediaFormat, BOOL isSource)
{
  return endpoint.CreateSoundChannel(*this, mediaFormat, isSource);
}


BOOL OpalPCSSConnection::SendUserInputString(const PString & value)
{
  PTRACE(3, "PCSS\tSendUserInputString(" << value << ')');
  return endpoint.OnShowUserInput(*this, value);
}


void OpalPCSSConnection::InitiateCall()
{
  phase = SetUpPhase;
  if (!OnIncomingConnection())
    Release(EndedByCallerAbort);
  else {
    PTRACE(2, "PCSS\tOutgoing call routed to " << ownerCall.GetPartyB() << " for " << *this);
    if (!ownerCall.OnSetUp(*this))
      Release(EndedByNoAccept);
  }
}


void OpalPCSSConnection::AcceptIncoming()
{
  if (!LockReadOnly())
    return;

  if (phase != AlertingPhase) {
    UnlockReadOnly();
    return;
  }

  LockReadWrite();
  phase = ConnectedPhase;
  UnlockReadWrite();
  UnlockReadOnly();

  OnConnected();

  if (!LockReadOnly())
    return;

  if (mediaStreams.IsEmpty()) {
    UnlockReadOnly();
    return;
  }

  LockReadWrite();
  phase = EstablishedPhase;
  UnlockReadWrite();
  UnlockReadOnly();

  OnEstablished();
}

/////////////////////////////////////////////////////////////////////////////
