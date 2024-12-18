CC = g++
CFLAGS = -std=c++17 -O2 -Wall -pthread
LDFLAGS = -lSDL2 -lSDL2_image -lSDL2_ttf

TARGET = game
SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)


all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
	
clean:
	rm -f $(OBJS) $(TARGET)

test: $(TARGET)
	./$(TARGET)
