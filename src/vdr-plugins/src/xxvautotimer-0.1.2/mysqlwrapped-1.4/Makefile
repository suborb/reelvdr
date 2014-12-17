VERSION =	1.4

INSTALL_PREFIX = /usr/devel
INSTALL_LIB =	$(INSTALL_PREFIX)/lib
INSTALL_INCLUDE = $(INSTALL_PREFIX)/include
INSTALL =	/usr/bin/install

INCLUDE =	-I/usr/devel/include 
CFLAGS =	-fPIC -Wall -g -O2 $(INCLUDE) -MD
CPPFLAGS =	$(CFLAGS)

LIBS =		-L/usr/local/lib/mysql -lmysqlclient -lz

PROGS =		

all:		$(PROGS) libmysqlwrapped.a libmysqlwrapped.h

t:		t.o enum_t.o set_t.o
		g++ -o $@ $^ $(LIBS)
		
LIBM =		Database.o Query.o StderrLog.o SysLog.o enum_t.o set_t.o
libmysqlwrapped.a: \
		$(LIBM)
		ar cru libmysqlwrapped.a $(LIBM)
		ranlib libmysqlwrapped.a

libmysqlwrapped.h: \
		IError.h StderrLog.h SysLog.h \
			enum_t.h set_t.h Database.h Query.h
		cat IError.h StderrLog.h SysLog.h \
			enum_t.h set_t.h Database.h Query.h \
			> libmysqlwrapped.h

# 'make install' removes old versions first
install:	all
		rm -f \
			/usr/local/include/Connect.* \
			/usr/local/include/Database.* \
			/usr/local/include/Query.* \
			/usr/devel/include/Connect.* \
			/usr/devel/include/Database.* \
			/usr/devel/include/Query.* \
			/usr/local/include/libsql++.h \
			/usr/devel/include/libsql++.h \
			/usr/local/lib/libsql++.a \
			/usr/devel/lib/libsql++.a
		$(INSTALL) libmysqlwrapped.a $(INSTALL_LIB)
		$(INSTALL) libmysqlwrapped.h $(INSTALL_INCLUDE)

docs:
		cp simple.cfg /usr/local/apache/www.dbdesigner.net/htdocs/project/mysql

clean:
		rm -f *~ *.o $(PROGS) *.d slask *.a

TARFILE =	mysqlwrapped-$(VERSION)
tar:		clean
		tar czf $(TARFILE).tar.gz \
			Query.* \
			Database.* \
			IError.h \
			StderrLog.* \
			SysLog.* \
			enum_t.* \
			set_t.* \
			Makefile README Changelog gpl.txt \
			Project/mysqlwrapped.dsp
		/usr/local/bin/tarfix.sh $(TARFILE)
		cp $(TARFILE).tar.gz /usr/local/apache/www.dbdesigner.net/htdocs/project/mysql

-include	*.d
