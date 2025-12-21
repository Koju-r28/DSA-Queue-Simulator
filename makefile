all:
	g++ -o bin/generator src/generator.c src/traffic_simulation.c -lSDL2 -Iinclude -Llib -lmingw32 -lSDL2main -lSDL2

	g++ -Iinclude -Llib -o bin/main.exe src/main.c src/traffic_simulation.c -lmingw32 -lSDL2main -lSDL2

generator: 
	g++ -o bin/generator src/generator.c src/traffic_simulation.c -lSDL2 -Iinclude -Llib -lmingw32 -lSDL2main -lSDL2

main:
	g++ -Iinclude -Llib -o bin/main.exe src/main.c src/traffic_simulation.c -lmingw32 -lSDL2main -lSDL2

clean:
	rm bin/generator.exe
	rm bin/main.exe
	rm bin/vehicles.txt