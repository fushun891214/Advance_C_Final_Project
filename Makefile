
# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Werror -g

# Source files
SRCS = main.c space.c commands.c

# Header files
HEADERS = space.h commands.h

# Output executable
TARGET = command

# Default rule
all: $(TARGET)

# Rule to link object files and create the executable
$(TARGET): $(SRCS) $(HEADERS)
	$(CC)  -o $(TARGET) $(SRCS)

# Clean rule to remove the executable
clean:
	@if [ -f $(TARGET) ]; then rm -f $(TARGET); fi
	@if [ -f $(hello*.c) ]; then rm -f $(hello*.c); fi
	@if [ -d test ]; then rm -r test; fi
