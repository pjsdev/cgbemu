gcc src/*.c -o run_tree/cgbemu -Iinclude -Irun_tree/SDL2.framework/Headers -Frun_tree -framework SDL2 && install_name_tool -change @rpath/SDL2.framework/Versions/A/SDL2 @executable_path/SDL2.framework/Versions/A/SDL2 run_tree/cgbemu
