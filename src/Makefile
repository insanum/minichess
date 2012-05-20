# "$Id: Makefile,v 1.13 1999/07/29 04:59:39 edavis Exp $"
#
# miniCHESS - another stupid dock app
# Copyright (c) 1999 by Eric Davis, ead@pobox.com
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
# (See COPYING / GPL-2.0)
#


# Specify the location of gnuchessx on your system.
GNUCHESS = /usr/local/bin/gnuchessx

# Where do you want the binary installed?
DESTDIR = /usr/local/bin

# What do you want to name the binary?
BIN_NAME = minichess

# Uncomment this line if your using Solaris.
#SOLARIS_LIBS = -lsocket

CFLAGS = -ansi -I/usr/X11R6/include
LIBDIR = -L/usr/X11R6/lib
LIBS   = -lXpm -lXext -lX11 $(SOLARIS_LIBS)
OBJS   = chess.o xgen.o

.c.o:
	gcc $(CFLAGS) -DENGINE=\"$(GNUCHESS)\" -c $< -o $*.o

$(BIN_NAME): $(OBJS)
	gcc $(CFLAGS) -o $(BIN_NAME) $(OBJS) $(LIBDIR) $(LIBS)

clean:
	rm -f $(OBJS) $(BIN_NAME)

install:
	cp -f $(BIN_NAME) $(DESTDIR)
	chmod 755 $(DESTDIR)/$(BIN_NAME)

