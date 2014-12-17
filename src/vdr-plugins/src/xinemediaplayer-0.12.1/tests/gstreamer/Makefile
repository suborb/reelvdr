GSTREAMERLIB=../../GstreamerLib.c
INCLUDES=../.. 
LIBS=`pkg-config --libs --cflags gstreamer-0.10`
LIBS+=`pkg-config --libs --cflags gstreamer-interfaces-0.10`

CC=g++
CC_OPTS= -g -O3



test% : test%.c ${GSTREAMERLIB} main.c
	 $(CC) $(CC_OPTS) ${GSTREAMERLIB} main.c -DFILE_INCLUDE=\"$<\" -o $@ -I${INCLUDES} ${LIBS}
	 @echo $<
