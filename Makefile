CFLAGS = -I ./include
LFLAGS = -lrt -lX11 -lGLU -lGL -pthread -lm #-lXrandr

all: egoshadow

egoshadow: egoshadow.cpp egoshadow.h log.cpp log.h nchiang.cpp nchiang.h
	g++ $(CFLAGS) egoshadow.cpp log.cpp nchiang.cpp libggfonts.a -Wall -Wextra $(LFLAGS) -oegoshadow

clean:
	rm -f egoshadow
	rm -f *.o

