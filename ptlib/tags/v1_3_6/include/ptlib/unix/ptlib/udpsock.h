/*
 * udpsock.h
 *
 * User Datagram Protocol socket I/O channel class.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-1998 Equivalence Pty. Ltd.
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
 * Portions are Copyright (C) 1993 Free Software Foundation, Inc.
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: udpsock.h,v $
 * Revision 1.5  2001/05/22 12:49:32  robertj
 * Did some seriously wierd rewrite of platform headers to eliminate the
 *   stupid GNU compiler warning about braces not matching.
 *
 * Revision 1.4  1998/11/30 22:07:29  robertj
 * New directory structure.
 *
 * Revision 1.3  1998/09/24 04:12:07  robertj
 * Added open software license.
 *
 * Revision 1.2  1996/08/03 12:10:23  craigs
 * Changed for new common directories
 *
 * Revision 1.1  1995/01/23 18:43:27  craigs
 * Initial revision
 *
 * Revision 1.1  1994/04/12  08:31:05  robertj
 * Initial revision
 *
 */

#ifndef _PUDPSOCKET

#pragma interface

///////////////////////////////////////////////////////////////////////////////
// PUDPSocket

#define _PUDPSOCKET_PLATFORM_INCLUDE
#include "../../udpsock.h"

#endif
#ifdef _PUDPSOCKET_PLATFORM_INCLUDE
#undef _PUDPSOCKET_PLATFORM_INCLUDE


#endif


// End Of File ////////////////////////////////////////////////////////////////
