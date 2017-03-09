An experimental Game Boy emulator built in C.
The only input to this project is SDL.

Build
======

To build on windows, you may need to tweak the .bat file and add the following:

    ./run_tree/SDL2.dll
    ./run_tree/obj/
    ./run_tree/data/
    ./input/include/
    ./input/SDL2.lib

To build on osx you may need to tweak the build_mac.sh file and add the following:

    ./run_tree/SDL2.framework/
    ./run_tree/data/

