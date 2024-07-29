# Compiler
CXX = g++
CXXFLAGS = -std=c++11 -Wall

# Source files
FS_SOURCES = FileSystem.cpp FAT12.cpp DirectoryEntry.cpp
FS_OBJECTS = $(FS_SOURCES:.cpp=.o)

# Executables
MAKE_FILESYSTEM = makeFileSystem
FILE_SYSTEM_OPER = fileSystemOper

# Default target
all: $(MAKE_FILESYSTEM) $(FILE_SYSTEM_OPER)

# makeFileSystem executable
$(MAKE_FILESYSTEM): $(FS_OBJECTS) makeFileSystem.o
	$(CXX) $(CXXFLAGS) -o $@ $^

# fileSystemOper executable
$(FILE_SYSTEM_OPER): $(FS_OBJECTS) fileSystemOper.o
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compiling source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(FS_OBJECTS) makeFileSystem.o fileSystemOper.o $(MAKE_FILESYSTEM) $(FILE_SYSTEM_OPER)

# Phony targets
.PHONY: all clean
