@echo off
setlocal
::If you are not Matthew Sessions, or you are but compiling on Lionel instead of Martin, you'll need to adjust these environment variables:
set SDK="C:\devtools\WindowskSDK"
set DDK="C:\devtools\WindowsDDK"
set LuaP="C:\Program Files (x86)\Lua\5.1"

set CL86="C:\devtools\WindowsDDK\bin\x86\x86\cl.exe"
set LINK86="C:\devtools\WindowsDDK\bin\x86\x86\link.exe"

%CL86% /c /MD /I%SDK%\Include /I%DDK%\inc\crt /I%DDK%\inc\api /I%LuaP%\include /arch:SSE2 /Ox /Tchello.c /Fohello.obj
if not errorlevel 1 (
%LINK86% /DLL /EXPORT:luaopen_hello /LIBPATH:%DDK%\lib\win7\i386 /LIBPATH:%DDK%\lib\Crt\i386 /LIBPATH:\Projects\GModWrapper /DEFAULTLIB:lua51.lib hello.obj /OUT:gm_hello.dll
)

del hello.obj
del gm_hello.exp
del gm_hello.lib
endlocal