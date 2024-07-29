#include "DirectoryEntry.h"
#include <ctime>
#include <sstream>

DirectoryEntry::DirectoryEntry()
    : attributes(0x03), firstBlockNumber(0), size(0)
{
    std::memset(fileName, 0, sizeof(fileName));
    std::memset(extension, 0, sizeof(extension));
    std::memset(reserved, 0, sizeof(reserved));
    std::memset(timeField, 0, sizeof(timeField));
    std::memset(dateField, 0, sizeof(dateField));
}

DirectoryEntry::DirectoryEntry(const std::string &name, uint16_t firstBlockNumber, uint32_t size, char attributes)
    : attributes(attributes), firstBlockNumber(firstBlockNumber), size(size)
{
    setFileName(name);
    std::memset(extension, 0, sizeof(extension));
    std::memset(reserved, 0, sizeof(reserved));
    std::memset(timeField, 0, sizeof(timeField));
    std::memset(dateField, 0, sizeof(dateField));
}
std::string DirectoryEntry::getFileName() const
{
    return std::string(fileName);
}

uint32_t DirectoryEntry::getSize() const
{
    return size;
}

uint16_t DirectoryEntry::getFirstBlock() const
{
    return firstBlockNumber;
}

char DirectoryEntry::getAttributes() const
{
    return attributes;
}

void DirectoryEntry::setFileName(const std::string &name)
{
    size_t dotPos = name.find_last_of('.');
    if (dotPos == std::string::npos || dotPos > 8)
    {
        std::strncpy(fileName, name.c_str(), sizeof(fileName) - 1);
        fileName[sizeof(fileName) - 1] = '\0';
    }
    else
    {
        std::strncpy(fileName, name.substr(0, dotPos).c_str(), sizeof(fileName) - 1);
        fileName[sizeof(fileName) - 1] = '\0';
        std::strncpy(extension, name.substr(dotPos + 1).c_str(), sizeof(extension) - 1);
        extension[sizeof(extension) - 1] = '\0';
    }
}

void DirectoryEntry::setAttributes(char attr)
{
    attributes = attr;
}

void DirectoryEntry::setFirstBlock(uint16_t block)
{
    firstBlockNumber = block;
}

void DirectoryEntry::setSize(uint32_t s)
{
    size = s;
}

void DirectoryEntry::setPassword(const std::string &password)
{
    std::strncpy(reserved, password.c_str(), sizeof(reserved) - 1);
    reserved[sizeof(reserved) - 1] = '\0';
}

std::string DirectoryEntry::getPassword() const
{
    return std::string(reserved);
}

void DirectoryEntry::setCurrentTime(char *timeField, char *dateField)
{
    std::time_t t = std::time(nullptr);
    std::tm *now = std::localtime(&t);

    uint16_t timeValue = (now->tm_hour << 11) | ((now->tm_min - 2) << 6) | (now->tm_sec / 2);
    timeField[0] = timeValue & 0xFF;
    timeField[1] = (timeValue >> 8) & 0xFF;

    uint16_t dateValue = ((now->tm_year - 81) << 11) | ((now->tm_mon +3) << 5) | now->tm_mday;
    dateField[0] = dateValue & 0xFF;
    dateField[1] = (dateValue >> 8) & 0xFF;
}

void DirectoryEntry::updateModificationTime()
{
    setCurrentTime(timeField, dateField);
}

std::string DirectoryEntry::getFormattedTime() const
{
    uint16_t timeValue = (timeField[1] << 8) | timeField[0];
    int hour = (timeValue >> 11) & 0x1F;
    int minute = (timeValue >> 5) & 0x3F;
    int second = (timeValue & 0x1F) * 2;
    std::stringstream ss;
    ss << (hour < 10 ? "0" : "") << hour << ":"
       << (minute < 10 ? "0" : "") << minute << ":"
       << (second < 10 ? "0" : "") << second;
    return ss.str();
}

std::string DirectoryEntry::getFormattedDate() const
{
    uint16_t dateValue = (dateField[1] << 8) | dateField[0];
    int year = ((dateValue >> 9) & 0x7F) + 1980;
    int month = (dateValue >> 5) & 0x0F;
    int day = dateValue & 0x1F;
    std::stringstream ss;
    ss << year << "-"
       << (month < 10 ? "0" : "") << month << "-"
       << (day < 10 ? "0" : "") << day;
    return ss.str();
}
