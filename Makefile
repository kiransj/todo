all:
	g++  -I/usr/include/ncurses -L/usr/lib/ sqlite.cpp ui.cpp -lsqlite3 -lncurses
