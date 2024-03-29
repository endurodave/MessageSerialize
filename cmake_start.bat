REM cmake -B ../Build -T host=x86 -S . -G 'Visual Studio 17 2022' -A Win32
REM cmake -B ../BuildGCC -G "Unix Makefiles"
REM cmake -B ../BuildGCC -G "Eclipse CDT4 - Unix Makefiles"

cmake -B ../Build -G "Visual Studio 17 2022"
