
Build: main.c
	@if [ ! -d "Build" ]; then \
	echo "Directory doesn't exist, making..."; \
	mkdir Build; \
	fi
	gcc -Iinclude  main.c -o Build/simpleRythmGame -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

Debug: main.c
	@if [ ! -d "Build" ]; then \
	echo "Directory doesn't exist, making..."; \
	mkdir Build; \
	fi
	gcc -ggdb -o Build/Debug main.c -g -Wall -I/opt/raylib/src -L/opt/raylib/release/libs/linux -lraylib -lGL -lm  -lpthread -ldl -lrt -lX11
	#valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose ./Debug

windows: main.c
	x86_64-w64-mingw32-gcc main.c -static -o Build/simpleRythmGame.exe -s -w -Llib -Iinclude -Llib-mingw-w64 -l:libraylibdll.a -l:libraylib.a -lglfw3 -lopengl32 -lgdi32 -lwinmm -Wl,-allow-multiple-definition -Wl,--subsystem,windows
