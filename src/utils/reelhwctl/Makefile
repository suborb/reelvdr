LDFLAGS ?= -m32 
CXXFLAGS ?= -m32 -O3
INCLUDES = 

reelhwctl_OBJECTS= reelhwctl.o

all: reelhwctl

reelhwctl: $(reelhwctl_OBJECTS)
	@rm -f $@
	$(CXX) $(LDFLAGS) $(reelhwctl_OBJECTS) $(LDADD) $(LIBS) -o $@
	strip $@
.c.o:
	$(CXX) $(DEFINES) $(INCLUDES)  $(CXXFLAGS) -c $<

clean:
	$(RM)  *.o *~
