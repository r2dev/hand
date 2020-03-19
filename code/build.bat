@echo off

set CommonCompillerFlags=-MT -nologo -Oi -Gm- -GR- -EHa- -W4 -wd4201 -wd4100 -wd4189 -DHANDMADE_WIN32=1 -DHANDMADE_SLOW=1
set CommonLinkerFlags=-opt:ref user32.lib gdi32.lib winmm.lib


IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build

REM 32-bit build
REM cl %CommonCompillerFlags% -DHANDMADE_INTERNAL=1 -Z7 -FC -Fmwin32_handmade.map ..\handmade\code\win32_handmade.cpp /link -subsystem:windows %CommonLinkerFlags% 

REM 64-bit build
cl %CommonCompillerFlags% -DHANDMADE_INTERNAL=1 -Z7 -FC -Fmwin32_handmade.map ..\handmade\code\win32_handmade.cpp /link %CommonLinkerFlags% 

popd