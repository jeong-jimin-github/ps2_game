@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_DIR=%%~fI"

set "ELF_NAME=game_engine.elf"
set "ELF_FILE=%PROJECT_DIR%\%ELF_NAME%"
set "STAGING=%PROJECT_DIR%\.iso_staging"

if not defined PS2_GAME_ID (
    set "GAME_ID=SLUS_209.99"
) else (
    set "GAME_ID=%PS2_GAME_ID%"
)
call :to_upper GAME_ID GAME_ID
echo(%GAME_ID%| findstr /R /C:"^[A-Z][A-Z][A-Z][A-Z]_[0-9][0-9][0-9]\.[0-9][0-9]$" >nul || (
    echo WARNING: PS2_GAME_ID '%PS2_GAME_ID%' is invalid. Using default SLUS_209.99
    set "GAME_ID=SLUS_209.99"
)

if not defined PS2_GAME_TITLE (
    set "GAME_TITLE_OPL=PS2_PLATFORMER"
) else (
    set "GAME_TITLE_OPL=%PS2_GAME_TITLE%"
)
call :to_upper GAME_TITLE_OPL GAME_TITLE_OPL
set "GAME_TITLE_OPL=%GAME_TITLE_OPL: =_%"
if "%GAME_TITLE_OPL%"=="" set "GAME_TITLE_OPL=PS2_PLATFORMER"

set "VOL_LABEL=%GAME_ID%_%GAME_TITLE_OPL%"
set "VOL_LABEL=%VOL_LABEL:~0,32%"

if "%~1"=="" (
    set "ISO_FILE=%PROJECT_DIR%\%GAME_ID%.%GAME_TITLE_OPL%.iso"
) else (
    set "ISO_FILE=%~1"
)

call :detect_iso_tool || exit /b 1

if not exist "%ELF_FILE%" (
    echo ERROR: %ELF_FILE% not found. Run build first.
    exit /b 1
)

if exist "%STAGING%" rmdir /s /q "%STAGING%"
mkdir "%STAGING%" >nul 2>&1

> "%STAGING%\SYSTEM.CNF" (
    echo BOOT2 = cdrom0:\%GAME_ID%;1
    echo VER = 1.00
    echo VMODE = NTSC
)

copy /y "%ELF_FILE%" "%STAGING%\%GAME_ID%" >nul

if exist "%PROJECT_DIR%\audsrv.irx" copy /y "%PROJECT_DIR%\audsrv.irx" "%STAGING%\AUDSRV.IRX" >nul
if exist "%PROJECT_DIR%\freesd.irx" copy /y "%PROJECT_DIR%\freesd.irx" "%STAGING%\FREESD.IRX" >nul

if exist "%PROJECT_DIR%\sprites.pak" (
    copy /y "%PROJECT_DIR%\sprites.pak" "%STAGING%\SPRITES.PAK" >nul
) else (
    echo WARNING: sprites.pak not found. Run build assets first.
)

for %%F in ("%PROJECT_DIR%\assets\bgm*.wav") do (
    if exist "%%~fF" (
        set "BASE=%%~nxF"
        call :to_upper BASE BASE_UPPER
        copy /y "%%~fF" "%STAGING%\!BASE_UPPER!" >nul
    )
)

for %%F in ("%PROJECT_DIR%\assets\*.adpcm") do (
    if exist "%%~fF" (
        set "BASE=%%~nxF"
        call :to_upper BASE BASE_UPPER
        copy /y "%%~fF" "%STAGING%\!BASE_UPPER!" >nul
    )
)

for %%F in ("%PROJECT_DIR%\levels\*.txt") do (
    if exist "%%~fF" (
        set "BASE=%%~nxF"
        call :to_upper BASE BASE_UPPER
        copy /y "%%~fF" "%STAGING%\!BASE_UPPER!" >nul
    )
)

for %%F in ("%PROJECT_DIR%\assets\*.fnt") do (
    if exist "%%~fF" (
        set "BASE=%%~nxF"
        call :to_upper BASE BASE_UPPER
        if not exist "%STAGING%\!BASE_UPPER!" copy /y "%%~fF" "%STAGING%\!BASE_UPPER!" >nul
    )
)

for %%F in ("%PROJECT_DIR%\assets\font*.ps2tex") do (
    if exist "%%~fF" (
        set "BASE=%%~nxF"
        call :to_upper BASE BASE_UPPER
        if not exist "%STAGING%\!BASE_UPPER!" copy /y "%%~fF" "%STAGING%\!BASE_UPPER!" >nul
    )
)

echo === Staging contents ===
for %%F in ("%STAGING%\*") do (
    if exist "%%~fF" echo   %%~nxF
)
echo.

if exist "%ISO_FILE%" del /f /q "%ISO_FILE%" >nul 2>&1

if /I "%ISOTOOL%"=="pycdlib" (
    echo Using pycdlib ^(ISO 9660 + UDF^)...
    call :run_py "%SCRIPT_DIR%make_udf_iso.py" "%STAGING%" "%ISO_FILE%" "%VOL_LABEL%" "%VOL_LABEL%" || exit /b 1
) else if /I "%ISOTOOL%"=="xorriso" (
    echo WARNING: xorriso cannot produce UDF. ESR Patcher may not work.
    echo          Install pycdlib: pip install pycdlib
    xorriso -as mkisofs --norock -iso-level 2 -D -sysid PLAYSTATION -V "%VOL_LABEL%" -A "%VOL_LABEL%" -pad -o "%ISO_FILE%" "%STAGING%"
    if errorlevel 1 exit /b 1
) else (
    "%ISOTOOL%" -iso-level 2 -udf -D -sysid PLAYSTATION -V "%VOL_LABEL%" -A "%VOL_LABEL%" -pad -o "%ISO_FILE%" "%STAGING%"
    if errorlevel 1 exit /b 1
)

if exist "%STAGING%" rmdir /s /q "%STAGING%"

echo.
echo === ISO created: %ISO_FILE% ===
echo Game ID      : %GAME_ID%
echo Game Title   : %GAME_TITLE_OPL%
echo Volume Label : %VOL_LABEL%
echo.
echo Test in PCSX2:  File -^> Run ISO
echo Burn to CD-R:   imgburn / cdrecord at 4x-8x speed
exit /b 0

:detect_iso_tool
set "ISOTOOL="
set "PYTHON_EXE="
set "PYTHON_ARGS="

call :find_python_with_pycdlib
if defined PYTHON_EXE (
    set "ISOTOOL=pycdlib"
    exit /b 0
)

where mkisofs >nul 2>&1 && (
    set "ISOTOOL=mkisofs"
    exit /b 0
)
where genisoimage >nul 2>&1 && (
    set "ISOTOOL=genisoimage"
    exit /b 0
)
where xorriso >nul 2>&1 && (
    set "ISOTOOL=xorriso"
    exit /b 0
)

echo ERROR: No ISO tool found.
echo.
echo Install on Windows MSYS2:  pacman -S xorriso
echo Install on Python env:     pip install pycdlib
exit /b 1

:find_python_with_pycdlib
call :check_candidate "C:\Users\jm\AppData\Local\Programs\Python\Python310\python.exe" "" && exit /b 0
call :check_candidate "C:\Users\jm\AppData\Local\Programs\Python\Python311\python.exe" "" && exit /b 0
call :check_candidate "C:\Users\jm\AppData\Local\Programs\Python\Python312\python.exe" "" && exit /b 0
call :check_candidate "py" "-3" && exit /b 0
call :check_candidate "python3" "" && exit /b 0
call :check_candidate "python" "" && exit /b 0
exit /b 1

:check_candidate
set "CANDIDATE_EXE=%~1"
set "CANDIDATE_ARGS=%~2"
call :check_pycdlib
if %ERRORLEVEL% EQU 0 (
    set "PYTHON_EXE=%CANDIDATE_EXE%"
    set "PYTHON_ARGS=%CANDIDATE_ARGS%"
    exit /b 0
)
exit /b 1

:check_pycdlib
if not "%CANDIDATE_EXE%"=="py" (
    if not "%CANDIDATE_EXE%"=="python" if not "%CANDIDATE_EXE%"=="python3" (
        if not exist "%CANDIDATE_EXE%" exit /b 1
    )
)
if "%CANDIDATE_ARGS%"=="" (
    "%CANDIDATE_EXE%" -c "import pycdlib" >nul 2>&1
) else (
    "%CANDIDATE_EXE%" %CANDIDATE_ARGS% -c "import pycdlib" >nul 2>&1
)
exit /b %ERRORLEVEL%

:run_py
if "%PYTHON_ARGS%"=="" (
    "%PYTHON_EXE%" %*
) else (
    "%PYTHON_EXE%" %PYTHON_ARGS% %*
)
exit /b %ERRORLEVEL%

:to_upper
set "_TU_IN=!%~1!"
set "_TU_IN=!_TU_IN:a=A!"
set "_TU_IN=!_TU_IN:b=B!"
set "_TU_IN=!_TU_IN:c=C!"
set "_TU_IN=!_TU_IN:d=D!"
set "_TU_IN=!_TU_IN:e=E!"
set "_TU_IN=!_TU_IN:f=F!"
set "_TU_IN=!_TU_IN:g=G!"
set "_TU_IN=!_TU_IN:h=H!"
set "_TU_IN=!_TU_IN:i=I!"
set "_TU_IN=!_TU_IN:j=J!"
set "_TU_IN=!_TU_IN:k=K!"
set "_TU_IN=!_TU_IN:l=L!"
set "_TU_IN=!_TU_IN:m=M!"
set "_TU_IN=!_TU_IN:n=N!"
set "_TU_IN=!_TU_IN:o=O!"
set "_TU_IN=!_TU_IN:p=P!"
set "_TU_IN=!_TU_IN:q=Q!"
set "_TU_IN=!_TU_IN:r=R!"
set "_TU_IN=!_TU_IN:s=S!"
set "_TU_IN=!_TU_IN:t=T!"
set "_TU_IN=!_TU_IN:u=U!"
set "_TU_IN=!_TU_IN:v=V!"
set "_TU_IN=!_TU_IN:w=W!"
set "_TU_IN=!_TU_IN:x=X!"
set "_TU_IN=!_TU_IN:y=Y!"
set "_TU_IN=!_TU_IN:z=Z!"
set "%~2=%_TU_IN%"
exit /b 0
