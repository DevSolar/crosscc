.POSIX:
.SILENT:
.SUFFIXES:
.SUFFIXES: .c .o


# ----------- #
# Definitions #
# ----------- #

EXECUTABLES = crosscc test/crosscc_test

TARGETS = $(EXECUTABLES) check-win-api compile_commands.json

CC = clang

CFLAGS = -std=c11 -Weverything -g -I ./src \
         -Wno-cast-align \
         -Wno-cast-qual \
         -Wno-unsafe-buffer-usage \
         -Wno-used-but-marked-unused

SUPPORT_FILES = src/support/cc.h \
                src/support/greatest.h \
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
	$(RM) $(EXECUTABLES:=.o) $(EXECUTABLES:=.d) $(EXECUTABLES) check-win-api compile_commands.json
	$(RM) $(EXECUTABLES:=.json)

mrproper: clean
	$(RM) $(SUPPORT_FILES)

test: test/crosscc_test FORCE
	./test/crosscc_test

check-win-api: $(EXECUTABLES:=.c) Makefile $(SUPPORT_FILES)
	echo "CC    $@"
	x86_64-w64-mingw32-clang $(CFLAGS) -fsyntax-only $(EXECUTABLES:=.c)
	touch $@

compile_commands.json: $(EXECUTABLES:=.o)
	echo "IDX   $@"
	echo "[" > $@
	cat $(JSON) | sed '$$s/,$$//' >> $@
	echo "]" >> $@

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

# Convenient Containers v1.4.3
src/support/cc.h:
	echo "GET   $(@F)"
	curl -sSL "https://raw.githubusercontent.com/JacksonAllan/CC/refs/tags/v1.4.3/$(@F)" -o $@

# GreaTest v1.5.0
src/support/greatest.h:
	echo "GET   $(@F)"
	curl -sSL "https://raw.githubusercontent.com/silentbicycle/greatest/refs/tags/v1.5.0/$(@F)" -o $@

# SoLog v10
src/support/solog.h:
	echo "GET   $(@F)"
	curl -sSL "https://raw.githubusercontent.com/DevSolar/solog/refs/tags/v10/$(@F)" -o $@
