/*
 * url.h
 *
 * Universal Resource Locator (for HTTP/HTML) class.
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
 * $Log: url.h,v $
 * Revision 1.19  2002/09/16 01:08:59  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 1.18  2002/03/18 05:01:54  robertj
 * Added functions to set component parts of URL.
 *
 * Revision 1.17  2001/11/08 00:32:49  robertj
 * Added parsing of ';' based parameter fields into string dictionary if there are multiple parameters, with '=' values.
 *
 * Revision 1.16  2001/09/28 00:32:24  robertj
 * Broke out internal static function for unstranslating URL strings.
 *
 * Revision 1.15  1999/03/09 08:01:47  robertj
 * Changed comments for doc++ support (more to come).
 *
 * Revision 1.14  1999/02/16 08:07:10  robertj
 * MSVC 6.0 compatibility changes.
 *
 * Revision 1.13  1998/09/23 06:20:11  robertj
 * Added open source copyright license.
 *
 * Revision 1.12  1998/02/16 00:12:53  robertj
 * Added function to open a URL in a browser.
 *
 * Revision 1.11  1998/02/03 10:02:35  robertj
 * Added ability to get scheme, host and port from URL as a string.
 *
 * Revision 1.10  1998/02/03 06:18:49  robertj
 * Fixed URL encoding to be closer to RFC
 *
 * Revision 1.9  1997/01/12 04:22:54  robertj
 * Added has function so URL can be dictionary key.
 *
 * Revision 1.8  1996/08/19 13:37:28  robertj
 * Fixed URL parsing and definition (cannot have relative paths).
 *
 * Revision 1.7  1996/06/10 09:55:44  robertj
 * Added global function for query parameters parsing.
 *
 * Revision 1.6  1996/03/31 08:53:13  robertj
 * Added string representation for URI part only.
 *
 * Revision 1.5  1996/03/16 04:46:02  robertj
 * Added translation type to TranslateString() to accommodate query variables.
 *
 * Revision 1.4  1996/03/02 03:12:13  robertj
 * Added function to translate a string to a form suitable for inclusion in a URL.
 *
 * Revision 1.3  1996/02/03 11:06:27  robertj
 * Added splitting of query field into variables dictionary.
 *
 * Revision 1.2  1996/01/26 02:24:32  robertj
 * Further implemetation.
 *
 * Revision 1.1  1996/01/23 13:04:20  robertj
 * Initial revision
 *
 */

#ifndef _PURL
#define _PURL

#ifdef P_USE_PRAGMA
#pragma interface
#endif


//////////////////////////////////////////////////////////////////////////////
// PURL

/**
 This class describes a Universal Resource Locator.
 This is the desciption of a resource location as used by the World Wide
 Web and the #PHTTPSocket# class.
 */
class PURL : public PObject
{
  PCLASSINFO(PURL, PObject)
  public:
    /**Construct a new URL object from the URL string. */
    PURL();
    /**Construct a new URL object from the URL string. */
    PURL(
      const char * cstr     /// C string representation of the URL.
    );
    /**Construct a new URL object from the URL string. */
    PURL(
      const PString & str   /// String representation of the URL.
    );

  /**@name Overrides from class PObject */
  //@{
    /**Compare the two URLs and return their relative rank.

     @return
       #LessThan#, #EqualTo# or #GreaterThan#
       according to the relative rank of the objects.
     */
    virtual Comparison Compare(
      const PObject & obj   /// Object to compare against.
    ) const;

    /**This function yields a hash value required by the #PDictionary#
       class. A descendent class that is required to be the key of a dictionary
       should override this function. The precise values returned is dependent
       on the semantics of the class. For example, the #PString# class
       overrides it to provide a hash function for distinguishing text strings.

       The default behaviour is to return the value zero.

       @return
       hash function value for class instance.
     */
    virtual PINDEX HashFunction() const;

    /**Output the contents of the URL to the stream as a string.
     */
    virtual void PrintOn(
      ostream &strm   /// Stream to print the object into.
    ) const;

    /**Input the contents of the URL from the stream. The input is a URL in
       string form.
     */
    virtual void ReadFrom(
      istream &strm   /// Stream to read the objects contents from.
    );
  //@}
 
  /**@name New functions for class. */
  //@{
    /**Parse the URL string into the fields in the object instance. */
    void Parse(
      const char * cstr   /// URL as a string to parse.
    );
    /**Parse the URL string into the fields in the object instance. */
    void Parse(
      const PString & str /// URL as a string to parse.
    ) { Parse((const char *)str); }

    /**Print/String output representation formats. */
    enum UrlFormat {
      /// Translate to a string as a full URL
      FullURL,      
      /// Translate to a string as only path
      PathOnly,     
      /// Translate to a string with no scheme or host
      URIOnly,      
      /// Translate to a string with scheme and host/port
      HostPortOnly  
    };

    /**Convert the URL object into its string representation. The parameter
       indicates whether a full or partial representation os to be produced.

       @return
       String representation of the URL.
     */
    PString AsString(
      UrlFormat fmt = FullURL   /// The type of string to be returned.
    ) const;

    /// Type for translation of strings to URL format,
    enum TranslationType {
      /// Translate a username/password field for a URL.
      LoginTranslation,
      /// Translate the path field for a URL.
      PathTranslation,
      /// Translate the query parameters field for a URL.
      QueryTranslation
    };

    /**Translate a string from general form to one that can be included into
       a URL. All reserved characters for the particular field type are
       escaped.

       @return
       String for the URL ready translation.
     */
    static PString TranslateString(
      const PString & str,    /// String to be translated.
      TranslationType type    /// Type of translation.
    );

    /**Untranslate a string from a form that was included into a URL into a
       normal string. All reserved characters for the particular field type
       are unescaped.

       @return
       String from the URL untranslated.
     */
    static PString UntranslateString(
      const PString & str,    /// String to be translated.
      TranslationType type    /// Type of translation.
    );

    /** Split a string in &= form to a dictionary of names and values. */
    static void SplitQueryVars(
      const PString & queryStr,   /// String to split into variables.
      PStringToString & queryVars /// Dictionary of variable names and values.
    );


    /// Get the scheme field of the URL.
    const PCaselessString & GetScheme() const { return scheme; }

    /// Set the scheme field of the URL
    void SetScheme(const PString & scheme);

    /// Get the username field of the URL.
    const PString & GetUserName() const { return username; }

    /// Set the username field of the URL.
    void SetUserName(const PString & username);

    /// Get the password field of the URL.
    const PString & GetPassword() const { return password; }

    /// Set the password field of the URL.
    void SetPassword(const PString & password);

    /// Get the hostname field of the URL.
    const PCaselessString & GetHostName() const { return hostname; }

    /// Set the hostname field of the URL.
    void SetHostName(const PString & hostname);

    /// Get the port field of the URL.
    WORD GetPort() const { return port; }

    /// Set the port field in the URL.
    void SetPort(WORD newPort);

    /// Get the path field of the URL as a string.
    const PString & GetPathStr() const { return pathStr; }

    /// Set the path field of the URL as a string.
    void SetPathStr(const PString & pathStr);

    /// Get the path field of the URL as a string array.
    const PStringArray & GetPath() const { return path; }

    /// Set the path field of the URL as a string array.
    void SetPath(const PStringArray & path);

    /// Get the parameter (;) field of the URL.
    PString GetParameters() const;

    /// Set the parameter (;) field of the URL.
    void SetParameters(const PString & parameters);

    /// Get the parameter (;) field(s) of the URL as a string dictionary.
    const PStringToString & GetParamVars() const { return paramVars; }

    /// Set the parameter (;) field(s) of the URL as a string dictionary.
    void SetParamVars(const PStringToString & paramVars);

    /// Set the parameter (;) field of the URL as a string dictionary.
    void SetParamVar(const PString & key, const PString & data);

    /// Get the fragment (##) field of the URL.
    const PString & GetFragment() const { return fragment; }

    /// Get the Query (?) field of the URL as a string.
    PString GetQuery() const;

    /// Set the Query (?) field of the URL as a string.
    void SetQuery(const PString & query);

    /// Get the Query (?) field of the URL as a string dictionary.
    const PStringToString & GetQueryVars() const { return queryVars; }

    /// Set the Query (?) field(s) of the URL as a string dictionary.
    void SetQueryVars(const PStringToString & queryVars);

    /// Set the Query (?) field of the URL as a string dictionary.
    void SetQueryVar(const PString & key, const PString & data);

    /**Open the URL in a browser.

       @return
       The browser was successfully opened. This does not mean the URL exists and was
       displayed.
     */
    static BOOL OpenBrowser(
      const PString & url   /// URL to open
    );
  //@}

  protected:
    void Recalculate();
    PString urlString;

    PCaselessString scheme;
    PString username;
    PString password;
    PCaselessString hostname;
    WORD port;
    PString pathStr;
    PStringArray path;
    PStringToString paramVars;
    PString fragment;
    PStringToString queryVars;
};


#endif


// End Of File ///////////////////////////////////////////////////////////////
