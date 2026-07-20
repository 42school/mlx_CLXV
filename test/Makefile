


UNAME_S:=$(shell uname -s)

# the root Makefile builds libmlx.dylib on macOS (native shared-library
# convention there, via -dynamiclib) and libmlx.so everywhere else; on
# macOS an rpath is also needed, or dyld won't find a bare "../libmlx.dylib"
# reference at runtime (it doesn't search relative to the executable by
# default the way some other loaders do)
ifeq ($(UNAME_S),Darwin)
MLXLIB=../libmlx.dylib
RPATH=-Wl,-rpath,@executable_path/..
else
MLXLIB=../libmlx.so
RPATH=
endif

all:
	clang -o mlxtest main.c -I../src $(MLXLIB) $(RPATH) -Wall -Werror
	clang -o mlxspeed speed.c -I../src $(MLXLIB) $(RPATH) -Wall -Werror -gdwarf-4
