#include "DriveMetadataEntry.h"

DriveMetadataEntry::DriveMetadataEntry(const string fileName, const uint entryOffset, const uint fileSize, const uint fileStart, const uint parentOffset, const DriveMetadataEntryType entryType):
    fileName(fileName), entryOffset(entryOffset), fileSize(fileSize), fileStart(fileStart), parentOffset(parentOffset), entryType(entryType) {}

/**
 * Gets the name of the file or directory.
 */
string DriveMetadataEntry::getFileName()
{
    return this->fileName;
}

/**
 * Gets the size of the zlib compressed data. 0 if type is directory.
 */
uint DriveMetadataEntry::getFileSize()
{
    return this->fileSize;
}

/**
 * Gets the position at which the zlib compressed file starts. 0 if type is directory.
 */
uint DriveMetadataEntry::getFileStart()
{
    return this->fileStart;
}

/**
 * Gets the position at which the parent entry itself is located. 0 if there is no parent (root level).
 */
uint DriveMetadataEntry::getParentOffset()
{
    return this->parentOffset;
}

/**
 * Gets the position at which the metdata entry itself is located.
 */
uint DriveMetadataEntry::getEntryOffset()
{
    return this->entryOffset;
}

/**
 * Gets the type of this entry.
 */
DriveMetadataEntryType DriveMetadataEntry::getEntryType()
{
    return this->entryType;
}

/**
 * Gets whether the metadata entry is of a directory.
 */
bool DriveMetadataEntry::isDirectory()
{
    return this->entryType == DriveMetadataEntryType::DIRECTORY;
}
