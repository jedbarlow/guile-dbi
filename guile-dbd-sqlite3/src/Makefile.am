# Makefile.am - Main sources automake makefile
# Copyright (C) 2004, 2005 Free Software Foundation, Inc.
# Written by Maurizio Boriani <baux@member.fsf.org>
#
# This file is part of the guile-dbd-sqlite3.
# 
# The guile-dbd-sqlite3 is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# The guile-dbd-sqlite3 is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
#
#                                               -*- Makefile -*-
# Process this file with autoconf to produce a configure script.


AUTOMAKE_OPTIONS = gnu
INCLUDES = ${CFLAGS} -I/usr/include


lib_LTLIBRARIES = libguile-dbd-sqlite3.la


libguile_dbd_sqlite3_la_SOURCES = guile-dbd-sqlite3.c
libguile_dbd_sqlite3_la_LIBADD = -ldl -lz ${LIBS}
libguile_dbd_sqlite3_la_LDFLAGS = -export-dynamic \
	-version-info @DBD_INTERFACE@
libguile_dbd_sqlite3_la_DEPENDENCIES = $(LTLIBOBJS)

