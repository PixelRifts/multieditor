@ECHO off
SetLocal EnableDelayedExpansion

IF NOT EXIST bin mkdir bin
IF NOT EXIST plugins mkdir plugins

SET cc=clang

REM ==============
REM Gets list of all C files
SET c_filenames= 
FOR %%f in (source\impl\*.c) do SET c_filenames=!c_filenames! %%f
REM ==============

REM ==============
SET compiler_flags=-Wall -Wvarargs -Werror -Wno-unused-function -Wno-format-security -Wno-incompatible-pointer-types-discards-qualifiers -Wno-unused-but-set-variable -Wno-int-to-void-pointer-cast

REM SET compiler_flags=!compiler_flags! -fsanitize=address

SET include_flags=-Isource -Ithird_party/include -Ithird_party/source
SET linker_flags=-g -lshell32 -luser32 -lwinmm -luserenv -lgdi32 -Lthird_party/lib
SET defines=-D_DEBUG -D_CRT_SECURE_NO_WARNINGS
SET backend=-DBACKEND_GL33
REM ==============

%cc% %c_filenames% %compiler_flags% -shared %defines% %backend% %include_flags% -obin\libimpl.o
llvm-ar rc bin/libimpl.lib bin/libimpl.o
