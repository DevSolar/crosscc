.POSIX:
.SILENT:
.SUFFIXES:
.SUFFIXES: .c .o


# ----------- #
# Definitions #
# ----------- #

TARGETS = crosscc

CC = clang

CFLAGS = -std=c11 -Weverything -g -I ./src \
         -Wno-unsafe-buffer-usage

SUPPORT_FILES = src/support/greatest.h \
                src/support/solog.h

DEPS = $(TARGETS:=.d)


# ----- #
# Rules #
# ----- #

all: $(TARGETS)

FORCE:

clean: FORCE
	$(RM) $(TARGETS:=.o) $(TARGETS:=.d) $(TARGETS)

mrproper: clean
	$(RM) $(SUPPORT_FILES)

test: test/crosscc_test FORCE
	./test/crosscc_test

crosscc: crosscc.o
	echo "LD    $@"
	$(CC) $(CFLAGS) $(LDFLAGS) crosscc.o -o $@

test/crosscc_test: test/crosscc_test.o
	echo "LD    $@"
	$(CC) $(CFLAGS) $(LDFLAGS) test/crosscc_test.o -o $@

$(TARGETS:=.o): Makefile $(SUPPORT_FILES)

.c.o:
	echo "CC    $@"
	$(CC) $(CFLAGS) -MMD -MF $(@:.o=.d) -c $< -o $@

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
