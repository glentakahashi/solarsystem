TARGET   = glutharness

CC       = g++
CFLAGS   = -c -g -DLINUX
LDFLAGS  = -lGL -lGLU -lglut -lGLEW
SRC      = $(wildcard *.cpp)
OBJ      = $(SRC:.cpp=.o)

#for some reason, using make all would not link in main.o into my program... had to write a fix.
fix: 
	g++ -c -g -DDEBUG -DLINUX InitShader.cpp -o InitShader.o
	g++ -c -g -DDEBUG -DLINUX main.cpp -o main.o
	g++ -lGL -lGLU -lglut -lGLEW main.o InitShader.o -o glutharness

all: $(TARGET)
	
$(TARGET): $(OBJ) 
	$(CC) $(LDFLAGS) $< -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o
	rm -f $(TARGET)
