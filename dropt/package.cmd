@echo off

setlocal

if not defined DROPT_VERSION set DROPT_VERSION=%~1
if "%DROPT_VERSION%" == "" (
    (echo.No version number specified.) 1>&2
    exit /b 1
)

:: Verify that everything builds.  This doesn't provide complete coverage,
:: but it should be good enough.
::
nmake /NOLOGO /f Makefile.vcwin32 all
nmake /NOLOGO /f Makefile.vcwin32 DEBUG=1 all
nmake /NOLOGO /f Makefile.vcwin32 _UNICODE=1 all
nmake /NOLOGO /f Makefile.vcwin32 DROPT_NO_STRING_BUFFERS=1 all
nmake /NOLOGO /f Makefile.vcwin32 DEBUG=1 _UNICODE=1 DROPT_NO_STRING_BUFFERS=1 all

7z a -tzip -mx=9 -mcu=on -r -- "build\dropt-%DROPT_VERSION%.zip" ^
    dropt.c ^
    dropt.h ^
    dropt_example.c ^
    dropt_handlers.c ^
    dropt_string.c ^
    dropt_string.h ^
    droptxx.cpp ^
    droptxx.hpp ^
    LICENSE ^
    Makefile.gcc ^
    Makefile.vcwin32 ^
    README.html ^
    test_dropt.c
