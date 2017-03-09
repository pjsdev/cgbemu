call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64
call cl.exe src\*.c /Forun_tree\obj\ /Ferun_tree\main.exe /Iinclude\ /Iinput\include\ /link input\SDL2.lib
