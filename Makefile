# Copyright 2009 The RE2 Authors.  All Rights Reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

all: obj/libre2.a

# to build against PCRE for testing or benchmarking,
# uncomment the next two lines
# CCPCRE=-I/usr/local/include -DUSEPCRE
# LDPCRE=-L/usr/local/lib -lpcre

CC=g++
CFLAGS=-c -Wall -Wno-sign-compare -O3 -g -I. $(CCPCRE)
AR=ar
ARFLAGS=rsc
NM=nm
NMFLAGS=-p

HFILES=\
	util/arena.h\
	util/atomicops.h\
	util/benchmark.h\
	util/hash_map.h\
	util/logging.h\
	util/mutex.h\
	util/pcre.h\
	util/random.h\
	util/sparse_array.h\
	util/test.h\
	util/utf.h\
	util/util.h\
	re2/filtered_re2.h\
	re2/prefilter.h\
	re2/prefilter_tree.h\
	re2/prog.h\
	re2/re2.h\
	re2/regexp.h\
	re2/stringpiece.h\
	re2/testing/exhaustive_tester.h\
	re2/testing/regexp_generator.h\
	re2/testing/string_generator.h\
	re2/testing/tester.h\
	re2/unicode_casefold.h\
	re2/unicode_groups.h\
	re2/walker-inl.h\

OFILES=\
	obj/util/arena.o\
	obj/util/hash.o\
	obj/util/rune.o\
	obj/util/stringpiece.o\
	obj/util/stringprintf.o\
	obj/util/strutil.o\
	obj/re2/bitstate.o\
	obj/re2/compile.o\
	obj/re2/dfa.o\
	obj/re2/filtered_re2.o\
	obj/re2/mimics_pcre.o\
	obj/re2/nfa.o\
	obj/re2/onepass.o\
	obj/re2/parse.o\
	obj/re2/perl_groups.o\
	obj/re2/prefilter.o\
	obj/re2/prefilter_tree.o\
	obj/re2/prog.o\
	obj/re2/re2.o\
	obj/re2/regexp.o\
	obj/re2/simplify.o\
	obj/re2/tostring.o\
	obj/re2/unicode_casefold.o\
	obj/re2/unicode_groups.o\

TESTOFILES=\
	obj/util/pcre.o\
	obj/util/random.o\
	obj/util/thread.o\
	obj/re2/testing/backtrack.o\
	obj/re2/testing/dump.o\
	obj/re2/testing/exhaustive_tester.o\
	obj/re2/testing/null_walker.o\
	obj/re2/testing/regexp_generator.o\
	obj/re2/testing/string_generator.o\
	obj/re2/testing/tester.o\

TESTS=\
	obj/test/charclass_test\
	obj/test/compile_test\
	obj/test/filtered_re2_test\
	obj/test/mimics_pcre_test\
	obj/test/parse_test\
	obj/test/possible_match_test\
	obj/test/re2_test\
	obj/test/re2_arg_test\
	obj/test/required_prefix_test\
	obj/test/search_test\
	obj/test/simplify_test\
	obj/test/string_generator_test\
	obj/test/dfa_test\
	obj/test/exhaustive1_test\
	obj/test/exhaustive2_test\
	obj/test/exhaustive3_test\
	obj/test/exhaustive_test\
	obj/test/random_test\

obj/%.o: %.cc $(HFILES)
	@mkdir -p $$(dirname $@)
	$(CC) -o $@ $(CFLAGS) $*.cc 2>&1 | sed 5q

obj/%.o: %.c $(HFILES)
	@mkdir -p $$(dirname $@)
	$(CC) -o $@ $(CFLAGS) $*.c 2>&1 | sed 5q

obj/libre2.a: $(OFILES)
	@mkdir -p obj
	$(AR) $(ARFLAGS) obj/libre2.a $(OFILES)

obj/test/%: obj/libre2.a obj/re2/testing/%.o $(TESTOFILES) obj/util/test.o
	@mkdir -p obj/test
	$(CC) -o $@ obj/re2/testing/$*.o $(TESTOFILES) obj/util/test.o obj/libre2.a -lpthread $(LDPCRE)

obj/test/regexp_benchmark: obj/libre2.a obj/re2/testing/regexp_benchmark.o $(TESTOFILES) obj/util/benchmark.o
	@mkdir -p obj/test
	$(CC) -o $@ obj/re2/testing/regexp_benchmark.o $(TESTOFILES) obj/util/benchmark.o obj/libre2.a -lpthread $(LDPCRE)

clean:
	rm -rf obj

testofiles: $(TESTOFILES)

test: $(TESTS)
	@./runtests $(TESTS)

benchmark: obj/test/regexp_benchmark

install: obj/libre2.a
	mkdir -p /usr/local/include/re2
	install -m 444 re2/re2.h /usr/local/include/re2/re2.h
	install -m 444 re2/stringpiece.h /usr/local/include/re2/stringpiece.h
	install -m 444 re2/variadic_function.h /usr/local/include/re2/variadic_function.h
	install -m 555 obj/libre2.a /usr/local/lib/libre2.a

testinstall:
	@mkdir -p obj
	cp testinstall.cc obj
	(cd obj && g++ -I/usr/local/include testinstall.cc -lre2 -o testinstall)
	obj/testinstall

benchlog: obj/test/regexp_benchmark
	(echo '==BENCHMARK==' `hostname` `date`; \
	  (uname -a; g++ --version; hg identify; file obj/test/regexp_benchmark) | sed 's/^/# /'; \
	  echo; \
	  ./obj/test/regexp_benchmark 'PCRE|RE2') | tee -a benchlog.$$(hostname | sed 's/\..*//')
