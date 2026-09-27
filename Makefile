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

SUPPORT_FILES = src/support/solog.h

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

crosscc: crosscc.o
	echo "LD    $@"
	$(CC) $(CFLAGS) $(LDFLAGS) crosscc.o -o $@

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

# SoLog v10
src/support/solog.h:
	echo "GET   $(@F)"
	curl -sSL "https://raw.githubusercontent.com/DevSolar/solog/refs/tags/v10/$(@F)" -o $@
