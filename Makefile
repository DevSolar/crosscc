.POSIX:
.SILENT:
.SUFFIXES:
.SUFFIXES: .c .o


# ----------- #
# Definitions #
# ----------- #

EXECUTABLES = crosscc test/crosscc_test

TARGETS = $(EXECUTABLES) check-win-api

CC = clang

CFLAGS = -std=c11 -Weverything -g -I ./src \
         -Wno-unsafe-buffer-usage \
         -Wno-used-but-marked-unused

SUPPORT_FILES = src/support/greatest.h \
                src/support/solog.h

DEPS = $(EXECUTABLES:=.d)

JSON = $(EXECUTABLES:=.json)


# ----- #
# Rules #
# ----- #

all: $(TARGETS)

FORCE:

clean: FORCE
	$(RM) $(EXECUTABLES:=.o) $(EXECUTABLES:=.d) $(EXECUTABLES) check-win-api

mrproper: clean
	$(RM) $(SUPPORT_FILES)

test: test/crosscc_test FORCE
	./test/crosscc_test

check-win-api: $(EXECUTABLES:=.c) Makefile $(SUPPORT_FILES)
	echo "CC    $@"
	x86_64-w64-mingw32-clang $(CFLAGS) -fsyntax-only $(EXECUTABLES:=.c)
	touch $@

crosscc: crosscc.o
	echo "LD    $@"
	$(CC) $(CFLAGS) $(LDFLAGS) crosscc.o -o $@

test/crosscc_test: test/crosscc_test.o
	echo "LD    $@"
	$(CC) $(CFLAGS) $(LDFLAGS) test/crosscc_test.o -o $@

$(EXECUTABLES:=.o): Makefile $(SUPPORT_FILES)

.c.o:
	echo "CC    $@"
	$(CC) $(CFLAGS) -MMD -MF $(@:.o=.d) -MJ $*.json -c $< -o $@

$(DEPS):
	touch $@

include $(DEPS)


# ------------- #
# Support files #
# ------------- #

# GreaTest v1.5.0
src/support/greatest.h:
	echo "GET   $(@F)"
	curl -sSL "https://raw.githubusercontent.com/silentbicycle/greatest/refs/tags/v1.5.0/$(@F)" -o $@

# SoLog v10
src/support/solog.h:
	echo "GET   $(@F)"
	curl -sSL "https://raw.githubusercontent.com/DevSolar/solog/refs/tags/v10/$(@F)" -o $@
