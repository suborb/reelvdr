
SEARCH_TEST=searchapi_test.out

LIB= -L .. -lreelsearchapi
INCLUDES= -I ..
CXXFLAGS=-g -O0
CXX=ccache g++

default:all

all:$(SEARCH_TEST)

%.o:%.c
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $^

$(SEARCH_TEST):searchapi_test.o
	$(CXX) $(CXXFLAGS) $(LIB) $(INCLUDES) -o $@ $^

clean:
	@rm -f *.o *.out
