@ECHO off
SetLocal EnableDelayedExpansion

IF NOT EXIST bin mkdir bin
IF NOT EXIST plugins mkdir plugins

SET cc=clang

REM ================= CORE =================
REM ==============
REM Gets list of all C files
SET c_filenames= 
FOR %%f in (source\base\*.c) do SET c_filenames=!c_filenames! %%f
FOR %%f in (source\impl\*.c) do SET c_filenames=!c_filenames! %%f
FOR %%f in (source\core\*.c) do SET c_filenames=!c_filenames! %%f
FOR %%f in (source\os\*.c) do SET c_filenames=!c_filenames! %%f
REM ==============

REM ==============
REM optional layers

ECHO Optional Layer Selected: Render2D
SET c_filenames=!c_filenames! source\opt\render_2d.c

ECHO Optional Layer Selected: UI
SET c_filenames=!c_filenames! source\opt\ui.c
REM ==============

REM ==============
SET compiler_flags=-Wall -Wvarargs -Werror -Wno-unused-function -Wno-format-security -Wno-incompatible-pointer-types-discards-qualifiers -Wno-unused-but-set-variable -Wno-int-to-void-pointer-cast

REM SET compiler_flags=!compiler_flags! -fsanitize=address

SET include_flags=-Isource -Ithird_party/include -Ithird_party/source
SET linker_flags=-g -lshell32 -luser32 -lwinmm -luserenv -lgdi32 -Lthird_party/lib -lbin\libimpl
SET defines=-D_DEBUG -D_CRT_SECURE_NO_WARNINGS
SET backend=-DBACKEND_GL33
REM ==============

ECHO Building core.dll...
%cc% %c_filenames% %compiler_flags% -shared %defines% -DCORE %backend% %include_flags% %linker_flags% -obin/core.dll
REM ================= CORE END =================

REM ================= PNG PLUGIN ===============
ECHO Building png.dll
%cc% source/plugins/png_plug.c %compiler_flags% -shared %defines% -DPLUGIN %backend% %include_flags% %linker_flags% -lbin/core -oplugins/png.dll
REM ================= PNG PLUGIN END ===============

REM ================= PARTICLE PLUGIN ===============
ECHO Building psys.dll
%cc% source/plugins/psys_plug.c %compiler_flags% -shared %defines% -DPLUGIN %backend% %include_flags% %linker_flags% -lbin/core -oplugins/psys.dll
REM ================= PARTICLE PLUGIN END ===============

REM ================= SOLID STATE PLUGIN ===============
ECHO Building solidstate.dll
%cc% source/plugins/solid_state_plug.c %compiler_flags% -shared %defines% -DPLUGIN %backend% %include_flags% %linker_flags% -lbin/core -oplugins/solidstate.dll
REM ================= SOLID STATE END ===============

REM ================= CLIENT =================
ECHO Building client.exe
%cc% source/main.c source/client/fexp.c %compiler_flags% %defines% -DPLUGIN %backend% %include_flags% %linker_flags% -lbin/core -obin/client.exe
REM ================= CLIENT END =================
