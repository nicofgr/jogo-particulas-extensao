build: 
	#clang -I./include/ -std=c99 -Wall ./src/*.c -lSDL2 -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lm -o  saida.out
	#clang -I./include/ -std=c99 -Wall -Werror -fsanitize=address ./src/*.c -lSDL2 -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lm -o  saida.out
	clang -I./include/ -std=c99 -Wall -fsanitize=address ./src/*.c -lSDL2 -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lm -lGLESv2 -lEGL -o  saida.out
	#Turn -fsanitize off for release build

profile:
	clang -I./include/ -std=c99 -Wall -fsanitize=address ./src/*.c -lSDL2 -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lm -pg -o saida.out

run:
	./saida.out

clean:
	rm ./saida.out
