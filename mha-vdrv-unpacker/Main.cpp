#include <iostream>
#include <filesystem>
#include <memory>

#include "VDRV.h"
#include "zlib.h"

using namespace std;

/**
 * Creates a directory if it does not exist already on the file system.
 */
void ensureDirectory(const string absoluteDirPath) {
    if (!filesystem::exists(absoluteDirPath)) {
        filesystem::create_directories(absoluteDirPath);
    }
}

/**
 * Processes a single file entry from the drive and saves it to the result directory.
 */
void processFile(VDRV& vdrv, DriveMetadataEntry fileEntry, const string currentDestPath) {
    cout << "* " << fileEntry.getFileName() << " -> " << fileEntry.getFileSize() << " B compressed";

    // Append file name to the current destination path.
    const string filePath = currentDestPath + "\\" + fileEntry.getFileName();

    // Read the compressed data from the drive file in a zlib compatible way.
    unique_ptr<char[]> compressedFile = vdrv.readCompressedFile(fileEntry);
    unsigned char* compressedBuf = reinterpret_cast<unsigned char*>(&compressedFile[0]);
    
    // Estimate the length of the uncompressed data and create a new zlib compatible buffer.
    uLongf uncompressedLength = compressBound(fileEntry.getFileSize());
    unique_ptr<unsigned char[]> uncompressedBuf(new unsigned char[uncompressedLength]);

    // Zlib call. Stores the actual length of the uncompressed data back into uncompressed length.
    uncompress(&uncompressedBuf[0], &uncompressedLength, compressedBuf, fileEntry.getFileSize());

    cout << ", " << uncompressedLength << " B uncompressed" << endl;

    // Write out the uncompressed data.
    ofstream binFile(filePath, ios::out | ios::binary);
    if (binFile.is_open())
    {
        size_t len = fileEntry.getFileSize();
        binFile.write(reinterpret_cast<char*>(&uncompressedBuf[0]), uncompressedLength);
    }
}

/**
 * Processes a single directory entry from the drive, recursively walking into sub-directories and writing out files.
 */
void processDirectory(VDRV& vdrv, DriveMetadata metadata, DriveMetadataEntry directoryEntry, const string currentDestPath) {
    // Append file name to the current destination path.
    const string currentDirPath = currentDestPath + "\\" + directoryEntry.getFileName();

    cout << endl << "Processing directory: " << currentDirPath << endl;

    // Create the directory in the file system.
    ensureDirectory(currentDirPath);
    
    // Get all entries contained within this directory (files and sub-directories).
    vector<DriveMetadataEntry> childEntries = metadata.getChildEntries(directoryEntry);

    // Separate the list by of entries by entry type. The only purpose of this split is so we can
    // output the console logs in the right order without putting in any actual effort, tbh.
    vector<DriveMetadataEntry> fileEntries;
    vector<DriveMetadataEntry> dirEntries;

    copy_if(childEntries.begin(), childEntries.end(), back_inserter(fileEntries), [](DriveMetadataEntry entry) {
        return !entry.isDirectory();
    });
    
    copy_if(childEntries.begin(), childEntries.end(), back_inserter(dirEntries), [](DriveMetadataEntry entry) {
        return entry.isDirectory();
    });

    // Write out files first for correct console print order.
    for (auto entry : fileEntries) {
        processFile(vdrv, entry, currentDirPath);
    }
    
    // Recursively walk any sub-directories.
    for (auto entry : dirEntries) {
        processDirectory(vdrv, metadata, entry, currentDirPath);
    }
}

/**
 * Entry point.
 * Takes in two arguments:
 * 1) Source path to the VDRV file.
 * 2) Destination directory to unpack the files to.
 */
int main(int argc, char* argv[])
{
    cout << "MHA2-VDRV-UNPACKER" << endl;
    cout << "==================" << endl;

    // Ensure argument list is correct.
    if (argc != 3) {
        cout << "Usage: " << argv[0] << " SOURCE_VDRV DESTINATION_FOLDER" << endl;
        return 1;
    }

    const char* sourcePath = argv[1];
    const string destPath = string(argv[2]);

    cout << "Source: " << sourcePath << endl;
    cout << "Destination: " << destPath << endl;

    // Check if the VDRV file actually exists.
    // This must suffice as an integrity check for now, in the future we could also read the magic string at the beginning of the file.
    if (!filesystem::exists(sourcePath)) {
        cout << "Source file does not exist." << endl;
        return 1;
    }

    try
    {
        ensureDirectory(destPath);

        cout << endl << "# 1. Read metadata" << endl << endl;
        cout << "Opening drive..." << endl;
    
        VDRV vdrv(sourcePath);

        cout << "=> Drive file size: " << vdrv.getFileSize() << " B." << endl;
        cout << "Parsing metadata...";

        // Decrypts and parses the obfuscated/encrypted metadata which tells us where 
        // which files are located and how they are linked.
        DriveMetadata meta = vdrv.readMetadata();
        
        cout << " DONE." << endl;
        cout << "=> Found " << meta.getSize() << " entries in the drive metadata." << endl;
        cout << "Generating list of root folders...";

        vector<DriveMetadataEntry> rootEntries = meta.getRootEntries();

        cout << " DONE." << endl;
        cout << endl << "# 2. Unpack drive" << endl;

        for (auto entry : rootEntries) {
            processDirectory(vdrv, meta, entry, destPath);
        }

        cout << endl << "Drive fully unpacked." << endl;
    } catch (std::exception& e)
    {
        cout << endl << "Error during execution: " << e.what() << endl;
    }

    return 0;
}