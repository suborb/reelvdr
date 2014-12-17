
OUT=test_fileexists.out test_progressbar.out test_networkdevice.out

LIBS=-lreel

all: $(OUT)

%.o:%.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^

%.out:%.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

clean:
	@rm -f *.o $(OUT)
