CC=clang++
LLVM_BUILD_OPTS=$(shell llvm-config --cxxflags)
LLVM_LINK_OPTS=$(shell llvm-config --ldflags --libs)
LLVM_OPTS=$(LLVM_BUILD_OPTS) $(LLVM_LINK_OPTS)

BDWGC_OPTS=$(shell pkg-config --libs bdw-gc) -lgccpp
CXXFLAGS=-std=c++11 -stdlib=libc++ -I/usr/lib/c++/v1
EXTRAS=-fcxx-exceptions -fno-rtti -lc++

CC_FILES=reader.cc printer.cc compiler.cc lisp.cc
O_FILES=reader.o printer.o compiler.o lisp.o

compile: build link

link:
	$(CC) $(CXXFLAGS) -ggdb $(BDWGC_OPTS) $(LLVM_OPTS) $(EXTRAS) -o lisp $(O_FILES)

build: clean
	$(CC) $(CXXFLAGS) -ggdb $(LLVM_BUILD_OPTS) $(EXTRAS) -c $(CC_FILES)

clean:
	rm -f lisp $(O_FILES)
	rm -rf lisp.dSYM

run: clean compile
	./lisp

debug: clean compile
	gdb lisp
