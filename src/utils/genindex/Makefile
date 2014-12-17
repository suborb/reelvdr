#
# A VDR index file generator
#
# (C) 2003-2006 Stefan Huelswitt <s.huelswitt@gmx.de>
#
# This code is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This code is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
# Or, point your browser to http://www.gnu.org/copyleft/gpl.html
#

CC       ?= gcc
CFLAGS   ?= -O2

CXX      ?= g++
CXXFLAGS ?= -g -O2 -Wall -Woverloaded-virtual

DEFINES = -D_GNU_SOURCE

OBJS = genindex.o file.o thread.o ringbuffer.o pes.o

VERSION = $(shell grep 'define PRG_VERSION' version.h | awk '{ print $$3 }' | sed -e 's/[";]//g')
TMPDIR = /tmp
ARCHIVE = "genindex-$(VERSION)"

all: genindex

# Implicit rules:

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $(DEFINES) $(INCLUDES) $<

# Dependencies:

MAKEDEP = $(CXX) -MM -MG
DEPFILE = .dependencies
$(DEPFILE): Makefile
	@$(MAKEDEP) $(DEFINES) $(INCLUDES) $(OBJS:%.o=%.c) > $@

-include $(DEPFILE)

# The main program:

genindex: $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

rmstream: rmstream.o thread.o ringbuffer.o pes.o
	$(CXX) $(CXXFLAGS) $^ -o $@

testindex: testindex.o file.o
	$(CXX) $(CXXFLAGS) $^ -o $@

# Housekeeping:

clean:
	-rm -f $(OBJS) $(ARCHIVE).tar.gz $(DEPFILE) genindex rmstream rmstream.o testindex.o testindex core* *~

dist: clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(ARCHIVE).tar.gz -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(ARCHIVE).tar.gz
