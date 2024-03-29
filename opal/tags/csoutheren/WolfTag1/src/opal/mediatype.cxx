/*
 * mediatype.h
 *
 * Media Format Type descriptions
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (C) 2007 Post Increment and Hannes Friederich
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
 * The Original Code is OPAL
 *
 * The Initial Developer of the Original Code is Hannes Friederich and Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "mediatype.h"
#endif

#include <opal/buildopts.h>

#include <opal/mediatype.h>
#include <opal/rtpconn.h>
#include <opal/call.h>

#if OPAL_H323
#include <h323/channels.h>
#endif

namespace PWLibStupidLinkerHacks {
  int mediaTypeLoader;
}; // namespace PWLibStupidLinkerHacks


OPAL_INSTANTIATE_MEDIATYPE(audio, OpalAudioMediaType);

#if OPAL_VIDEO
OPAL_INSTANTIATE_MEDIATYPE(video, OpalVideoMediaType);
#endif

OPAL_INSTANTIATE_SIMPLE_MEDIATYPE_NO_SDP(userinput); 

///////////////////////////////////////////////////////////////////////////////

ostream & operator << (ostream & strm, const OpalMediaType & mediaType)
{ mediaType.PrintOn(strm); return strm; }

const OpalMediaType & OpalMediaType::Audio()     { static const OpalMediaType type = "audio";     return type; }
const OpalMediaType & OpalMediaType::Video()     { static const OpalMediaType type = "video";     return type; }
const OpalMediaType & OpalMediaType::Fax()       { static const OpalMediaType type = "fax";       return type; };
const OpalMediaType & OpalMediaType::UserInput() { static const OpalMediaType type = "userinput"; return type; };

///////////////////////////////////////////////////////////////////////////////

OpalMediaTypeDefinition * OpalMediaType::GetDefinition() const
{
  return OpalMediaTypeFactory::CreateInstance(*this);
}


OpalMediaTypeDefinition * OpalMediaType::GetDefinition(const OpalMediaType & key)
{
  return OpalMediaTypeFactory::CreateInstance(key);
}

///////////////////////////////////////////////////////////////////////////////

#if OPAL_SIP
OpalMediaTypeDefinition::OpalMediaTypeDefinition(const char * _mediaType, const char * _sdpType, unsigned preferredSessionId)
  : mediaType(_mediaType), sdpType(_sdpType != NULL ? _sdpType : "")
#else
OpalMediaTypeDefinition::OpalMediaTypeDefinition(const char * _mediaType, const char *         , unsigned preferredSessionId)
#endif
{
  PWaitAndSignal m(GetMapMutex());

  SessionIDToMediaTypeMap_T & typeMap = GetSessionIDToMediaTypeMap();

  // if the preferred session ID is already taken, then find a new one
  if (typeMap.find(preferredSessionId) != typeMap.end())
    preferredSessionId = 0;

  // if no preferred session Id 
  if (preferredSessionId == 0) {
    preferredSessionId = 1;
    while (typeMap.find(preferredSessionId) != typeMap.end())
      ++preferredSessionId;
  }

  typeMap.insert(SessionIDToMediaTypeMap_T::value_type(preferredSessionId, _mediaType));
  GetMediaTypeToSessionIDMap().insert(MediaTypeToSessionIDMap_T::value_type(_mediaType, preferredSessionId));
}

unsigned OpalMediaTypeDefinition::GetDefaultSessionId(const OpalMediaType & mediaType)
{
  PWaitAndSignal m(GetMapMutex());

  MediaTypeToSessionIDMap_T::iterator r = GetMediaTypeToSessionIDMap().find(mediaType);
  if (r != GetMediaTypeToSessionIDMap().end())
    return r->second;

  return 0;
}


OpalMediaType OpalMediaTypeDefinition::GetMediaTypeForSessionId(unsigned sessionId)
{
  PWaitAndSignal m(GetMapMutex());

  SessionIDToMediaTypeMap_T::iterator r = GetSessionIDToMediaTypeMap().find(sessionId);
  if (r != GetSessionIDToMediaTypeMap().end())
    return r->second;

  return OpalMediaType();
}


OpalMediaTypeDefinition::SessionIDToMediaTypeMap_T & OpalMediaTypeDefinition::GetSessionIDToMediaTypeMap()
{
  PWaitAndSignal m(GetMapMutex());
  static SessionIDToMediaTypeMap_T map;
  return map;
}

OpalMediaTypeDefinition::MediaTypeToSessionIDMap_T & OpalMediaTypeDefinition::GetMediaTypeToSessionIDMap()
{
  PWaitAndSignal m(GetMapMutex());
  static MediaTypeToSessionIDMap_T map;
  return map;
}


PMutex & OpalMediaTypeDefinition::GetMapMutex()
{
  static PMutex mutex;
  return mutex;
}


RTP_UDP * OpalMediaTypeDefinition::CreateRTPSession(OpalRTPConnection & connection,
#if OPAL_RTP_AGGREGATE
                                                    PHandleAggregator * agg,
#endif
                                                    OpalSecurityMode * securityMode,
                                                    unsigned sessionID,
                                                    bool remoteIsNAT)
{
  RTP_Session::Params params;
  params.id = sessionID;
  params.encoding = GetRTPEncoding();
#if OPAL_RTP_AGGREGATE
  params.aggregator = agg;
#endif
  params.isAudio = mediaType == OpalMediaType::Audio();
  params.remoteIsNAT = remoteIsNAT;

  if (securityMode != NULL) 
    return securityMode->CreateRTPSession(connection, params);

  return new RTP_UDP(params);
}

///////////////////////////////////////////////////////////////////////////////

OpalRTPAVPMediaType::OpalRTPAVPMediaType(const char * mediaType, const char * sdpType, unsigned preferredSessionId)
  : OpalMediaTypeDefinition(mediaType, sdpType, preferredSessionId)
{
}

PString OpalRTPAVPMediaType::GetRTPEncoding() const
{
  return "rtp/avp";
}

///////////////////////////////////////////////////////////////////////////////

OpalAudioMediaType::OpalAudioMediaType()
  : OpalRTPAVPMediaType("audio", "audio", 1)
{
}


///////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO

OpalVideoMediaType::OpalVideoMediaType()
  : OpalRTPAVPMediaType("video", "video", 2)
{ }

#endif // OPAL_VIDEO

///////////////////////////////////////////////////////////////////////////////

