# Choose one of the two definitions of X by removing the initial #.
# The first is for the lab, the second for MSYS2.
# If SDL2 has been installed in a standard place on your computer, comment out
# both definitions by putting # in front.
# If your computer has SDL2 installed somewhere else, use the sdl2-config
# command to find out where, and add your own definition of X.

# X = -I/opt/SDL/2.0.3/include -L/opt/SDL/2.0.3/lib -Wl,-rpath,/opt/SDL/2.0.3/lib
# X = -I/mingw64/include -L/mingw64/lib

compile:
	gcc $(X) -std=c99 -O3 -pedantic -Wall -Werror main.c -o main -lSDL2
