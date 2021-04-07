#pragma once

#include <fstream>
#include <memory>

#include "DriveMetadata.h"

using uint = uint32_t;
using namespace std;

class VDRV {
public:
    VDRV(const char* filePath);
    VDRV(const VDRV&) = delete;
    VDRV& operator=(const VDRV&) = delete;
    uint getFileSize();
    DriveMetadata readMetadata();
    unique_ptr<char[]> readCompressedFile(DriveMetadataEntry entry);
private:
    uint readUInt32FromFile();
    uint readUInt32FromBuffer(const char* buffer, size_t bufferSize, const int from);
    char readUInt8FromFile();
    void readByteArrayFromFile(char* destBuf, size_t arraySize);
    void moveTo(const uint pos);
    uint getCursorPos();
    uint readMetadataEntry(const uint currentReadPointer, DriveMetadata& meta);
    void decryptEntry(unique_ptr<char[]>& entryData, const uint size);
    void decrypt(unique_ptr<char[]>& entryData, const uint from, const uint size);

    std::ifstream in;
    uint fileSize;
};
