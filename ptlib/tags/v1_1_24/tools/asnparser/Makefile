#
# Makefile
#
# Make file for ASN parser
#
# Portable Windows Library
#
# Copyright (c) 1998-2000 Equivalence Pty. Ltd.
#
# The contents of this file are subject to the Mozilla Public License
# Version 1.0 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
# the License for the specific language governing rights and limitations
# under the License.
#
# The Original Code is Portable Windows Library.
#
# The Initial Developer of the Original Code is Equivalence Pty. Ltd.
#
# Portions are Copyright (C) 1993 Free Software Foundation, Inc.
# All Rights Reserved.
# 
# Contributor(s): ______________________________________.
#
# $Log: Makefile,v $
# Revision 1.9  2000/04/26 01:03:46  robertj
# Removed tarfile creation target, this is done differently now.
#
# Revision 1.8  2000/04/06 20:18:57  craigs
# Removed local install target
#
# Revision 1.7  2000/01/22 22:39:58  robertj
# Added missing license and log messages comment.
#


ifndef PWLIBDIR
PWLIBDIR=$(HOME)/pwlib
endif

PROG		= asnparser
SOURCES		= asn_grammar.cxx asn_lex.cxx main.cxx

CLEAN_FILES	= asn_grammar.cxx asn_grammar.h asn_grammar.output asn_lex.cxx


include $(PWLIBDIR)/make/ptlib.mak

asn_grammar.cxx: asn_grammar.y
	@if [ ! -d $(OBJDIR) ] ; then mkdir $(OBJDIR) ; fi
	bison -t -v -d -o $* $<
	mv $* $*.cxx

asn_lex.cxx: asn_lex.l
	flex -t $< > $@

