#include "DriveMetadata.h"

#include <algorithm>
#include <iterator>

using namespace std;

DriveMetadata::DriveMetadata():
    entries()
{}

/**
 * Adds an entry to the list.
 */
void DriveMetadata::addEntry(const DriveMetadataEntry entry)
{
    this->entries.push_back(entry);
}

/**
 * Gets the total amount of metadata entries present.
 */
int DriveMetadata::getSize()
{
    return this->entries.size();
}

/**
 * Gets an entry from the metadata list at a certain list index.
 */
DriveMetadataEntry DriveMetadata::getEntryAt(int pos)
{
    return this->entries.at(pos);
}

/**
 * Gets a list of all entries that are located at the root of the drive.
 */
vector<DriveMetadataEntry> DriveMetadata::getRootEntries()
{
    vector<DriveMetadataEntry> rootEntries;

    // Filters the list of entries for all entries that have no parent and are a directory.
    copy_if(this->entries.begin(), this->entries.end(), back_inserter(rootEntries), [](DriveMetadataEntry entry) {
        return entry.isDirectory() && entry.getParentOffset() == 0;
    });

    return rootEntries;
}

/**
 * Gets a list of all the sub-entries of a directory type entry.
 */
vector<DriveMetadataEntry> DriveMetadata::getChildEntries(DriveMetadataEntry entry)
{
    vector<DriveMetadataEntry> childEntries;

    // Filter list of entries for all entries that have a matching parent offset.
    copy_if(this->entries.begin(), this->entries.end(), back_inserter(childEntries), [entry] (DriveMetadataEntry otherEntry) mutable {
        return entry.getEntryOffset() == otherEntry.getParentOffset();
    });

    return childEntries;
}
