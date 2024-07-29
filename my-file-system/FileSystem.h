#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "FAT12.h"
#include "DirectoryEntry.h"
#include <string>
#include <vector>

class FileSystem
{
public:
    FileSystem(double blockSizeKB, const std::string &fileName);

    bool filesystemExists(const std::string &fileName) const;
    void listDirectory() const;
    void printFileSystem() const;
    void makeDirectory(const std::string &dirName);
    void removeDirectory(const std::string &dirName);
    void writeFile(const std::string &fileName, const std::string &content);
    void listDirectory(const std::string &path) const;
    void readFile(const std::string &fileName, std::string &content);
    void readFile(const std::string &fileName, const std::string &outputFile, const std::string &password = "");
    void deleteFile(const std::string &fileName);
    void changeMode(const std::string &fileName, const std::string &permissions);
    void addPassword(const std::string &fileName, const std::string &password);
    void dumpe2fs();
    void printDirectoryPages();
    void printBlockContents() const;
    void saveFileSystem() const;
    void loadFileSystem(const std::string &fileName);
    void writeFileToFile(const std::string &fileName, const std::string &linuxFileName);

private:
    FAT12 fat;
    std::vector<DirectoryEntry> directoryEntries;

    void writeFileWithPassword(const std::string &fileName, const std::string &content, std::string password);
    void initializeFileSystem();
    std::string getAttributesString(char attributes) const;
    void saveDirectoryEntries();
    void printBits(unsigned char byte);
    void writeFileWithAttribute(const std::string &fileName, const std::string &content, char Attribute);
    void loadDirectoryEntries();
    void writeDirectoryEntryToPage(int block, const DirectoryEntry &dirEntry);
    void saveDirectoryEntry(const DirectoryEntry &entry);
    void addDirectoryEntryToParent(const DirectoryEntry &entry, int parentBlock);
    std::vector<std::string> splitPath(const std::string &path);
    DirectoryEntry *findDirectoryEntry(const std::string &path);
    DirectoryEntry *findDirectoryEntryByBlock(int block);
    bool fileExistinDirectoryEntry(DirectoryEntry *parent, const std::string &path);
    std::string getParentDirectoryName(const std::string &path);
    std::string getDirectoryName(const std::string &path);
    bool filesystemExists(const std::string &fileName);
};

#endif // FILESYSTEM_H
