@echo off

set CommonCompilerFlags=-nologo -GR- -EHa- -Oi -Od -MT -FC -W4 -WX -wd4100 -Zi

set LibSources=..\json\src\json.cpp
set ExampleSources=..\json\src\example.cpp

set BuildDir=..\..\json-build

IF NOT EXIST %BuildDir% mkdir %BuildDir%
pushd %BuildDir%

cl %CommonCompilerFlags% /DBUILDING_JSON %LibSources% -Fm:json.map /LD

cl %CommonCompilerFlags% %ExampleSources% -Fm:example.map /link -opt:ref ws2_32.lib json.lib

popd
