@echo off
setlocal EnableExtensions EnableDelayedExpansion

cd /d "%~dp0"
set "SCRIPT_DIR=%CD%"

rem ----- 환경 설정 -----
if not defined PS2DEV set "PS2DEV=C:\Users\jm\Documents\ps2dev"
if not defined PS2SDK set "PS2SDK=%PS2DEV%\ps2sdk"
if not defined GSKIT set "GSKIT=%PS2DEV%\gsKit"

set "PATH=%PS2DEV%\bin;%PS2DEV%\ee\bin;%PS2DEV%\iop\bin;%PS2DEV%\dvp\bin;%PS2SDK%\bin;%PATH%"
set "PATH=C:\msys64\mingw32\bin;C:\msys64\mingw64\bin;%PATH%"

call :find_python

set "CC=%PS2DEV%\ee\bin\mips64r5900el-ps2-elf-gcc"
set "ELF_NAME=game_engine.elf"

if not exist "%CC%" if not exist "%CC%.exe" (
    echo ERROR: EE compiler not found at %CC%
    echo        PS2DEV=%PS2DEV% 경로를 확인하세요.
    exit /b 1
)

set "CFLAGS=-D_EE -G0 -O2 -Wall -gdwarf-2 -gz"
set "CFLAGS=%CFLAGS% -I%PS2SDK%\ee\include"
set "CFLAGS=%CFLAGS% -I%PS2SDK%\common\include"
set "CFLAGS=%CFLAGS% -I."
set "CFLAGS=%CFLAGS% -I%GSKIT%\include"
set "CFLAGS=%CFLAGS% -I%GSKIT%\ee\include"
set "CFLAGS=%CFLAGS% -I%PS2SDK%\ports\include"

set "LDFLAGS=-T%PS2SDK%\ee\startup\linkfile -O2"
set "LDFLAGS=%LDFLAGS% -L%PS2SDK%\ee\lib -Wl,-zmax-page-size=128"
set "LDFLAGS=%LDFLAGS% -L%GSKIT%\lib -L%GSKIT%\ee\lib"
set "LDFLAGS=%LDFLAGS% -L%PS2SDK%\ports\lib"

set "LIBS=-lgskit -ldmakit -lpad -laudsrv -lpatches -lc -ldebug"

set "SRCS=main.c src\system.c src\audio.c src\sprite.c src\asset.c src\animation.c src\level.c src\physics.c src\render.c src\font.c src\sfx.c"

if "%~1"=="" set "CMD=build"
if not "%~1"=="" set "CMD=%~1"

if /I "%CMD%"=="clean" (
    call :do_clean
    exit /b %ERRORLEVEL%
)
if /I "%CMD%"=="rebuild" (
    call :do_clean || exit /b 1
    call :do_assets || exit /b 1
    call :do_compile || exit /b 1
    exit /b 0
)
if /I "%CMD%"=="iso" (
    call :do_assets || exit /b 1
    call :do_compile || exit /b 1
    call :do_iso || exit /b 1
    exit /b 0
)
if /I "%CMD%"=="build" (
    call :do_assets || exit /b 1
    call :do_compile || exit /b 1
    exit /b 0
)

echo Usage: build.bat [build^|iso^|clean^|rebuild]
exit /b 1

:do_clean
echo === Clean ===
if exist "%ELF_NAME%" del /f /q "%ELF_NAME%" >nul 2>&1
if exist "sprites.pak" del /f /q "sprites.pak" >nul 2>&1
if exist "assets\*.ps2tex" del /f /q "assets\*.ps2tex" >nul 2>&1
for %%S in (%SRCS%) do (
    set "OBJ=%%~dpnS.o"
    if exist "!OBJ!" del /f /q "!OBJ!" >nul 2>&1
)
echo Done.
exit /b 0

:do_assets
echo === Converting sprites ===
if not defined PYTHON_EXE (
    echo WARNING: Python not found, skipping asset conversion.
    echo          수동으로 에셋을 변환하세요.
    exit /b 0
)

if exist "assets\src\*.png" (
    call :run_py tools\png_to_ps2tex.py --all assets\src assets || exit /b 1
) else (
    echo   ^(assets\src에 PNG 없음, 스킵^)
)

if exist "assets\*.ps2tex" (
    call :run_py tools\make_spritepack.py assets sprites.pak || exit /b 1
) else (
    echo   ^(ps2tex 파일 없음, 팩 생성 스킵^)
)

echo === Downsampling WAVs ===
for %%W in (assets\bgm.wav assets\bgm_menu.wav) do (
    if exist "%%W" (
        if not exist "%%W.bak" (
            echo   Converting %%W -^> 11025Hz mono 16bit
            call :run_py tools\wav_convert.py "%%W" || exit /b 1
        ) else (
            echo   ^(skip^) %%W ^(already converted, .bak exists^)
        )
    )
)

echo === Converting SFX to ADPCM ===
if exist "assets\sfx_*.wav" (
    call :run_py tools\wav_to_adpcm.py --all assets || exit /b 1
) else (
    echo   ^(no sfx_*.wav files in assets\, skip^)
)
exit /b 0

:do_compile
echo === Building %ELF_NAME% ===
set "OBJS="
for %%S in (%SRCS%) do (
    set "OBJ=%%~dpnS.o"
    set "OBJS=!OBJS! "!OBJ!""
    echo   CC  %%S
    "%CC%" %CFLAGS% -c "%%S" -o "!OBJ!"
    if errorlevel 1 exit /b 1
)

echo   LD  %ELF_NAME%
"%CC%" %LDFLAGS% -o "%ELF_NAME%" %OBJS% %LIBS%
if errorlevel 1 exit /b 1

echo === Build OK: %ELF_NAME% ===
exit /b 0

:do_iso
echo.
echo === Creating ISO ===
call tools\make_iso.bat
exit /b %ERRORLEVEL%

:find_python
set "PYTHON_EXE="
set "PYTHON_ARGS="
if exist "C:\Users\jm\AppData\Local\Programs\Python\Python310\python.exe" (
    set "PYTHON_EXE=C:\Users\jm\AppData\Local\Programs\Python\Python310\python.exe"
    exit /b 0
)
where py >nul 2>&1 && (
    set "PYTHON_EXE=py"
    set "PYTHON_ARGS=-3"
    exit /b 0
)
where python3 >nul 2>&1 && (
    set "PYTHON_EXE=python3"
    exit /b 0
)
where python >nul 2>&1 && (
    set "PYTHON_EXE=python"
    exit /b 0
)
exit /b 0

:run_py
if not defined PYTHON_EXE exit /b 1
if defined PYTHON_ARGS (
    "%PYTHON_EXE%" %PYTHON_ARGS% %*
) else (
    "%PYTHON_EXE%" %*
)
exit /b %ERRORLEVEL%
