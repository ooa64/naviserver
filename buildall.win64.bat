@echo off

set TCLPATH=c:\naviserver
set TCLLIB=tcl90.lib tclstub.lib
set TCLSH=%TCLPATH%\bin\tclsh90.exe 
set ZLIBSRC=%CD%\..\tcl\compat\zlib
set OPENSSL=c:\openssl3-win64
set INSTALLDIR=c:\naviserver

call "C:\Program Files\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

nmake.exe -nologo -ef makefile.win32 %*
