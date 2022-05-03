
Build: main.c
	gcc -w -g0 -I /opt/raylib/src  main.c -o simpleRythmGame -s -w -I/opt/raylib/src -L/opt/raylib/release/libs/linux -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

debug: main.c
	gcc -ggdb -o Debug main.c -g -Wall -I/opt/raylib/src -L/opt/raylib/release/libs/linux -lraylib -lGL -lm  -lpthread -ldl -lrt -lX11
	#valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose ./Debug

windows: main.c
	x86_64-w64-mingw32-gcc main.c -static -o simpleRythmGame.exe -s -w -Llib -Iinclude -Llib-mingw-w64 -l:libraylibdll.a -l:libraylib.a -lglfw3 -lopengl32 -lgdi32 -lwinmm -Wl,-allow-multiple-definition -Wl,--subsystem,windows
