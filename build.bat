@echo off

PUSHD build

CL -nologo -FC -Zi -W4 -WX -wd4201 -wd4053 -wd4100 -wd4189 -wd4200 ..\code\oda.c
POPD