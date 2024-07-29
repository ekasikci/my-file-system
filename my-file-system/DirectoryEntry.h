#ifndef DIRECTORYENTRY_H
#define DIRECTORYENTRY_H

#include <string>
#include <cstring>

class DirectoryEntry {
public:
    DirectoryEntry();
    DirectoryEntry(const std::string &name, uint16_t firstBlockNumber, uint32_t size, char attributes);

    std::string getFileName() const;
    uint32_t getSize() const;
    uint16_t getFirstBlock() const;
    char getAttributes() const;

    void setFileName(const std::string &name);
    void setAttributes(char attr);
    void setFirstBlock(uint16_t block);
    void setSize(uint32_t s);
    void setPassword(const std::string &password);
    std::string getPassword() const;
    void setCurrentTime(char *timeField, char *dateField);
    void updateModificationTime();
    std::string getFormattedTime() const;
    std::string getFormattedDate() const;

private:
    char fileName[8];
    char extension[3];
    char attributes;
    char reserved[10];
    char timeField[2];
    char dateField[2];
    uint16_t firstBlockNumber;
    uint32_t size;
};

#endif // DIRECTORYENTRY_H
