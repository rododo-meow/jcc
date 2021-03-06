.PHONY: all clean test vars deps flex bison update-html archive lib
.PRECIOUS: tests/%

CLANG := clang
CXX := clang++
CC := clang
LEX := flex
LLVM_COMPONENTS := all

OS_NAME = $(shell uname -o | tr '[A-Z]' '[a-z]')

# libjsoncpp
ifeq ($(OS_NAME), cygwin)
CPPFLAGS += 
LIBS += -ljsoncpp
else
CPPFLAGS += `pkg-config --cflags jsoncpp`
LIBS += `pkg-config --libs jsoncpp`
endif

# llvm
ifeq ($(OS_NAME), cygwin)
LIBS += -lLLVM-3.5
else
CPPFLAGS += `llvm-config --cxxflags`
LDFLAGS += `llvm-config --ldflags`
LIBS += `llvm-config --libs $(LLVM_COMPONENTS)`
#LIBS += -lLLVM-3.7
LIBS += `llvm-config --system-libs $(LLVM_COMPONENTS)`
endif

# jascal
SOURCES := $(patsubst src/%,%,$(wildcard src/*.cpp))
SOURCES += jascal.tab.cc lex.yy.cc
OBJS := $(patsubst %.cpp,objs/%.o,$(SOURCES))
OBJS := $(patsubst %.cc,objs/%.o,$(OBJS))
OBJS += objs/HtmlTemplate.o
DEPS := $(patsubst %.cpp,deps/%.d,$(SOURCES))
DEPS := $(patsubst %.cc,deps/%.d,$(DEPS))
PROG := jcc
TESTS := $(wildcard tests/*.jas)
TESTS_TARGET := $(patsubst tests/%.jas,test-%,$(TESTS))
JASCAL_LIB_SRC := $(wildcard lib/*.jas)
JASCAL_LIB := $(patsubst %.jas,%.jsym,$(JASCAL_LIB_SRC))
JASCAL_LIB_OBJ := $(patsubst %.jas,%.ll,$(JASCAL_LIB_SRC))
JASCAL_LIB_IMPL := $(patsubst %.jas,%.impl.ll,$(JASCAL_LIB_SRC))
CPPFLAGS ?=
CPPFLAGS += -Iinclude -g -DDEBUG -fexceptions
ifeq ($(DEBUG), 1)
CPPFLAGS += -g -DDEBUG
endif

all: $(PROG)

deps/%.d: src/%.cpp | flex bison
	@echo "[DEP ] $< -> $@"
	@$(CXX) -MM $(CPPFLAGS) $< > $@.$$$$ || exit "$$?"; \
	sed 's,\($*\)\.o[ :]*,objs/\1.o $@: ,g' < $@.$$$$ > $@ || exit "$$?"; \
	$(RM) $@.$$$$ || exit "$$?"

deps/%.d: src/%.cc | flex bison
	@echo "[DEP ] $< -> $@"
	@$(CXX) -MM $(CPPFLAGS) $< > $@.$$$$ || exit "$$?"; \
	sed 's,\($*\)\.o[ :]*,objs/\1.o $@: ,g' < $@.$$$$ > $@ || exit "$$?"; \
	$(RM) $@.$$$$ || exit "$$?"

objs/%.o: 
	@echo "[CXX ] $< -> $@"
	@$(CXX) $(CPPFLAGS) -c -o $@ $<

include/jascal.tab.hpp src/jascal.tab.cc: src/jascal.yy
	@echo "[YACC] $<"
	@$(YACC) --defines=include/jascal.tab.hpp --report-file=bison-report.txt -o src/jascal.tab.cc $<

src/lex.yy.cc: src/jascal.l
	@echo "[LEX ] $< -> $@"
	@$(LEX) --outfile=$@ $<
	
include $(DEPS)

vars:
	@echo "SOURCES=" $(SOURCES)
	@echo "DEPS=" $(DEPS)
	@echo "OBJS=" $(OBJS)
	@echo "JASCAL_LIB=" $(JASCAL_LIB)
	@echo "TESTS=" $(TESTS)
	
deps: $(DEPS)

$(PROG): $(OBJS)
	@echo "[LINK] $@"
	@$(CXX) -o $@ $(LDFLAGS) $^ $(LIBS)

bison: src/jascal.tab.cc include/jascal.tab.hpp

flex: src/lex.yy.cc

clean:
	@echo "CLEAN"
	@$(RM) -rf src/jascal.tab.cc include/jascal.tab.hpp src/lex.yy.cc \
		 $(PROG) $(OBJS) $(DEPS) bison-report.txt tests/*.txt tests/*.ll tests/*.json tests/*.o tests/*.html tests/*.jsym $(JASCAL_LIB_OBJ) $(JASCAL_LIB) $(TESTS_OUT)

test: $(TESTS_TARGET)

test-%: tests/%
	@(cd tests; ./test.sh $*)

tests/%: tests/%.jas $(PROG) $(JASCAL_LIB) $(JASCAL_LIB_IMPL)
	@echo "[JCC ] $< -> $@"
	@(./$(PROG) --dump-html tests/$*.html --llvm -o tests/$*.ll $< && $(CLANG) -m32 $(JASCAL_LIB_IMPL) tests/$*.ll -o $@)

objs/index-comb.html: codeview/index.html tools/HtmlCombiner/combiner.py
	@echo "[GEN ] $< -> $@"
	@./tools/HtmlCombiner/combiner.py $< $@
	@dd if=/dev/zero bs=1 count=1 >> $@ 2>/dev/null

objs/HtmlTemplate.o: objs/index-comb.html tools/bincc/bincc
	@echo "[GEN ] $< -> $@"
	@./tools/bincc/bincc $< $@ htmlTemplate

tools/bincc/bincc: tools/bincc/bincc.c
	@echo "[CC  ] $< -> $@"
	@$(CC) -o $@ $<

objs/output.o: src/output.cpp
	@echo "[G++ ] $< -> $@"
	@g++ $(CPPFLAGS) -c -o $@ $<

update-html:
	@rm -f objs/index-comb.html
	@make all

archive:
	@echo ARCHIVE
	@git archive --format tar.gz --prefix jcc/ HEAD -o jcc.tar.gz

lib/%.jsym: lib/%.jas $(PROG)
	@echo "[LIB ] $< -> $@"
	@./$(PROG) --jsym -o lib/$*.ll --llvm -fno-builtin $<

lib: $(JASCAL_LIB)

lib/io.jsym: lib/string.jsym
