/*
 * shttpsvc.h
 *
 * Class for secure service applications using HTTPS as the user interface.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-2001 Equivalence Pty. Ltd.
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
 * $Log: shttpsvc.h,v $
 * Revision 1.3  2001/05/16 06:02:05  craigs
 * Changed to allow detection of non-SSL connection to SecureHTTPServiceProcess
 *
 * Revision 1.2  2001/03/27 03:56:01  craigs
 * Added hack to allow secure servers to act as non-secure servers
 *
 * Revision 1.1  2001/02/15 02:41:14  robertj
 * Added class to do secure HTTP based service process.
 *
 */

#ifndef SHTTPSVC_H
#define SHTTPSVC_H

#include <ptclib/httpsvc.h>
#include <ptclib/pssl.h>


/////////////////////////////////////////////////////////////////////

class PSecureHTTPServiceProcess : public PHTTPServiceProcess
{
  PCLASSINFO(PSecureHTTPServiceProcess, PHTTPServiceProcess)

  public:
    PSecureHTTPServiceProcess(const Info & inf);
    ~PSecureHTTPServiceProcess();

    virtual PHTTPServer * CreateHTTPServer(PTCPSocket & socket);

    BOOL SetServerCertificate(
      const PFilePath & certFile
    );

    virtual BOOL OnDetectedNonSSLConnection(PChannel * chan, const PString & line);

    virtual PString CreateNonSSLMessage(const PString & url);
    virtual PString CreateRedirectMessage(const PString & url);

  protected:
    PSSLContext * sslContext;

#ifdef _DEBUG
    static BOOL secureServerHack;
#endif
};


#endif // SHTTPSVC_H
