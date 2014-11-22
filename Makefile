EXE=solve
SRCFILES=source.cpp lex.cpp lex.test.cpp solve.cpp 

.PHONY: all test
all: $(EXE) tags

test: all
	./$(EXE)

CXXFLAGS+=-std=c++11 -Wall -Wextra -g3
#LDFLAGS+=-lncurses
OBJS=$(patsubst %.cpp,%.o,$(SRCFILES))

CXXFLAGS+=-MMD # Generate .d files
-include $(OBJS:.o=.d)

ifdef OPTIMIZED
	CXXFLAGS+=-O3 -DNDEBUG
endif

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(EXE) *.o *.d tags

tags: $(SRCFILES)
	ctags --c++-kinds=+p --fields=+iaS --extra=+q $(SRCFILES) *.h 2>/dev/null
