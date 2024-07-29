#ifndef FAT12_H
#define FAT12_H

#include <string>
#include <iostream>

class FAT12
{
public:
    FAT12(double blockSizeKB, const std::string &fileName);
    void initializeFileSystem();
    void printFAT() const;
    const std::string &getFileName() const;
    int getTotalBlocks() const;
    double getBlockSize() const;
    int getFATEntrySize() const;
    int allocateBlock();
    void freeBlock(int block);
    void setNextBlock(int block, int nextBlock);
    int getNextBlock(int block) const;
    bool isBlockBusy(int block) const;
    void setBlockSize(double blockSize);
    std::string fileName;
    double blockSize;
    static const int totalBlocks = 4096;
    struct FATEntry
    {
        bool isBusy;
        int nextBlock;
    };
    FATEntry FAT[totalBlocks];
};

#endif // FAT12_H
