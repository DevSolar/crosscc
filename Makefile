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
         -Wno-c++-keyword \
         -Wno-cast-align \
         -Wno-cast-qual \
         -Wno-implicit-void-ptr-cast \
         -Wno-pre-c11-compat \
         -Wno-unknown-warning-option \
         -Wno-unsafe-buffer-usage \
         -Wno-unused-macros \
         -Wno-used-but-marked-unused

MODULES = src/utils

TEST_MODULES = test/utils_test

SUPPORT_FILES = src/support/cc.h \
                src/support/greatest.h \
                src/support/solog.h

DEPS = $(EXECUTABLES:=.d) $(MODULES:=.d) $(TEST_MODULES:=.d)

JSON = $(EXECUTABLES:=.json) $(MODULES:=.json) $(TEST_MODULES:=.json)


# ----- #
# Rules #
# ----- #

all: $(TARGETS)

FORCE:

clean: FORCE
	$(RM) check-win-api compile_commands.json
	$(RM) $(EXECUTABLES:=.o) $(EXECUTABLES:=.d) $(EXECUTABLES:=.json) $(EXECUTABLES)
	$(RM) $(MODULES:=.o) $(TEST_MODULES:=.o)
	$(RM) $(MODULES:=.d) $(TEST_MODULES:=.d)
	$(RM) $(MODULES:=.json) $(TEST_MODULES:=.json)

mrproper: clean
	$(RM) $(SUPPORT_FILES)

test: test/crosscc_test FORCE
	./test/crosscc_test

check-win-api: $(EXECUTABLES:=.c) $(MODULES:=.c) Makefile $(SUPPORT_FILES)
	echo "CC    $@"
	x86_64-w64-mingw32-clang $(CFLAGS) -fsyntax-only $(EXECUTABLES:=.c) $(MODULES:=.c)
	touch $@

compile_commands.json: $(EXECUTABLES:=.o) $(MODULES:=.o)
	echo "IDX   $@"
	echo "[" > $@
	cat $(JSON) | sed '$$s/,$$//' >> $@
	echo "]" >> $@

crosscc: crosscc.o $(MODULES:=.o)
	echo "LD    $@"
	$(CC) $(CFLAGS) $(LDFLAGS) crosscc.o $(MODULES:=.o) -o $@

test/crosscc_test: test/crosscc_test.o $(TEST_MODULES:=.o) $(MODULES:=.o)
	echo "LD    $@"
	$(CC) $(CFLAGS) $(LDFLAGS) test/crosscc_test.o $(TEST_MODULES:=.o) $(MODULES:=.o) -o $@

$(MODULES:=.o) $(TEST_MODULES:=.o) $(EXECUTABLES:=.o): Makefile $(SUPPORT_FILES)

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
