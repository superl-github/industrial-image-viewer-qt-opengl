@echo off

::DirVariable
set "batDir=%~dp0"
set "LIBSrcDir=%batDir%\OutFile"
set "GenDir=%batDir%\OutFile\CyMedia"
set "GenDir_include=%GenDir%\include\"

set "GenDir_Bin=%GenDir%\Bin\"
set "GenDir_Bin_32=%GenDir%\Bin\Win32"
set "GenDir_Bin_64=%GenDir%\Bin\x64"

set "GenDir_Lib_32=%GenDir%\Lib\Win32"
set "GenDir_Lib_64=%GenDir%\Lib\x64"

set "GenDir_Demo=%GenDir%\Demo"

::mackDir
md "%GenDir_include%" 2>nul
md "%GenDir_include%\View\" 2>nul
md "%GenDir_include%\View\DrawItem\" 2>nul
md "%GenDir_include%\Parse\" 2>nul


md "%GenDir_Bin_32%\Debug\" 2>nul
md "%GenDir_Bin_32%\Release\" 2>nul
md "%GenDir_Bin_64%\Debug\" 2>nul
md "%GenDir_Bin_64%\Release\" 2>nul
md "%GenDir_Bin%\colorMap\" 2>nul

md "%GenDir_Lib_32%\Debug\" 2>nul
md "%GenDir_Lib_32%\Release\" 2>nul
md "%GenDir_Lib_64%\Debug\" 2>nul
md "%GenDir_Lib_64%\Release\" 2>nul

md "%GenDir_Demo%" 2>nul

::doc
copy "README.md" "%GenDir%"

::include->CyMediaDis
echo "copy Header"
copy "%batDir%\CyMedia\src\common\CyMediaBaseDef.h" "%GenDir_include%"
copy "%batDir%\CyMedia\src\View\Display.h" "%GenDir_include%"
copy "%batDir%\CyMedia\src\View\DrawItem\BaseItem.h" "%GenDir_include%"
copy "%batDir%\CyMedia\src\View\DrawItem\BaseItem.h" "%GenDir_include%"
copy "%batDir%\CyMedia\LicenseW.h" "%GenDir_include%"

::include->CyMediaParse
copy "%batDir%\CyMedia\src\Parse\ImageParse.h" "%GenDir_include%\Parse"
copy "%batDir%\CyMedia\src\Parse\VideoParse.h" "%GenDir_include%\Parse"


::Bin->colorMaP
echo "copy ColorMap"
xcopy "%batDir%\CyMedia\resources\colorMap_ALL" "%GenDir_Bin%colorMap\" /i/s/y

::Lib/Bin->x64
echo "copy Bin/Lib->x64"
copy "%LIBSrcDir%\x64\Debug\CyMedia.lib" "%GenDir_Lib_64%\Debug\"
copy "%LIBSrcDir%\x64\Debug\CyMedia.pdb" "%GenDir_Lib_64%\Debug\"
copy "%LIBSrcDir%\x64\Debug\CyMedia.dll" "%GenDir_Bin_64%\Debug\"

xcopy "%batDir%\CyMedia\3rdparty\ffmpeg\Bin\x64" "%GenDir_Bin_64%\Debug\" /i/s/y

copy "%LIBSrcDir%\x64\Release\CyMedia.lib" "%GenDir_Lib_64%\Release\"
copy "%LIBSrcDir%\x64\Release\CyMedia.dll" "%GenDir_Bin_64%\Release\"

xcopy "%batDir%\CyMedia\3rdparty\ffmpeg\Bin\x64" "%GenDir_Bin_64%\Release\" /i/s/y

::Lib/Bin->Win32
echo "copy Bin/Lib->Win32"
copy "%LIBSrcDir%\Win32\Debug\CyMedia.lib" "%GenDir_Lib_32%\Debug\"
copy "%LIBSrcDir%\Win32\Debug\CyMedia.pdb" "%GenDir_Lib_32%\Debug\"
copy "%LIBSrcDir%\Win32\Debug\CyMedia.dll" "%GenDir_Bin_32%\Debug\"

xcopy "%batDir%\CyMedia\3rdparty\ffmpeg\Bin\Win32" "%GenDir_Bin_32%\Debug\" /i/s/y

copy "%LIBSrcDir%\Win32\Release\CyMedia.lib" "%GenDir_Lib_32%\Release\"
copy "%LIBSrcDir%\Win32\Release\CyMedia.dll" "%GenDir_Bin_32%\Release\"

xcopy "%batDir%\CyMedia\3rdparty\ffmpeg\Bin\Win32" "%GenDir_Bin_32%\Release\" /i/s/y
::Demo
