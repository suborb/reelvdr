### 
### I think it's not worth to make such a small project
### modular. So this is a simple gnu Makefile...
###

include ../../Make.config

all: hdfbshot

hdfbshot:
	$(CC) fbshot.c -lpng -lz -o hdfbshot
	strip hdfbshot

install:
	install fbshot /usr/bin/fbshot
	install fbshot.1.man /usr/man/man1/fbshot.1

clean:
	rm -f hdfbshot *~
	
