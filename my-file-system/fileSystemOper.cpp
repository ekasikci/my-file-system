#include "FileSystem.h"
#include <iostream>
#include <cstring>

void printUsage()
{
    std::cerr << "Usage: fileSystemOper <fileName> <operation> <parameters>\n";
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printUsage();
        return 1;
    }

    std::string fileName = argv[1];
    std::string operation = argv[2];

    FileSystem fs(1, fileName); // Block size doesn't matter here since we're loading an existing file system

    if (operation == "dir")
    {
        if (argc != 4)
        {
            printUsage();
            return 1;
        }
        std::string path = argv[3];
        fs.listDirectory(path);
    }
    else if (operation == "mkdir")
    {
        if (argc != 4)
        {
            printUsage();
            return 1;
        }
        std::string path = argv[3];
        fs.makeDirectory(path);
    }
    else if (operation == "rmdir")
    {
        if (argc != 4)
        {
            printUsage();
            return 1;
        }
        std::string path = argv[3];
        fs.removeDirectory(path);
    }
    else if (operation == "dumpe2fs")
    {
        fs.dumpe2fs();
    }
    else if (operation == "writeDirect")
    {
        if (argc != 5)
        {
            printUsage();
            return 1;
        }
        std::string path = argv[3];
        std::string content = argv[4];
        fs.writeFile(path, content);
    }
    else if (operation == "write")
    {
        if (argc != 5)
        {
            printUsage();
            return 1;
        }
        std::string path = argv[3];
        std::string linuxFile = argv[4];
        fs.writeFileToFile(path, linuxFile);
    }
    else if (operation == "read")
    {
        if (argc < 5 || argc > 6)
        {
            printUsage();
            return 1;
        }
        std::string path = argv[3];
        std::string linuxFile = argv[4];
        std::string password = argc == 6 ? argv[5] : "";

        fs.readFile(path, linuxFile, password);
    }
    else if (operation == "del")
    {
        if (argc != 4)
        {
            printUsage();
            return 1;
        }
        std::string path = argv[3];
        fs.deleteFile(path);
    }
    else if (operation == "chmod")
    {
        if (argc != 5)
        {
            printUsage();
            return 1;
        }
        std::string path = argv[3];
        std::string permissions = argv[4];
        fs.changeMode(path, permissions);
    }
    else if (operation == "addpw")
    {
        if (argc != 5)
        {
            printUsage();
            return 1;
        }
        std::string path = argv[3];
        std::string password = argv[4];
        fs.addPassword(path, password);
    }
    else if (operation == "test")
    {
        fs.printFileSystem();
        fs.printDirectoryPages();
        fs.printBlockContents();
    }
    else
    {
        printUsage();
        return 1;
    }
    
    fs.saveFileSystem();
    return 0;
}
