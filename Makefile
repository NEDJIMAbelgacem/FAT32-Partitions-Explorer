fat_explorer: ./sources/main.c
	gcc -o ./bin/app.exe ./sources/main.c ./sources/fat_explorer.c -I./headers -Wno-deprecated -D LINUX
