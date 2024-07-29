#!/bin/bash

# Clean and build
make clean
make

# Create a file system
./makeFileSystem 1 fileSystem.data

# Run operations
./fileSystemOper fileSystem.data mkdir "/usr"
./fileSystemOper fileSystem.data mkdir "/usr/ysa"
./fileSystemOper fileSystem.data mkdir "/bin/ysa"                   # Should produce an error
./fileSystemOper fileSystem.data writeDirect "/usr/ysa/lf" "lf content" # Create lf file and write content
./fileSystemOper fileSystem.data write "/usr/ysa/file1" "/usr/ysa/lf"
./fileSystemOper fileSystem.data write "/usr/file2" "/usr/ysa/lf"
./fileSystemOper fileSystem.data write "/file3" "/usr/ysa/lf"
./fileSystemOper fileSystem.data dir "/"
./fileSystemOper fileSystem.data del "/usr/ysa/file1"
./fileSystemOper fileSystem.data dumpe2fs
./fileSystemOper fileSystem.data writeDirect "/usr/lf2" "" # Create lf2 file 
./fileSystemOper fileSystem.data read "/usr/file2" "/usr/lf2"
#cmp /usr/lf2 /usr/ysa/lf
./fileSystemOper fileSystem.data chmod "/usr/file2" "-rw"
./fileSystemOper fileSystem.data dumpe2fs
./fileSystemOper fileSystem.data read "/usr/file2" "/usr/lf2"            # Should produce an error
./fileSystemOper fileSystem.data chmod "/usr/file2" "+rw"
./fileSystemOper fileSystem.data addpw "/usr/file2" "test1234"
./fileSystemOper fileSystem.data read "/usr/file2" "/usr/lf2"            # Should produce an error
./fileSystemOper fileSystem.data read "/usr/file2" "/usr/lf2" "test1234" # Should be OK
./fileSystemOper fileSystem.data test

rm fileSystem.data