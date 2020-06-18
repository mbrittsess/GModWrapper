@echo off
setlocal
set gmod_include="C:\Projects\GMech\Reference Code\secretheaders"
set lua_include="C:\Program Files (x86)\Lua\5.1\include"
set DDK="C:\devtools\WindowsDDK"

set CL86="C:\devtools\WindowsDDK\bin\x86\x86\cl.exe"
set LINK86="C:\devtools\WindowsDDK\bin\x86\x86\link.exe"
set LIB86=%LINK86% /LIB

%CL86% /c /I%DDK%\inc\crt /I%DDK%\inc\api /I%lua_include% /I%gmod_include% /Tpgtypes.c /Fogtypes.obj
%LINK86% /DLL /EXPORT:gmod_open /LIBPATH:%DDK%\lib\win7\i386 /LIBPATH:%DDK%\lib\Crt\i386 /LIBPATH:\Projects\GModWrapper gtypes.obj /OUT:gm_gtypes.dll

del gtypes.obj
del gm_gtypes.exp
del gm_gtypes.lib

endlocal