REELDVB ?= ../../kernel/reeldvb
INCLUDES += -I$(REELDVB)

all:
	gcc -Wall -m32  -Os $(INCLUDES) init_ci.c -o init_ci
clean:
	rm init_ci
