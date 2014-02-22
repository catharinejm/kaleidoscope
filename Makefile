CC=clang++
LLVM_OPTS=$(shell llvm-config --cxxflags --ldflags --libs)
BDWGC_OPTS=$(shell pkg-config --libs bdw-gc) -lgccpp
CXXFLAGS=-std=c++11 -stdlib=libc++ -I/usr/lib/c++/v1

CC_FILES=$(shell ls *.cc)

all:
	$(CC) $(CXXFLAGS) $(LLVM_OPTS) $(BDWGC_OPTS) -o kaleidoscope $(CC_FILES)

clean:
	rm -f *.o kaleidoscope

run: clean all
	./kaleidoscope

lisp:
	$(CC) $(CXXFLAGS) -ggdb $(BDWGC_OPTS) $(LLVM_OPTS) -fcxx-exceptions -o lisp reader.cc printer.cc lisp.cc

lispclean:
	rm -f lisp

lisprun: lispclean lisp
	./lisp

lispdebug: lispclean lisp
	gdb lisp
