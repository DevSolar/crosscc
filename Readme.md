# CrossCC

A bit like CMake toolchain files, just for plain C / make.

## User's Story

You are writing something in C.[1]

You want it to be as portable as possible. So you keep dependencies light. Your build system is GNU make, perhaps even pure POSIX make.

You want to *test* your code, on as many targets as you can make happen.

Suddenly, your Makefile moves into focus. How do you "make it work" for all those rather distinct targets?

You could add conditional expressions to your Makefile. You could define environment variables. You could write individual Makefiles for each target, and *somehow* keep them all in sync.

Or you could use CrossCC, which will make your Makefile *cleaner* even if you don't cross-compile, and makes cross-compilation as easy as calling 'make TARGET=aarch64', or 'make TARGET=amiga31'.

Including actually *running* the generated executables, or *debugging* them.

## What it does

### Toolchain Frontend

CrossCC's main purpose is a *development toolchain frontend*. You invoke it as a compiler ('crcc -c main.c -o main.o'), or as a linker ('crld func.o main.o -o prog'), or as a debugger ('crdb prog').

You can pass a target specification ('crcc -Wx,target=aarch64 ...'), or call a target-specific symlink ('crcc-aarch64 ...'). If you don't, CrossCC assumes the 'native' target.

CrossCC then loads the target configuration (a '<target>.ini') file) containing all the secret sauce needed for that target -- Specific compiler flags, the location for the sysroot etc. -- and "enriches" the command line it has been given with that "secret sauce".

So a call like this in your makefile...

```
crcc-arch64 -c main.c -o main.o
```

...could be turned into:

```
gcc --target=aarch64 -Wall -Wextra --sysroot=sysroots/arch64 -Bsysroots/arch64/lib/gcc/aarch64/11.2.1 -Lsysroots/arch64/lib/gcc/aarch64/11.2.1 -c main.c -o main.o
```

And if you need to change any of the settings, you can do it in the target's .ini file, in one single place, instead of hunting invokations through one or more Makefiles (and then having to re-test the *other* targets as well).

### Sysroot Provider

To relieve you (or your Makefile) from checking whether the target's sysroot actually exists, a target's .ini file also contains instructions on how to set it up if it doesn't exist. This can be as simple as downloading and unpacking a sysroot archive, or calling an external process to do the provisioning.

### Executable / Debugger Runner

Apart from the information on how to *build* binaries, a CrossCC target .ini also contains information on how to *run* those binaries, presuming appropriate emulators are available. So after you built your binary, just run 'crdb' to run it in emulation and attach a debugger to it.

I am sure you know how to do it manually -- call the correct emulator, tell it to open the GDB port, start GDB, tell it to *connect* to that port... I am not claiming CrossCC does anything magical, it just makes it easier to do so.

### Flexible Configuration

CrossCC's .ini configuration format is *very* flexible. You can provide multiple alternatives for search paths or binary names; resolve environment variables; automatically choose the latest version of a tool or directory (i.e., picking bin/clang-11 over bin/clang-9); or re-use settings in path resolution (e.g. '-L {sysroot.dir}/libs').

Since CrossCC first checks the current working directory for its central config.ini before checking for a user- or system-wide one, and the config.ini contains the search path for the target .ini files, you can have a global set of target.ini's as well as project-specific ones, giving you full authority.

### Portable Makefile Tools

As a secondary courtesy to Makefile authors, CrossCC offers a range of portable implementations for common operations in Makefiles that become tricky once that Makefile is supposed to run on vastly different targets. This functionality was inspired by 'cmake -E' offering similar services:

concat, copy (--if-newer), copydir, echo, set_env, unset_env, makedir (recursive), checksum, rm (--recurse, ignoring missing files), sleep,, touch (--create), link, unpack (supporting tarballs, zipfiles, and lha archives)
