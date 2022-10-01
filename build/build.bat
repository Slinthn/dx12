@echo off

where cl
if %errorlevel% neq 0 call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

cls

pushd ..\bin

dxc -nologo -Zi -Qembed_debug -Fo default_vertex.cso -T vs_6_2 -E VertexEntry ..\src\hlsl\default.hlsl
dxc -nologo -Zi -Qembed_debug -Fo default_pixel.cso -T ps_6_2 -E PixelEntry ..\src\hlsl\default.hlsl

dxc -nologo -Zi -Qembed_debug -Fo shader_vertex.cso -T vs_6_2 -E VertexEntry ..\src\hlsl\shader.hlsl
dxc -nologo -Zi -Qembed_debug -Fo shader_pixel.cso -T ps_6_2 -E PixelEntry ..\src\hlsl\shader.hlsl

rc -nologo -fo resources.res ..\src\resources.rc

cl -nologo  ..\src\app.c /link KERNEL32.LIB USER32.LIB DXGUID.LIB D3D12.LIB DXGI.LIB resources.res

popd
