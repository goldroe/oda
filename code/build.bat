@echo off

SET warning_flags=-W4 -WX -wd4201 -wd4053 -wd4100 -wd4189 -wd4200
PUSHD ..\build

CL -nologo -FC -Zi -MDd %warning_flags% ..\code\oda.c
POPD
