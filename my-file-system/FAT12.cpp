#include "FAT12.h"
#include <fstream>
#include <cstring>

FAT12::FAT12(double blockSizeKB, const std::string &fileName) : fileName(fileName), blockSize(static_cast<double>(blockSizeKB * 1024))
{
    std::memset(FAT, 0, sizeof(FAT));
}

void FAT12::initializeFileSystem()
{
    std::ofstream file(fileName, std::ios::binary | std::ios::out);
    if (file.is_open())
    {
        // Initialize file to the correct size
        file.seekp(totalBlocks * blockSize - 1);
        file.write("", 1);
        file.close();
    }

    for (int i = 0; i < totalBlocks; ++i)
    {
        FAT[i].isBusy = false;
        FAT[i].nextBlock = -1;
    }

    // Allocate the root directory block
    allocateBlock();
}

void FAT12::printFAT() const
{
    std::cout << "FAT Table (Occupied Blocks Only):\n";
    for (int i = 0; i < totalBlocks; ++i)
    {
        if (FAT[i].isBusy)
        {
            std::cout << "Block " << i << ": isBusy = " << FAT[i].isBusy << ", nextBlock = " << FAT[i].nextBlock << "\n";
        }
    }
}

const std::string &FAT12::getFileName() const
{
    return fileName;
}

int FAT12::getTotalBlocks() const
{
    return totalBlocks;
}
int FAT12::getFATEntrySize() const
{
    return sizeof(FATEntry);
}

int FAT12::allocateBlock()
{
    for (int i = 0; i < totalBlocks; ++i)
    {
        if (!FAT[i].isBusy)
        {
            FAT[i].isBusy = true;
            FAT[i].nextBlock = -1;
            return i;
        }
    }
    return -1; // No free blocks available
}

void FAT12::freeBlock(int block)
{
    if (block >= 0 && block < totalBlocks)
    {
        FAT[block].isBusy = false;
        FAT[block].nextBlock = -1;
    }
}

void FAT12::setNextBlock(int block, int nextBlock)
{
    if (block >= 0 && block < totalBlocks)
    {
        FAT[block].nextBlock = nextBlock;
    }
}

int FAT12::getNextBlock(int block) const
{
    if (block >= 0 && block < totalBlocks)
    {
        return FAT[block].nextBlock;
    }
    return -1;
}

bool FAT12::isBlockBusy(int block) const
{
    if (block >= 0 && block < totalBlocks)
    {
        return FAT[block].isBusy;
    }
    return false;
}

void FAT12::setBlockSize(double newBlockSize)
{
    blockSize = newBlockSize;
}

double FAT12::getBlockSize() const
{
    return blockSize;
}
