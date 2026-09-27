.POSIX:
.SILENT:
.SUFFIXES:
.SUFFIXES: .c .o


# ----------- #
# Definitions #
# ----------- #

TARGETS = crosscc

CC = clang

CFLAGS = -std=c11 -Weverything -g

DEPS = $(TARGETS:=.d)


# ----- #
# Rules #
# ----- #

all: $(TARGETS)

FORCE:

clean: FORCE
	$(RM) $(TARGETS:=.o) $(TARGETS:=.d) $(TARGETS)

crosscc: crosscc.o
	echo "LD    $@"
	$(CC) $(CFLAGS) $(LDFLAGS) crosscc.o -o $@

$(TARGETS:=.o): Makefile

.c.o:
	echo "CC    $@"
	$(CC) $(CFLAGS) -MMD -MF $(@:.o=.d) -c $< -o $@

$(DEPS):
	touch $@

include $(DEPS)
