CFLAGS = -I ./include
LFLAGS = -lrt -lX11 -lGLU -lGL -pthread -lm  #-lXrandr

all: egoshadow

egoshadow: egoshadow.cpp log.cpp log.h
	g++ $(CFLAGS) egoshadow.cpp log.cpp  libggfonts.a -Wall -Wextra $(LFLAGS) -oegoshadow

clean:
	rm -f egoshadow
	rm -f *.o

