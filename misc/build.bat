@echo off

where cl
if %errorlevel% neq 0 call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

mkdir ..\bin
cls

pushd ..\bin

dxc -nologo -Zi -Qembed_debug -Fo default_vertex.cso -T vs_6_2 -E VertexEntry ..\src\default.hlsl
dxc -nologo -Zi -Qembed_debug -Fo default_pixel.cso -T ps_6_2 -E PixelEntry ..\src\default.hlsl

rc -nologo -fo resources.res ..\src\resources.rc

cl -nologo -Wall -Z7 -wd5045 -DSLINAPP_DEBUG=1 ..\src\win64_app.c /link KERNEL32.LIB USER32.LIB D3D12.LIB DXGUID.LIB DXGI.LIB WINMM.LIB LIBCMT.LIB LIBVCRUNTIME.LIB LIBUCRT.LIB resources.res /ENTRY:MainEntry /SUBSYSTEM:WINDOWS

popd
