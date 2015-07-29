COMPILER=gcc
CFLAGS=-Wall
all:echotcp
echotcp: server
server:  
	$(COMPILER) -o server ServerMain.c $(CFLAGS)

clean:
	rm *.o server



