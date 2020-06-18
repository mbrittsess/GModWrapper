@echo off
del lua51.lib
del tests\gm_hello.dll
call build.bat
cd tests
call build_hello.bat
cd ..
set gmod_binary_dir=C:\Program Files (x86)\Steam\steamapps\infectiousfight\garrysmod\garrysmod\lua\includes\modules
set gmod_binary_dir2=C:\Program Files (x86)\Steam\steamapps\infectiousfight\garry's mod beta\garrysmodbeta\lua\includes\modules
del %gmod_binary_dir%\gm_hello.dll
del %gmod_binary_dir2%\gm_hello.dll
copy /B .\tests\gm_hello.dll "%gmod_binary_dir%\gm_hello.dll"
copy /B .\tests\gm_hello.dll "%gmod_binary_dir2%\gm_hello.dll"