CC = gcc
CFLAGS = $(shell pkg-config --cflags libftdi1) -g
LDFLAGS = $(shell pkg-config --libs libftdi1)

# Name of your executable
TARGET =  bitbangchrono

# Source files
SOURCES =  bitbangchrono.c
OBJECTS = $(SOURCES:.c=.o)

# Default target
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean
