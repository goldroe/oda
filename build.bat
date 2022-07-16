@echo off

PUSHD build

CL -nologo -FC -Zi -W4 -WX -wd4100 -wd4189 ..\code\oda.c
POPD