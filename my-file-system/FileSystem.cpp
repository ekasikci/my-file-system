#include "FileSystem.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <bitset>

FileSystem::FileSystem(double blockSizeKB, const std::string &fileName) : fat(blockSizeKB, fileName)
{
    if (filesystemExists(fileName))
    {
        loadFileSystem(fileName);
    }
    else
    {
        initializeFileSystem();
    }
}

void FileSystem::initializeFileSystem()
{
    if (filesystemExists(fat.getFileName()))
    {
        loadDirectoryEntries();
    }
    else
    {
        fat.initializeFileSystem();
        directoryEntries.clear();

        // Add the root directory
        int rootBlock = fat.allocateBlock();
        DirectoryEntry root("/", rootBlock, 0, 0x13); // 0x10: directory attribute
        root.updateModificationTime();
        directoryEntries.push_back(root);

        // Write the root directory to the file
        writeDirectoryEntryToPage(root.getFirstBlock(), root);
    }
}

void FileSystem::listDirectory() const
{
    for (const auto &entry : directoryEntries)
    {
        std::cout << "File Name: " << entry.getFileName()
                  << ", Size: " << entry.getSize()
                  << ", First Block: " << entry.getFirstBlock()
                  << ", Attributes: " << getAttributesString(entry.getAttributes())
                  << ", Password: " << (entry.getPassword().empty() ? "No" : entry.getPassword())
                  << ", Last Modified: " << entry.getFormattedDate() << " " << entry.getFormattedTime() << "\n";
    }
}

void FileSystem::printDirectoryPages()
{
    std::cout << "Directory Pages:\n";
    for (const auto &entry : directoryEntries)
    {
        if ((entry.getAttributes() & 0x10) == 0x10)
        {
            std::cout << "Directory: " << entry.getFileName() << " at block " << entry.getFirstBlock() << "\n";

            int block = entry.getFirstBlock();
            while (block != -1)
            {
                std::ifstream file(fat.getFileName(), std::ios::binary);
                if (!file.is_open())
                {
                    std::cerr << "Failed to open file system.\n";
                    return;
                }

                file.seekg(block * fat.getBlockSize(), std::ios::beg);
                for (size_t i = 0; i < fat.getBlockSize() / sizeof(DirectoryEntry); ++i)
                {
                    DirectoryEntry dirEntry;
                    file.read(reinterpret_cast<char *>(&dirEntry), sizeof(DirectoryEntry));
                    if (!dirEntry.getFileName().empty())
                    {
                        std::cout << "File Name: " << dirEntry.getFileName()
                                  << ", Size: " << dirEntry.getSize()
                                  << ", First Block: " << dirEntry.getFirstBlock()
                                  << ", Attributes: " << getAttributesString(dirEntry.getAttributes())
                                  << ", Password: " << (dirEntry.getPassword().empty() ? "No" : dirEntry.getPassword())
                                  << ", Last Modified: " << dirEntry.getFormattedDate() << " " << dirEntry.getFormattedTime() << "\n";
                    }
                }
                block = fat.getNextBlock(block);
                file.close();
            }
        }
    }
}

void FileSystem::printFileSystem() const
{
    fat.printFAT();
    listDirectory();
}

void FileSystem::makeDirectory(const std::string &dirName)
{
    std::string parentName = getParentDirectoryName(dirName);
    std::string splittedDirName = getDirectoryName(dirName);

    DirectoryEntry *parentDir = findDirectoryEntry(parentName);
    if (!parentDir)
    {
        std::cerr << "Parent directory " << parentName << " does not exist.\n";
        return;
    }

    int block = fat.allocateBlock();
    if (block == -1)
    {
        std::cerr << "No space left to allocate new directory.\n";
        return;
    }

    DirectoryEntry newDir(splittedDirName, block, 0, 0x13); // Set as directory
    newDir.updateModificationTime();
    directoryEntries.push_back(newDir);

    writeDirectoryEntryToPage(newDir.getFirstBlock(), newDir);
    writeDirectoryEntryToPage(newDir.getFirstBlock(), *findDirectoryEntry(parentName));
    writeDirectoryEntryToPage(findDirectoryEntry(parentName)->getFirstBlock(), newDir);
}

void FileSystem::removeDirectory(const std::string &dirName)
{
    std::cout << "Removing directory: " << dirName << "\n";
    auto it = std::find_if(directoryEntries.begin(), directoryEntries.end(),
                           [&](const DirectoryEntry &entry)
                           { return entry.getFileName() == dirName && ((entry.getAttributes() & 0x10) || (entry.getAttributes() & 0x11) || (entry.getAttributes() & 0x12) || (entry.getAttributes() & 0x13)); });

    if (it == directoryEntries.end())
    {
        std::cerr << "Directory not found.\n";
        return;
    }

    int dirBlock = it->getFirstBlock();
    bool isEmpty = true;

    for (const auto &entry : directoryEntries)
    {
        if (entry.getFirstBlock() != dirBlock && entry.getFileName() != dirName &&
            fat.getNextBlock(entry.getFirstBlock()) == dirBlock)
        {
            isEmpty = false;
            break;
        }
    }

    if (!isEmpty)
    {
        std::cerr << "Directory is not empty.\n";
        return;
    }

    int block = it->getFirstBlock();
    do
    {
        int nextBlock = fat.getNextBlock(block);
        fat.freeBlock(block);
        block = nextBlock;
    } while (block != -1);

    directoryEntries.erase(it);
}

// Function to check if a file exists in the directory entry
bool FileSystem::fileExistinDirectoryEntry(DirectoryEntry *parent, const std::string &path)
{
    int block = parent->getFirstBlock();
    while (block != -1)
    {
        std::ifstream file(fat.getFileName(), std::ios::binary);
        if (!file.is_open())
        {
            std::cerr << "Failed to open file system.\n";
            return false;
        }

        file.seekg(block * fat.getBlockSize(), std::ios::beg);
        DirectoryEntry dirEntry;
        while (file.read(reinterpret_cast<char *>(&dirEntry), sizeof(dirEntry)))
        {
            if (dirEntry.getFileName() == path)
            {
                return true;
            }
        }
        block = fat.getNextBlock(block);
    }
    return false;
}

void FileSystem::writeFile(const std::string &fileName, const std::string &content)
{
    std::string parentName = getParentDirectoryName(fileName);
    std::string shortFileName = getDirectoryName(fileName);

    auto it = std::find_if(directoryEntries.begin(), directoryEntries.end(),
                           [&](const DirectoryEntry &entry)
                           { return entry.getFileName() == parentName && ((entry.getAttributes() & 0x10) || (entry.getAttributes() & 0x11) || (entry.getAttributes() & 0x12) || (entry.getAttributes() & 0x13)); });

    if (it == directoryEntries.end())
    {
        std::cerr << "Parent directory not found.\n";
        return;
    }
    // If the file exists, check write permission
    if (!(it->getAttributes() & 0x02)) // Check if the file has write permission
    {
        std::cerr << "Write permission denied.\n";
        return;
    }
    DirectoryEntry *parentDir = &(*it);

    // Check if the file already exists
    if (fileExistinDirectoryEntry(parentDir, shortFileName))
    {
        std::cerr << "File already exists.\n";
        return;
    }

    int block = fat.allocateBlock();
    if (block == -1)
    {
        std::cerr << "No space left to allocate new file.\n";
        return;
    }

    DirectoryEntry newFile(shortFileName, block, content.size(), 0x23); // Set as file
    newFile.updateModificationTime();
    directoryEntries.push_back(newFile);

    // Write the new file entry to the parent directory's block
    writeDirectoryEntryToPage(parentDir->getFirstBlock(), newFile);

    // Write the content to the allocated blocks
    std::string::size_type contentIndex = 0;
    int currentBlock = block;
    while (contentIndex < content.size())
    {
        std::ofstream file(fat.getFileName(), std::ios::binary | std::ios::in | std::ios::out);
        if (!file.is_open())
        {
            std::cerr << "Failed to open filesystem file.\n";
            return;
        }

        file.seekp(currentBlock * fat.getBlockSize(), std::ios::beg);
        std::string::size_type bytesToWrite = std::min(static_cast<std::string::size_type>(fat.getBlockSize()), content.size() - contentIndex);
        file.write(content.c_str() + contentIndex, bytesToWrite);
        contentIndex += bytesToWrite;
        file.close();

        if (contentIndex < content.size())
        {
            int nextBlock = fat.allocateBlock();
            if (nextBlock == -1)
            {
                std::cerr << "No space left to allocate new block.\n";
                return;
            }

            fat.setNextBlock(currentBlock, nextBlock);
            currentBlock = nextBlock;
        }
        else
        {
            fat.setNextBlock(currentBlock, -1);
        }
    }
    newFile.updateModificationTime();
}

void FileSystem::listDirectory(const std::string &path) const
{
    std::cout << "Listing contents of directory: " << path << "\n";

    auto dirEntryIt = std::find_if(directoryEntries.begin(), directoryEntries.end(),
                                   [&](const DirectoryEntry &entry)
                                   {
                                       return entry.getFileName() == path && ((entry.getAttributes() & 0x10) || (entry.getAttributes() & 0x11) || (entry.getAttributes() & 0x12) || (entry.getAttributes() & 0x13));
                                   });

    if (dirEntryIt == directoryEntries.end())
    {
        std::cerr << "Directory not found.\n";
        return;
    }

    int block = dirEntryIt->getFirstBlock();

    while (block != -1)
    {
        std::ifstream file(fat.getFileName(), std::ios::binary);
        if (!file.is_open())
        {
            std::cerr << "Failed to open file system.\n";
            return;
        }

        file.seekg(block * fat.getBlockSize(), std::ios::beg);

        for (size_t i = 0; i < fat.getBlockSize() / sizeof(DirectoryEntry); ++i)
        {
            DirectoryEntry dirEntry;
            file.read(reinterpret_cast<char *>(&dirEntry), sizeof(DirectoryEntry));

            if (std::strlen(dirEntry.getFileName().c_str()) > 0)
            {
                std::cout << "    File Name: " << dirEntry.getFileName()
                          << ", Size: " << dirEntry.getSize()
                          << ", First Block: " << dirEntry.getFirstBlock()
                          << ", Attributes: " << getAttributesString(dirEntry.getAttributes())
                          << ", Password: " << (dirEntry.getPassword().empty() ? "No" : dirEntry.getPassword())
                          << ", Last Modified: " << dirEntry.getFormattedDate() << " " << dirEntry.getFormattedTime()
                          << "\n";
            }
        }

        block = fat.getNextBlock(block);
    }
}

void FileSystem::deleteFile(const std::string &fileName)
{
    std::string parentName = getParentDirectoryName(fileName);
    std::string shortFileName = getDirectoryName(fileName);

    // Find the parent directory entry
    DirectoryEntry *parentDir = findDirectoryEntry(parentName);
    if (!parentDir)
    {
        std::cerr << "Parent directory not found.\n";
        return;
    }
    // Remove the file entry from the parent directory's block
    int parentBlock = parentDir->getFirstBlock();
    bool fileFound = false;
    while (parentBlock != -1)
    {
        std::fstream file(fat.getFileName(), std::ios::binary | std::ios::in | std::ios::out);
        if (!file.is_open())
        {
            std::cerr << "Failed to open file system.\n";
            return;
        }

        file.seekg(parentBlock * fat.getBlockSize(), std::ios::beg);

        for (size_t i = 0; i < fat.getBlockSize() / sizeof(DirectoryEntry); ++i)
        {
            DirectoryEntry dirEntry;
            file.read(reinterpret_cast<char *>(&dirEntry), sizeof(DirectoryEntry));

            if (dirEntry.getFileName() == shortFileName)
            {
                // Clear the entry
                dirEntry = DirectoryEntry(); // Value-initialize the DirectoryEntry object
                file.seekp(parentBlock * fat.getBlockSize() + i * sizeof(DirectoryEntry), std::ios::beg);
                file.write(reinterpret_cast<const char *>(&dirEntry), sizeof(DirectoryEntry));
                fileFound = true;
                break;
            }
        }

        if (fileFound)
            break;

        parentBlock = fat.getNextBlock(parentBlock);
    }

    if (!fileFound)
    {
        std::cerr << "File not found in parent directory.\n";
        return;
    }

    auto it = directoryEntries.begin();

    for (; it != directoryEntries.end(); ++it)
    {
        if (it->getFileName() == shortFileName)
        {
            int block = it->getFirstBlock();
            do
            {
                int nextBlock = fat.getNextBlock(block);

                // Clear the content of the block
                std::fstream file(fat.getFileName(), std::ios::binary | std::ios::in | std::ios::out);
                if (file.is_open())
                {
                    file.seekp(block * fat.getBlockSize(), std::ios::beg);
                    std::vector<char> emptyBlock(static_cast<int>(fat.getBlockSize()), 0);
                    file.write(emptyBlock.data(), emptyBlock.size());
                    file.close();
                }

                fat.freeBlock(block);
                block = nextBlock;
            } while (block != -1);

            directoryEntries.erase(it);
            break;
        }
    }
}

void FileSystem::readFile(const std::string &fileName, const std::string &outputFile, const std::string &password)
{
    std::string shortFileName = getDirectoryName(fileName);
    std::string targetShortFileName = getDirectoryName(outputFile);

    DirectoryEntry *entry = findDirectoryEntry(shortFileName);
    if (!entry)
    {
        std::cerr << "File not found.\n";
        return;
    }

    if (!(entry->getAttributes() & 0x01)) // Check if the file has read permission
    {
        std::cerr << "Read permission denied.\n";
        return;
    }

    if (shortFileName.empty())
    {
        deleteFile(outputFile);
        writeFileToFile(outputFile, fileName);
    }
    else
    {
        if (strcmp(password.c_str(), findDirectoryEntry(shortFileName)->getPassword().c_str()) == 0)
        {
            if (!findDirectoryEntry(shortFileName)->getPassword().empty())
                std::cout << "Password is correct.\n";
            deleteFile(outputFile);
            writeFileToFile(outputFile, fileName);
        }
        else
        {
            std::cerr << "Password is incorrect.\n";
        }
    }
}

void FileSystem::changeMode(const std::string &fileName, const std::string &permissions)
{
    std::string shortFileName = getDirectoryName(fileName);

    auto it = directoryEntries.begin();

    for (; it != directoryEntries.end(); ++it)
    {
        if (it->getFileName() == shortFileName)
        {
            break;
        }
    }

    if (it == directoryEntries.end())
    {
        std::cerr << "File not found.\n";
        return;
    }

    char newAttributes = it->getAttributes();
    if (permissions == "+rw")
    {
        newAttributes |= 0x03; // Add read and write permissions
    }
    else if (permissions == "+r")
    {
        newAttributes |= 0x01; // Add read permission
    }
    else if (permissions == "+w")
    {
        newAttributes |= 0x02; // Add write permission
    }
    else if (permissions == "-rw")
    {
        newAttributes &= ~0x03; // Remove read and write permissions
    }
    else if (permissions == "-r")
    {
        newAttributes &= ~0x01; // Remove read permission
    }
    else if (permissions == "-w")
    {
        newAttributes &= ~0x02; // Remove write permission
    }

    // Retrieve the content of the file
    int block = it->getFirstBlock();
    std::string content;
    while (block != -1)
    {
        std::ifstream file(fat.getFileName(), std::ios::binary);
        if (!file.is_open())
        {
            std::cerr << "Failed to open filesystem file.\n";
            return;
        }

        file.seekg(block * static_cast<int>(fat.getBlockSize()), std::ios::beg);
        std::vector<char> buffer(static_cast<int>(fat.getBlockSize()));
        file.read(buffer.data(), buffer.size());
        content.append(buffer.data(), file.gcount());
        block = fat.getNextBlock(block);
    }

    // Determine the actual size by ignoring trailing null characters
    std::string trimmedContent;
    for (char c : content)
    {
        if (c == '\0')
        {
            break;
        }
        trimmedContent += c;
    }

    // Delete the original file
    deleteFile(fileName);

    // Create a new file with the same content and password
    writeFileWithAttribute(fileName, trimmedContent, newAttributes);
    std::cout << "Permissions changed.\n";
}

std::string FileSystem::getAttributesString(char attributes) const
{
    std::string result = "--"; // Initialize with no permissions
    if (attributes & 0x01)
        result[0] = 'r'; // Read permission
    if (attributes & 0x02)
        result[1] = 'w'; // Write permission
    return result;
}

void FileSystem::addPassword(const std::string &fileName, const std::string &password)
{
    std::string shortFileName = getDirectoryName(fileName);

    auto it = directoryEntries.begin();

    for (; it != directoryEntries.end(); ++it)
    {
        if (it->getFileName() == shortFileName)
        {
            break;
        }
    }

    if (it == directoryEntries.end())
    {
        std::cerr << "File not found.\n";
        return;
    }

    // Retrieve the content of the file
    int block = it->getFirstBlock();
    std::string content;
    while (block != -1)
    {
        std::ifstream file(fat.getFileName(), std::ios::binary);
        if (!file.is_open())
        {
            std::cerr << "Failed to open filesystem file.\n";
            return;
        }

        file.seekg(block * static_cast<int>(fat.getBlockSize()), std::ios::beg);
        std::vector<char> buffer(static_cast<int>(fat.getBlockSize()));
        file.read(buffer.data(), buffer.size());
        content.append(buffer.data(), file.gcount());
        block = fat.getNextBlock(block);
    }

    // Determine the actual size by ignoring trailing null characters
    std::string trimmedContent;
    for (char c : content)
    {
        if (c == '\0')
        {
            break;
        }
        trimmedContent += c;
    }

    // Delete the original file
    deleteFile(fileName);

    // Create a new file with the same content and password
    writeFileWithPassword(fileName, trimmedContent, password);

    std::cout << "Password added.\n";
}

void FileSystem::dumpe2fs()
{
    int freeBlocks = 0;
    int occupiedBlocks = 0;
    int numberOfFiles = 0;
    int numberOfDirectories = 0;

    for (int i = 0; i < fat.getTotalBlocks(); ++i)
    {
        if (!fat.isBlockBusy(i))
        {
            ++freeBlocks;
        }
        else
        {
            ++occupiedBlocks;
        }
    }

    for (const auto &entry : directoryEntries)
    {
        if ((entry.getAttributes() & 0x10))
        {
            ++numberOfDirectories;
        }
        else
        {
            ++numberOfFiles;
        }
    }

    std::cout << "Filesystem Summary:\n";
    std::cout << "Block count: " << fat.getTotalBlocks() << "\n";
    std::cout << "Block size: " << fat.getBlockSize() << " bytes\n";
    std::cout << "Free blocks: " << freeBlocks << "\n";
    std::cout << "Occupied blocks: " << occupiedBlocks << "\n";
    std::cout << "Number of files: " << numberOfFiles << "\n";
    std::cout << "Number of directories: " << numberOfDirectories << "\n";
    std::cout << "Occupied blocks and file names:\n";

    for (const auto &entry : directoryEntries)
    {
        if ((entry.getAttributes() & 0x10) == 0x10)
        {
            std::cout << "Directory: " << entry.getFileName() << " at block " << entry.getFirstBlock()
                          << ", Size: " << entry.getSize()
                          << ", First Block: " << entry.getFirstBlock()
                          << ", Attributes: " << getAttributesString(entry.getAttributes())
                          << ", Password: " << (entry.getPassword().empty() ? "No" : entry.getPassword())
                          << ", Last Modified: " << entry.getFormattedDate() << " " << entry.getFormattedTime()
                          << "\n";
            
        }
        else
        {
            std::cout << "File: " << entry.getFileName() << " at blocks: "  << entry.getFirstBlock()
                          << ", Size: " << entry.getSize()
                          << ", First Block: " << entry.getFirstBlock()
                          << ", Attributes: " << getAttributesString(entry.getAttributes())
                          << ", Password: " << (entry.getPassword().empty() ? "No" : entry.getPassword())
                          << ", Last Modified: " << entry.getFormattedDate() << " " << entry.getFormattedTime()
                          << "\n";
            int block = entry.getFirstBlock();
            while (block != -1)
            {
                block = fat.getNextBlock(block);
            }
        }
    }
}

bool FileSystem::filesystemExists(const std::string &fileName)
{
    std::ifstream file(fileName, std::ios::binary);
    return file.good();
}

void FileSystem::loadDirectoryEntries()
{
    std::ifstream file(fat.getFileName(), std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file system.\n";
        return;
    }

    directoryEntries.clear();
    for (int i = 0; i < fat.getTotalBlocks(); ++i)
    {
        if (fat.isBlockBusy(i))
        {
            file.seekg(i * fat.getBlockSize(), std::ios::beg);
            while (file.tellg() < (i + 1) * fat.getBlockSize())
            {
                DirectoryEntry entry;
                file.read(reinterpret_cast<char *>(&entry), sizeof(entry));
                if (std::strlen(entry.getFileName().c_str()) > 0)
                {
                    directoryEntries.push_back(entry);
                }
            }
        }
    }

    file.close();
}

void FileSystem::writeDirectoryEntryToPage(int block, const DirectoryEntry &dirEntry)
{
    std::fstream file(fat.getFileName(), std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file system.\n";
        return;
    }

    file.seekp(block * fat.getBlockSize(), std::ios::beg);

    // Iterate over the block and find the first empty entry
    bool entryWritten = false;
    for (size_t i = 0; i < fat.getBlockSize() / sizeof(DirectoryEntry); ++i)
    {
        DirectoryEntry tempEntry;
        file.read(reinterpret_cast<char *>(&tempEntry), sizeof(DirectoryEntry));
        if (tempEntry.getFileName()[0] == '\0')
        {
            file.seekp(block * fat.getBlockSize() + i * sizeof(DirectoryEntry), std::ios::beg);
            file.write(reinterpret_cast<const char *>(&dirEntry), sizeof(DirectoryEntry));
            entryWritten = true;
            break;
        }
    }

    if (!entryWritten)
    {
        std::cerr << "No space left in directory block to write new entry.\n";
    }

    file.close();
}

std::string FileSystem::getParentDirectoryName(const std::string &path)
{
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string item;

    while (std::getline(ss, item, '/'))
    {
        if (!item.empty())
        {
            parts.push_back(item);
        }
    }

    if (parts.size() < 2)
    {
        return "/"; // No parent directory exists
    }

    return parts[parts.size() - 2];
}
std::vector<std::string> FileSystem::splitPath(const std::string &path)
{
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string item;

    while (std::getline(ss, item, '/'))
    {
        if (!item.empty())
        {
            parts.push_back(item);
        }
    }
    return parts;
}
DirectoryEntry *FileSystem::findDirectoryEntry(const std::string &path)
{
    for (auto &entry : directoryEntries)
    {
        if (entry.getFileName() == path)
        {
            return &entry;
        }
    }
    return nullptr;
}

DirectoryEntry *FileSystem::findDirectoryEntryByBlock(int block)
{
    for (auto &entry : directoryEntries)
    {
        if (entry.getFirstBlock() == block)
        {
            return &entry;
        }
    }
    return nullptr;
}

std::string FileSystem::getDirectoryName(const std::string &path)
{
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string item;

    while (std::getline(ss, item, '/'))
    {
        if (!item.empty())
        {
            parts.push_back(item);
        }
    }

    if (parts.empty())
    {
        return ""; // No directory exists
    }

    return parts.back();
}
void FileSystem::printBlockContents() const
{
    std::ifstream file(fat.getFileName(), std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file system.\n";
        return;
    }

    std::cout << "Block Contents:\n";
    for (int i = 1; i < fat.getTotalBlocks(); ++i)
    {
        if (fat.isBlockBusy(i))
        {
            std::cout << "Block " << i << ":\n";
            file.seekg(i * static_cast<int>(fat.getBlockSize()), std::ios::beg);
            std::vector<char> buffer(static_cast<int>(fat.getBlockSize()));
            file.read(buffer.data(), buffer.size());

            // Print the content of the block as is
            for (char c : buffer)
            {
                std::cout << c;
            }
            std::cout << "\n";
        }
    }

    file.close();
}

void FileSystem::writeFileToFile(const std::string &fileName, const std::string &targetFile)
{
    std::string parentName = getParentDirectoryName(fileName);
    std::string shortFileName = getDirectoryName(fileName);
    std::string targetShortFileName = getDirectoryName(targetFile);

    // Find the source file entry
    auto sourceIt = std::find_if(directoryEntries.begin(), directoryEntries.end(),
                                 [&](const DirectoryEntry &entry)
                                 { return entry.getFileName() == targetShortFileName; });

    if (sourceIt == directoryEntries.end())
    {
        std::cerr << "Source file not found.\n";
        return;
    }

    // If the file exists, check write permission
    if (!(sourceIt->getAttributes() & 0x02)) // Check if the file has write permission
    {
        std::cerr << "Write permission denied.\n";
        return;
    }

    // Read the content from the source file
    int sourceBlock = sourceIt->getFirstBlock();
    std::string content;
    while (sourceBlock != -1)
    {
        std::ifstream file(fat.getFileName(), std::ios::binary);
        if (!file.is_open())
        {
            std::cerr << "Failed to open filesystem file.\n";
            return;
        }

        file.seekg(sourceBlock * static_cast<int>(fat.getBlockSize()), std::ios::beg);
        std::vector<char> buffer(static_cast<int>(fat.getBlockSize()));
        file.read(buffer.data(), buffer.size());
        content.append(buffer.data(), file.gcount());
        sourceBlock = fat.getNextBlock(sourceBlock);
    }

    // Determine the actual size by ignoring trailing null characters
    std::string trimmedContent;
    for (char c : content)
    {
        if (c == '\0')
        {
            break;
        }
        trimmedContent += c;
    }

    size_t fileSize = trimmedContent.size();

    // Check if the new file already exists
    auto targetIt = std::find_if(directoryEntries.begin(), directoryEntries.end(),
                                 [&](const DirectoryEntry &entry)
                                 { return entry.getFileName() == shortFileName; });

    if (targetIt != directoryEntries.end())
    {
        std::cerr << "File already exists.\n";
        return;
    }

    // Create a new file with the content
    int block = fat.allocateBlock();
    if (block == -1)
    {
        std::cerr << "No space left to allocate new file.\n";
        return;
    }

    DirectoryEntry newFile(shortFileName, block, fileSize, 0x23); // Set as file
    newFile.updateModificationTime();
    directoryEntries.push_back(newFile);

    // Write the new file entry to the parent directory's block
    writeDirectoryEntryToPage(findDirectoryEntry(parentName)->getFirstBlock(), newFile);

    // Write the content to the allocated blocks
    size_t contentIndex = 0;
    int currentBlock = block;
    while (contentIndex < fileSize)
    {
        std::ofstream file(fat.getFileName(), std::ios::binary | std::ios::in | std::ios::out);
        if (!file.is_open())
        {
            std::cerr << "Failed to open filesystem file.\n";
            return;
        }

        file.seekp(currentBlock * fat.getBlockSize(), std::ios::beg);
        size_t bytesToWrite = std::min(static_cast<size_t>(fat.getBlockSize()), fileSize - contentIndex);
        file.write(trimmedContent.data() + contentIndex, bytesToWrite);
        contentIndex += bytesToWrite;
        file.close();

        if (contentIndex < fileSize)
        {
            int nextBlock = fat.allocateBlock();
            if (nextBlock == -1)
            {
                std::cerr << "No space left to allocate new block.\n";
                return;
            }

            fat.setNextBlock(currentBlock, nextBlock);
            currentBlock = nextBlock;
        }
        else
        {
            fat.setNextBlock(currentBlock, -1);
        }
    }

    // Update the file size in the directory entry
    newFile.setSize(fileSize);
    newFile.updateModificationTime();
}

void FileSystem::saveFileSystem() const
{
    std::ofstream file(fat.getFileName(), std::ios::binary | std::ios::out | std::ios::in);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file system for saving.\n";
        return;
    }

    // Save the block size at the beginning of the file
    double blockSize = fat.getBlockSize();
    file.write(reinterpret_cast<const char *>(&blockSize), sizeof(blockSize));

    // Seek to the end of 4096 blocks
    file.seekp(fat.totalBlocks * blockSize, std::ios::beg);

    // Save FAT entries
    for (int i = 0; i < fat.totalBlocks; ++i)
    {
        file.write(reinterpret_cast<const char *>(&fat.FAT[i]), sizeof(FAT12::FATEntry));
    }

    // Save directory entries
    uint32_t entryCount = directoryEntries.size();
    file.write(reinterpret_cast<const char *>(&entryCount), sizeof(entryCount));
    for (const auto &entry : directoryEntries)
    {
        file.write(reinterpret_cast<const char *>(&entry), sizeof(entry));
    }
    file.close();
}

void FileSystem::loadFileSystem(const std::string &fileName)
{
    std::ifstream file(fileName, std::ios::binary | std::ios::in);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file system for loading.\n";
        return;
    }

    // Read the block size from the beginning of the file
    double blockSize;
    file.read(reinterpret_cast<char *>(&blockSize), sizeof(blockSize));
    fat.setBlockSize(blockSize);

    // Seek to the end of 4096 blocks
    file.seekg(fat.totalBlocks * blockSize, std::ios::beg);

    // Load FAT entries
    for (int i = 0; i < fat.totalBlocks; ++i)
    {
        file.read(reinterpret_cast<char *>(&fat.FAT[i]), sizeof(FAT12::FATEntry));
    }

    // Load directory entries
    directoryEntries.clear();
    uint32_t entryCount;
    file.read(reinterpret_cast<char *>(&entryCount), sizeof(entryCount));

    if (entryCount > fat.totalBlocks)
    {
        std::cerr << "Error: Entry count exceeds expected number.\n";
        return;
    }

    for (uint32_t i = 0; i < entryCount; ++i)
    {
        DirectoryEntry entry;
        file.read(reinterpret_cast<char *>(&entry), sizeof(entry));
        directoryEntries.push_back(entry);
    }

    file.close();
}

void FileSystem::writeFileWithPassword(const std::string &fileName, const std::string &content, std::string password)
{
    std::string parentName = getParentDirectoryName(fileName);
    std::string shortFileName = getDirectoryName(fileName);

    auto it = std::find_if(directoryEntries.begin(), directoryEntries.end(),
                           [&](const DirectoryEntry &entry)
                           { return entry.getFileName() == parentName && ((entry.getAttributes() & 0x10) || (entry.getAttributes() & 0x11) || (entry.getAttributes() & 0x12) || (entry.getAttributes() & 0x13)); });

    if (it == directoryEntries.end())
    {
        std::cerr << "Parent directory not found.\n";
        return;
    }

    DirectoryEntry *parentDir = &(*it);

    // Check if the file already exists
    if (fileExistinDirectoryEntry(parentDir, shortFileName))
    {
        std::cerr << "File already exists.\n";
        return;
    }

    int block = fat.allocateBlock();
    if (block == -1)
    {
        std::cerr << "No space left to allocate new file.\n";
        return;
    }

    DirectoryEntry newFile(shortFileName, block, content.size(), 0x23); // Set as file
    newFile.setPassword(password);
    newFile.updateModificationTime();
    directoryEntries.push_back(newFile);

    // Write the new file entry to the parent directory's block
    writeDirectoryEntryToPage(parentDir->getFirstBlock(), newFile);

    // Write the content to the allocated blocks
    std::string::size_type contentIndex = 0;
    int currentBlock = block;
    while (contentIndex < content.size())
    {
        std::ofstream file(fat.getFileName(), std::ios::binary | std::ios::in | std::ios::out);
        if (!file.is_open())
        {
            std::cerr << "Failed to open filesystem file.\n";
            return;
        }

        file.seekp(currentBlock * fat.getBlockSize(), std::ios::beg);
        std::string::size_type bytesToWrite = std::min(static_cast<std::string::size_type>(fat.getBlockSize()), content.size() - contentIndex);
        file.write(content.c_str() + contentIndex, bytesToWrite);
        contentIndex += bytesToWrite;
        file.close();

        if (contentIndex < content.size())
        {
            int nextBlock = fat.allocateBlock();
            if (nextBlock == -1)
            {
                std::cerr << "No space left to allocate new block.\n";
                return;
            }

            fat.setNextBlock(currentBlock, nextBlock);
            currentBlock = nextBlock;
        }
        else
        {
            fat.setNextBlock(currentBlock, -1);
        }
    }
}

void FileSystem::printBits(unsigned char byte)
{
    std::bitset<8> bits(byte);
    std::cout << bits << std::endl;
}

void FileSystem::writeFileWithAttribute(const std::string &fileName, const std::string &content, char attributeNew)
{
    std::string parentName = getParentDirectoryName(fileName);
    std::string shortFileName = getDirectoryName(fileName);

    auto it = std::find_if(directoryEntries.begin(), directoryEntries.end(),
                           [&](const DirectoryEntry &entry)
                           { return entry.getFileName() == parentName && (entry.getAttributes() & 0x10); });

    if (it == directoryEntries.end())
    {
        std::cerr << "Parent directory not found.\n";
        return;
    }

    DirectoryEntry *parentDir = &(*it);

    // Check if the file already exists
    if (fileExistinDirectoryEntry(parentDir, shortFileName))
    {
        std::cerr << "File already exists.\n";
        return;
    }

    int block = fat.allocateBlock();
    if (block == -1)
    {
        std::cerr << "No space left to allocate new file.\n";
        return;
    }

    DirectoryEntry newFile(shortFileName, block, content.size(), attributeNew); // Set as file
    newFile.updateModificationTime();
    directoryEntries.push_back(newFile);

    // Write the new file entry to the parent directory's block
    writeDirectoryEntryToPage(parentDir->getFirstBlock(), newFile);

    // Write the content to the allocated blocks
    std::string::size_type contentIndex = 0;
    int currentBlock = block;
    while (contentIndex < content.size())
    {
        std::ofstream file(fat.getFileName(), std::ios::binary | std::ios::in | std::ios::out);
        if (!file.is_open())
        {
            std::cerr << "Failed to open filesystem file.\n";
            return;
        }

        file.seekp(currentBlock * fat.getBlockSize(), std::ios::beg);
        std::string::size_type bytesToWrite = std::min(static_cast<std::string::size_type>(fat.getBlockSize()), content.size() - contentIndex);
        file.write(content.c_str() + contentIndex, bytesToWrite);
        contentIndex += bytesToWrite;
        file.close();

        if (contentIndex < content.size())
        {
            int nextBlock = fat.allocateBlock();
            if (nextBlock == -1)
            {
                std::cerr << "No space left to allocate new block.\n";
                return;
            }

            fat.setNextBlock(currentBlock, nextBlock);
            currentBlock = nextBlock;
        }
        else
        {
            fat.setNextBlock(currentBlock, -1);
        }
    }
}