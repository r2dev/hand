@echo off

set CommonCompilerFlags=-MTd -nologo -Zo -Od -Gm- -GR- -EHa- -WX -W4 -Gm- -GR- -EHa- -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -wd4302 -wd4311 -WX 
set CommonCompilerFlags=-DHANDMADE_WIN32=1 -DHANDMADE_SLOW=1 -DHANDMADE_INTERNAL=1 -Z7 -FC -D_CRT_SECURE_NO_WARNINGS %CommonCompilerFlags% 

set CommonLinkerFlags=-incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build

del *.pdb > NUL 2> NUL

REM Preprocessor
cl %CommonCompilerFlags% ..\handmade\code\preprocessor.cpp /link %CommonLinkerFlags% 

pushd ..\handmade\code
..\..\build\preprocessor.exe > zha_generated.h
popd

REM Asset builder 
REM cl %CommonCompilerFlags% ..\handmade\code\zha_test.cpp /link %CommonLinkerFlags% 

REM 32-bit build
REM cl %CommonCompilerFlags% -DHANDMADE_INTERNAL=1 -Z7 -FC -Fmwin32_handmade.map ..\handmade\code\win32_handmade.cpp /link -subsystem:windows %CommonLinkerFlags% 

REM 64-bit build

REM optimization build /O2 /Oi /fp:fast
echo WAITING FOR PDB > lock.tmp

cl %CommonCompilerFlags% -O2 -c ..\handmade\code\handmade_optimized.cpp -Fohandmade_optimized.obj -LD

cl %CommonCompilerFlags% ..\handmade\code\handmade.cpp handmade_optimized.obj -Fmhandmade.map -LD /link -incremental:no -opt:ref -PDB:handmade_%random%.pdb -EXPORT:GameUpdateAndRender -EXPORT:GameGetSoundSamples -EXPORT:DEBUGGameFrameEnd

del lock.tmp
cl %CommonCompilerFlags% ..\handmade\code\win32_handmade.cpp -Fmwin32_handmade.map /link %CommonLinkerFlags% 

popd