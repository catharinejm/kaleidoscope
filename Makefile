CC=clang++
LLVM_OPTS=$(shell llvm-config --cxxflags --ldflags --libs)
BDWGC_OPTS=$(shell pkg-config --libs bdw-gc) -lgccpp

CC_FILES=$(shell ls *.cc)

all:
	$(CC) $(LLVM_OPTS) $(BDWGC_OPTS) -o kaleidoscope $(CC_FILES)

clean:
	rm -f *.o kaleidoscope

run: clean all
	./kaleidoscope
