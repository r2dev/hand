@echo off

set CommonCompillerFlags=-MTd -nologo -Zo -Od -Gm- -GR- -EHa- -W4 -Gm- -GR- -EHa- -wd4201 -wd4100 -wd4456 -wd4189  -wd4505 -WX -DHANDMADE_WIN32=1 -DHANDMADE_SLOW=1 -DHANDMADE_INTERNAL=1 -Z7 -FC
set CommonLinkerFlags=-incremental:no -opt:ref user32.lib gdi32.lib winmm.lib


IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build

REM 32-bit build
REM cl %CommonCompillerFlags% -DHANDMADE_INTERNAL=1 -Z7 -FC -Fmwin32_handmade.map ..\handmade\code\win32_handmade.cpp /link -subsystem:windows %CommonLinkerFlags% 

REM 64-bit build
del *.pdb > NUL 2> NUL
REM optimization build /O2 /Oi /fp:fast
echo WAITING FOR PDB > lock.tmp
cl %CommonCompillerFlags% ..\handmade\code\handmade.cpp -Fmhandmade.map -LD /link -incremental:no -opt:ref -PDB:handmade_%random%.pdb -EXPORT:GameUpdateAndRender -EXPORT:GameGetSoundSamples 
del lock.tmp
cl %CommonCompillerFlags% ..\handmade\code\win32_handmade.cpp -Fmwin32_handmade.map /link %CommonLinkerFlags% 

popd