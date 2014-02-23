CC=clang++
#LLVM_OPTS=$(shell llvm-config --cxxflags --ldflags --libs)
#BDWGC_OPTS=$(shell pkg-config --libs bdw-gc) -lgccpp
#BDWGC_OPTS=-lgc -lgccpp
CXXFLAGS=-std=c++11 -stdlib=libc++ -I/usr/lib/c++/v1

all:
	$(CC) $(CXXFLAGS) $(BDWGC_OPTS) -ggdb -o testit testit.cc

clean:
	rm -f testit

run: clean all
	./testit

debug: clean all
	gdb testit
