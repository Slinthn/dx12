@echo off

where cl
if %errorlevel% neq 0 call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

mkdir ..\bin
cls

pushd ..\bin

cl -nologo -Wall -Z7 ..\src\win64_app.c /link KERNEL32.LIB USER32.LIB D3D12.LIB DXGUID.LIB DXGI.LIB /ENTRY:MainEntry /SUBSYSTEM:WINDOWS

popd
