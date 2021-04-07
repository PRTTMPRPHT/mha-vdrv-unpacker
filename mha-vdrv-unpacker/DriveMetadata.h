#pragma once

#include <vector>

#include "DriveMetadataEntry.h"

class DriveMetadata {
public:
    DriveMetadata();
    void addEntry(DriveMetadataEntry entry);
    int getSize();
    DriveMetadataEntry getEntryAt(int pos);
    std::vector<DriveMetadataEntry> getRootEntries();
    std::vector<DriveMetadataEntry> getChildEntries(DriveMetadataEntry entry);
private:
    std::vector<DriveMetadataEntry> entries;
};
