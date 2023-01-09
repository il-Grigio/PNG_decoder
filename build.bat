@echo off

if exist "bin" (
    RD /S /Q "bin"
)

MD "bin"

COPY "zlib\zlibwapi.dll" "bin\zlibwapi.dll"
@REM COPY "SDL2-2.26.0\lib\x64\SDL2.dll" "bin\SDL2.dll"

clang -o bin\decoderC.exe .\decoderC.c -I .\zlib -l .\zlib\zlibwapi -L .\zlib 