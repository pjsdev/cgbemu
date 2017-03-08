gcc *.c -o main.o -Ilib/SDL2.framework/Headers -Flib  -framework SDL2 && install_name_tool -change @rpath/SDL2.framework/Versions/A/SDL2 @executable_path/lib/SDL2.framework/Versions/A/SDL2 main.o
