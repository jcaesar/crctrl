VERSION = 0.2
CC      = /usr/bin/g++
CFLAGS  = -Wall -g -D_REENTRANT -DVERSION=\"$(VERSION)\"
LDFLAGS = -I/usr/include/mysql -I/usr/include/mysql++ -lmysqlpp -lboost_regex-mt -lpthread -o crctrl 

OBJ = AutoHost.o Config.o Control.o GameControl.o Lib.o Main.o
main = Main.cpp

cserv: $(main)
	$(CC) $(CFLAGS) -o cserv $(main) $(LDFLAGS)
all: $(OBJ)
	$(CC) $(CFLAGS) -o cserv $(OBJ) $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $<
	
clean:
	rm -r -f *.o
