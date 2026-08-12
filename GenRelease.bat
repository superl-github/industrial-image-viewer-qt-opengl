@echo off

::DirVariable
set "LIBSrcDir=OutFile"
set "GenDir=OutFile\CyMedia"
set "GenDir_include=%GenDir%\include\"

set "GenDir_Bin=%GenDir%\Bin\"
set "GenDir_Bin_32=%GenDir%\Bin\Win32"
set "GenDir_Bin_64=%GenDir%\Bin\x64"

set "GenDir_Lib_32=%GenDir%\Lib\Win32"
set "GenDir_Lib_64=%GenDir%\Lib\x64"

set "GenDir_Demo=%GenDir%\Demo"

::mackDir
md "%GenDir_include%" 2>nul
md "%GenDir_include%\CyMediaDis\" 2>nul
md "%GenDir_include%\CyMediaDis\drawItem\" 2>nul
md "%GenDir_include%\CyMediaParse\" 2>nul


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
copy "CyMedia\CyMediaBaseDef.h" "%GenDir_include%"
copy "CyMedia\CyMediaDis.h" "%GenDir_include%"

copy "CyMedia\CyMediaDis\drawItem\CyDisDrawItem.h" "%GenDir_include%\CyMediaDis\drawItem\"
copy "CyMedia\CyMediaDis\drawItem\BaseItem.h" "%GenDir_include%\CyMediaDis\drawItem\"
::include->CyMediaParse
copy "CyMedia\CyMediaParse\CyMediaImageParse.h" "%GenDir_include%\CyMediaParse"
copy "CyMedia\CyMediaParse\CyMediaVideoParse.h" "%GenDir_include%\CyMediaParse"


::Bin->colorMaP
echo "copy ColorMap"
xcopy "CyMedia\CyMediaDis\colorMap_ALL" "%GenDir_Bin%colorMap\" /i/s/y

::Bin/Lib->x64
echo "copy Bin/Lib->x64"
copy "%LIBSrcDir%\x64\Debug\CyMedia.dll" "%GenDir_Bin_64%\Debug\"
copy "%LIBSrcDir%\x64\Debug\CyMedia.lib" "%GenDir_Lib_64%\Debug\"
copy "%LIBSrcDir%\x64\Debug\CyMedia.pdb" "%GenDir_Lib_64%\Debug\"

copy "%LIBSrcDir%\x64\Release\CyMedia.dll" "%GenDir_Bin_64%\Release\"
copy "%LIBSrcDir%\x64\Release\CyMedia.lib" "%GenDir_Lib_64%\Release\"

::Bin/Lib->Win32
echo "copy Bin/Lib->Win32"
copy "%LIBSrcDir%\Win32\Debug\CyMedia.dll" "%GenDir_Bin_32%\Debug\"
copy "%LIBSrcDir%\Win32\Debug\CyMedia.lib" "%GenDir_Lib_32%\Debug\"
copy "%LIBSrcDir%\Win32\Debug\CyMedia.pdb" "%GenDir_Lib_32%\Debug\"

copy "%LIBSrcDir%\Win32\Release\CyMedia.dll" "%GenDir_Bin_32%\Release\"
copy "%LIBSrcDir%\Win32\Release\CyMedia.lib" "%GenDir_Lib_32%\Release\"
::Demo
