GAME     = tron
OBJS     = tron.o

all: $(OBJS)
	ar -r $(GAME).a $(OBJS)

clean:
	@echo -n "."
	@rm -rf *.o *.a

.cpp.o:
	@echo -n "."
	$(CXX) $(CXXFLAGS) -c $<
