REEL_LIB=libreel.so

CXXFLAGS=-fPIC -g -Wall

LIBS=

OBJS=tools.o

all: $(REEL_LIB)

%.o:%.cc 
	$(CXX) $(CXXFLAGS) -c -o $@  $^

$(REEL_LIB) : $(OBJS)
	$(CXX) $(CXXFLAGS) -shared $(LIBS) -o $@  $^

clean:
	@rm -f *.o $(REEL_LIB)
