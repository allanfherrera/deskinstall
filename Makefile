# Makefile for Desktop Environment Installer

# Compiler and flags
CC = gcc
CFLAGS = -O2 -Wall -g `pkg-config --cflags gtk+-3.0`
LDFLAGS = `pkg-config --libs gtk+-3.0`

# Target executable name
TARGET = deskinstall

# Source files
SOURCES = deskinstall.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJECTS)
    $(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Compile source files to object files
%.o: %.c
    $(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
    rm -f $(OBJECTS) $(TARGET)

# Install (optional - installs to /usr/local/bin)
install: $(TARGET)
    install -m 755 $(TARGET) /usr/local/bin/

# Uninstall (optional)
uninstall:
    rm -f /usr/local/bin/$(TARGET)

# Phony targets
.PHONY: all clean install uninstall

# Dependencies
main.o: main.c
