all:
	@echo [Building MainPlugin.so]
	clang++ -std=c++23 -g -ldl -Wall -Wextra -fpic -shared -Iinclude -I../../SDK -o MainPlugin.so src/MainPlugin.cpp
	@echo [Building GUIEngine.so]
	clang++ -std=c++23 -g -ldl -lSDL3 -Wall -Wextra -fpic -shared -Iinclude -I../../SDK -o GUIEngine.so src/GUIEngine.cpp
	@echo [Building main]
	clang++ -std=c++23 -g -ldl -Wall -Wextra -Iinclude -I../../SDK -o main src/Main.cpp

