#include "FileSystem.h"
#include <iostream>

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
    double blockSizeKB = std::stod(argv[1]);
    std::string fileName = argv[2];
    
    // Initialize file system
    FileSystem fs(blockSizeKB, fileName);

    fs.saveFileSystem();
    return 0;
}
