#pragma once

#include <string>

using namespace std;
using uint = uint32_t;

enum class DriveMetadataEntryType {
    FILE = 3,
    DIRECTORY = 4,
};

class DriveMetadataEntry {
public:
    DriveMetadataEntry(const string fileName, const uint entryOffset, const uint fileSize, const uint fileStart, const uint parentOffset, const DriveMetadataEntryType entryType);
    string getFileName();
    uint getFileSize();
    uint getFileStart();
    uint getParentOffset();
    uint getEntryOffset();
    DriveMetadataEntryType getEntryType();
    bool isDirectory();
private:
    const string fileName;
    const uint entryOffset;
    const uint fileSize;
    const uint fileStart;
    const uint parentOffset;
    const DriveMetadataEntryType entryType;
};
