@echo off
setlocal EnableExtensions EnableDelayedExpansion

:: ============================================================================
:: CyMedia release package generator
:: Usage:
::   GenRelease.bat        Incremental package
::   GenRelease.bat clean  Remove old package first
:: ============================================================================

set "batDir=%~dp0"
if "%batDir:~-1%"=="\" set "batDir=%batDir:~0,-1%"

set "srcRoot=%batDir%\CyMedia"
set "demoRoot=%batDir%\CyMediaDisTest"
set "buildRoot=%batDir%\OutFile"

set "genDir=%buildRoot%\CyMedia_Release"
set "incDir=%genDir%\include"
set "binDir=%genDir%\Bin"
set "libDir=%genDir%\Lib"
set "demoOut=%genDir%\Demo"

set "doClean=0"
if /I "%~1"=="clean" set "doClean=1"

echo ============================================================================
echo  CyMedia release package
echo  Source : %srcRoot%
echo  Output : %genDir%
echo ============================================================================

if not exist "%buildRoot%" (
    echo [ERROR] Build output directory not found: %buildRoot%
    echo         Build CyMedia.sln or the CMake project first.
    pause
    exit /b 1
)

if "%doClean%"=="1" (
    echo [INFO] Cleaning old package...
    if exist "%genDir%" rmdir /s /q "%genDir%"
)

call :MakeDir "%incDir%\View"
call :MakeDir "%incDir%\View\Annotation"
call :MakeDir "%incDir%\View\Dialogs"
call :MakeDir "%incDir%\View\Widgets"
call :MakeDir "%incDir%\Parse"
call :MakeDir "%incDir%\Calculate"
call :MakeDir "%incDir%\common"

call :MakeDir "%binDir%\x64\Debug"
call :MakeDir "%binDir%\x64\Release"
call :MakeDir "%binDir%\Win32\Debug"
call :MakeDir "%binDir%\Win32\Release"
call :MakeDir "%binDir%\colorMap"

call :MakeDir "%libDir%\x64\Debug"
call :MakeDir "%libDir%\x64\Release"
call :MakeDir "%libDir%\Win32\Debug"
call :MakeDir "%libDir%\Win32\Release"

call :MakeDir "%demoOut%"

:: ----------------------------------------------------------------------------
:: Documentation
:: ----------------------------------------------------------------------------
echo [INFO] Copying documentation...
if exist "%srcRoot%\README.md" (
    xcopy /y /d /i "%srcRoot%\README.md" "%genDir%\README.md" >nul
) else (
    if exist "%batDir%\README.md" xcopy /y /d /i "%batDir%\README.md" "%genDir%\README.md" >nul
)
if exist "%srcRoot%\src\View\README.md" (
    xcopy /y /d /i "%srcRoot%\src\View\README.md" "%incDir%\View\README.md" >nul
)

if exist "%batDir%\Third-Party_Licenses" (
    xcopy /y /d /i /e "%batDir%\Third-Party_Licenses\*" "%genDir%\Third-Party_Licenses\" >nul
)

:: ----------------------------------------------------------------------------
:: Public headers
:: ----------------------------------------------------------------------------
echo [INFO] Copying headers...
call :CopyFile "%srcRoot%\src\common\CyMediaBaseDef.h" "%incDir%\"
call :CopyFile "%srcRoot%\src\common\CyMediaBaseDef.h" "%incDir%\common\"
call :CopyHeaders "%srcRoot%\src\common" "%incDir%\common"

call :CopyFile "%srcRoot%\src\View\Display.h" "%incDir%\View\"
call :CopyFile "%srcRoot%\src\View\DisplayTypes.h" "%incDir%\View\"
call :CopyFile "%srcRoot%\src\View\Display.h" "%incDir%\"
call :CopyFile "%srcRoot%\src\View\DisplayTypes.h" "%incDir%\"
call :CopyFile "%srcRoot%\src\View\Annotation\BaseItem.h" "%incDir%\View\Annotation\"
call :CopyFile "%srcRoot%\src\View\Annotation\DrawItem.h" "%incDir%\View\Annotation\"
call :CopyHeaders "%srcRoot%\src\View\Dialogs" "%incDir%\View\Dialogs"
call :CopyFile "%srcRoot%\src\View\Widgets\CyMediaDisGrayStretch.h" "%incDir%\View\Widgets\"
call :CopyFile "%srcRoot%\src\View\Widgets\CyMediaDisGrayTest.h" "%incDir%\View\Widgets\"
call :CopyFile "%srcRoot%\src\View\Widgets\CyMediaRecTimeW.h" "%incDir%\View\Widgets\"

call :CopyFile "%srcRoot%\src\Parse\ImageParse.h" "%incDir%\Parse\"
call :CopyFile "%srcRoot%\src\Parse\VideoParse.h" "%incDir%\Parse\"
call :CopyFile "%srcRoot%\src\Parse\VideoParseBase.h" "%incDir%\Parse\"
call :CopyFile "%srcRoot%\src\Parse\FileFunc.h" "%incDir%\Parse\"

call :CopyHeaders "%srcRoot%\src\Calculate" "%incDir%\Calculate"

:: ----------------------------------------------------------------------------
:: Bin / Lib
:: ----------------------------------------------------------------------------
echo [INFO] Copying CyMedia binaries and libs...

call :CopyArch "x64" "Debug"
call :CopyArch "x64" "Release"
call :CopyArch "Win32" "Debug"
call :CopyArch "Win32" "Release"

:: FFmpeg runtime DLLs
if exist "%srcRoot%\3rdparty\ffmpeg\Bin\x64" (
    xcopy /y /d /i /e "%srcRoot%\3rdparty\ffmpeg\Bin\x64\*" "%binDir%\x64\Debug\" >nul
    xcopy /y /d /i /e "%srcRoot%\3rdparty\ffmpeg\Bin\x64\*" "%binDir%\x64\Release\" >nul
)
if exist "%srcRoot%\3rdparty\ffmpeg\Bin\Win32" (
    xcopy /y /d /i /e "%srcRoot%\3rdparty\ffmpeg\Bin\Win32\*" "%binDir%\Win32\Debug\" >nul
    xcopy /y /d /i /e "%srcRoot%\3rdparty\ffmpeg\Bin\Win32\*" "%binDir%\Win32\Release\" >nul
)
:: Color maps are external runtime data (not embedded in the DLL).
if exist "%srcRoot%\resources\colorMap_ALL" (
    echo [INFO] Copying ColorMap...
    xcopy /y /d /i /e "%srcRoot%\resources\colorMap_ALL\*" "%binDir%\colorMap\" >nul
)

:: ----------------------------------------------------------------------------
:: Demo: ship the CyMediaDisTest project source, not a prebuilt application
:: ----------------------------------------------------------------------------
echo [INFO] Copying CyMediaDisTest source + CMake (no prebuilt executable)...
call :MakeDir "%demoOut%\CyMediaDisTest"
if exist "%demoRoot%" (
    robocopy "%demoRoot%" "%demoOut%\CyMediaDisTest" /E /XO /XD ".vs" "build" "Debug" "Release" "x64" "Win32" /XF "*.user" "*.aps" "*.sdf" "*.VC.db" >nul
    if errorlevel 8 (
        echo   [WARN] Failed to copy CyMediaDisTest source project.
    ) else (
        echo   [OK] Demo source: Demo\CyMediaDisTest
    )
) else (
    echo   [SKIP] CyMediaDisTest source directory not found.
)

echo.
echo ============================================================================
echo  Package generated: %genDir%
echo    include\   public headers + View README
echo    Bin\       CyMedia.dll + FFmpeg DLLs + colorMap
echo    Lib\       CyMedia.lib + PDB
echo    Demo\      CyMediaDisTest source + CMake (no prebuilt exe)
echo ============================================================================

endlocal & exit /b 0
@REM echo.
@REM echo Press any key to exit...
@REM pause >nul
@REM endlocal & exit /b 0

:: ============================================================================
:: Helpers
:: ============================================================================

:MakeDir
if not exist "%~1" mkdir "%~1" >nul 2>nul
exit /b 0

:CopyFile
if not exist "%~1" (
    echo   [WARN] Missing file: %~1
    exit /b 0
)
if not exist "%~2" mkdir "%~2" >nul 2>nul
xcopy /y /d /i "%~1" "%~2" >nul
if errorlevel 2 echo   [WARN] Copy failed: %~1
exit /b 0

:CopyHeaders
if not exist "%~1\" (
    echo   [WARN] Missing header directory: %~1
    exit /b 0
)
if not exist "%~2" mkdir "%~2" >nul 2>nul
xcopy /y /d /i "%~1\*.h" "%~2\" >nul 2>nul
if errorlevel 2 echo   [WARN] Header copy failed: %~1
exit /b 0

:CopyArch
set "arch=%~1"
set "cfg=%~2"
set "outLib=%libDir%\%arch%\%cfg%"
set "outBin=%binDir%\%arch%\%cfg%"
call :CopyFile "%buildRoot%\%arch%\%cfg%\CyMedia.dll" "%outBin%\"
call :CopyFile "%buildRoot%\%arch%\%cfg%\CyMedia.lib" "%outLib%\"
if /I "%cfg%"=="Debug" (
    call :CopyFile "%buildRoot%\%arch%\%cfg%\CyMedia.pdb" "%outLib%\"
)
exit /b 0
