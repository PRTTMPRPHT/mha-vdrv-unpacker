#include "VDRV.h"

#include <stdexcept>

using namespace std;

VDRV::VDRV(const char* filePath):
    in(filePath, std::ios::binary | std::ios::ate), fileSize(in.tellg())
{
    in.seekg(0);
}

/**
 * Returns the total size of the read file.
 */
uint VDRV::getFileSize()
{
    return this->fileSize;
}

/**
 * Returns the current position of the file cursor.
 */
uint VDRV::getCursorPos()
{
    return this->in.tellg();
}

/**
 * Reads a metadata entry at a given position within the file, and adds it to the entry list of the metadata object.
 * Returns the position of the next metadata entry.
 */
uint VDRV::readMetadataEntry(const uint currentReadPointer, DriveMetadata& meta)
{
    this->moveTo(currentReadPointer);

    // Determine the first field, the entry type.
    DriveMetadataEntryType entryType = static_cast<DriveMetadataEntryType>(this->readUInt32FromFile());

    // Determine the entire length of this metadata entry.
    const uint entryLength = this->readUInt32FromFile();

    // Skip one value. (Pointer to previous entry)
    this->moveTo(this->getCursorPos() + 0x4);

    // Pointer to the next metadata entry in files.
    const uint nextPointer = this->readUInt32FromFile();
    
    // Size of the encrypted data.
    uint size = entryLength - 0x10;

    // Allocate data on the heap due to dynamic chunk size.
    unique_ptr<char[]> entryData = make_unique<char[]>(size);

    // Read the raw, encrypted data of the entry.
    char* rawDataBuffer = entryData.get();
    this->readByteArrayFromFile(rawDataBuffer, size);

    // Handle decryption for this entry.
    // From here on out we read everything from the decrypted data.
    this->decryptEntry(entryData, size);

    // First we determine the parent entry, so we can later build the directory structure.
    uint parentOffset = this->readUInt32FromBuffer(rawDataBuffer, size, 0x8);

    // Now we read the file name. File name is just a null-terminated string.
    char* fileNamePtr = rawDataBuffer + 0x10;

    vector<char> fileNameVector;
    char currentFileNameByte = *fileNamePtr;

    while (currentFileNameByte != 0)
    {
        fileNameVector.push_back(currentFileNameByte);
        fileNamePtr += 1;
        currentFileNameByte = *fileNamePtr;
    }

    // Build the actual string from the read bytes.

    string fileName(fileNameVector.begin(), fileNameVector.end());

    if (entryType == DriveMetadataEntryType::FILE)
    {
        // Read the remaining values we need to unpack.
        uint fileSize = this->readUInt32FromBuffer(rawDataBuffer, size, size - 0x8);
        uint fileStart = this->readUInt32FromBuffer(rawDataBuffer, size, size - 0x4);

        // Construct the entry and push.
        DriveMetadataEntry entry(fileName, currentReadPointer, fileSize, fileStart, parentOffset, entryType);
        meta.addEntry(entry);
    } else if (entryType == DriveMetadataEntryType::DIRECTORY)
    {
        // We don't need sizes and positions for directories, so we skip straight ahead to pushing the entry.
        DriveMetadataEntry entry(fileName, currentReadPointer, 0, 0, parentOffset, entryType);
        meta.addEntry(entry);
    } else
    {
        throw out_of_range("Found unknown entry type.");
    }

    return nextPointer;
}

/**
 * Decrypts a single metadata entry in the drive.
 */
void VDRV::decryptEntry(unique_ptr<char[]>& entryData, const uint size)
{
    if (size < 0x10)
    {
        throw out_of_range("Entry data length is too short.");
    }

    // Every encrypted entry is actually composed of two seperately encrypted sections.
    // Since the decryption is dependent on the position of the bytes within a section,
    // we need to perform two passes.
    this->decrypt(entryData, 0, 0x10);
    this->decrypt(entryData, 0x10, size - 0x10);
}

/**
 * Moves the file cursor to the given position.
 */
void VDRV::moveTo(const uint pos)
{
    if (pos >= this->fileSize)
    {
        throw out_of_range("Tried to move beyond EOF.");
    }

    this->in.seekg(pos);
}

/**
 * Reads a the encrypted metadata section at the end of the file and parses it into something we can actually process.
 */
DriveMetadata VDRV::readMetadata()
{
    DriveMetadata meta;

    // Read the pointer to the first metadata entry.
    this->moveTo(0x48);
    uint currentReadPointer = this->readUInt32FromFile();

    // Read entries until we reach EOF.
    while (currentReadPointer != 0)
    {
        currentReadPointer = this->readMetadataEntry(currentReadPointer, meta);
    }

    return meta;
}

/**
 * Reads an entire zlib compressed chunk from the drive based on the metadata read.
 */
unique_ptr<char[]> VDRV::readCompressedFile(DriveMetadataEntry entry)
{
    this->moveTo(entry.getFileStart());

    unique_ptr<char[]> resultPointer(new char[entry.getFileSize()]);
    
    this->readByteArrayFromFile(&resultPointer[0], entry.getFileSize());

    return resultPointer;
}

/**
 * Reads a uint32 from the file at the current position and jumps ahead 4 bytes.
 */
uint VDRV::readUInt32FromFile()
{
    uint target;

    if (this->getCursorPos() > this->fileSize - sizeof(target))
    {
        throw out_of_range("Tried to read beyond EOF.");
    }

    this->in.read(reinterpret_cast<char*>(&target), sizeof(target));

    return target;
}

/**
 * Reads a uint32 from the given buffer at the given position.
 */
uint VDRV::readUInt32FromBuffer(const char* buffer, size_t bufferSize, const int from)
{
    uint target;

    if (from > bufferSize - sizeof(target))
    {
        throw out_of_range("Tried to read beyond end of buffer.");
    }

    memcpy(&target, buffer + from, sizeof(target));

    return target;
}

/**
 * Reads a byte from the file at the current position and jumps ahead 1 byte.
 */
char VDRV::readUInt8FromFile()
{
    char target;

    if (this->getCursorPos() > this->fileSize - sizeof(target))
    {
        throw out_of_range("Tried to read beyond EOF.");
    }

    this->in.read(reinterpret_cast<char*>(&target), sizeof(target));

    return target;
}

/**
 * Reads a chunk of bytes from the file at the current position and jumps ahead to the end of the chunk.
 */
void VDRV::readByteArrayFromFile(char* destBuf, size_t arraySize)
{
    if (this->getCursorPos() > this->fileSize - arraySize)
    {
        throw out_of_range("Tried to read array beyond EOF.");
    }

    this->in.read(destBuf, arraySize);
}

/**
 * Decryption function reverse engineered from the original game.
 * It performs a series of XOR operations and byte swaps on the data.
 * This is reconstructed as best as I could to resemble what the assembly does.
 * (And might therefore look a bit nuts.)
 */
void VDRV::decrypt(unique_ptr<char[]>& entryData, const uint from, const uint size)
{
    if (size == 0)
    {
        throw out_of_range("Must read at least one byte.");
    }

    // We are only interested in the encrypted portion of the metadata entry, which is preceded by an unencrypted portion.
    char* payloadPtr = entryData.get() + from;

    // The first byte of the encrypted data just has all of the bits flipped and doesn't need to be decrypted in a complicated way.
    char* encryptedDataPtr = payloadPtr + 1;
    uint encryptedBlockLength = size - 1;
    *payloadPtr = ~*payloadPtr;

    if (encryptedBlockLength > 0)
    {
        // We iterate over the encrypted data byte by byte.
        char* currentReadPointer = encryptedDataPtr;
        uint bytesLeftToRead = encryptedBlockLength;

        // This algorithm uses a decryption key that is modified after each byte decrypted.
        uint currentDecryptionKey = 0;

        // The decryption key is rotated by some bits each time it is used.
        // The pattern by which it is rotated is derived from the length of the encrypted section,
        // which is looped through until we have all bytes decrypted.
        uint mask = ~encryptedBlockLength;

        do
        {
            // Read a byte and XOR it with the last byte of the decryption key.
            *currentReadPointer = *currentReadPointer ^ (currentDecryptionKey & 0xFF);

            // Will be either 5 or 7, depending on the last bit of the mask.
            char rotateAmount = (((mask & 1) << 1) | 5) & 0xFF;

            // We cycle through the bits of the mask until it is completely consumed, then we loop through the pattern again.
            uint newMask = mask >> 1;

            if (newMask == 0)
            {
                newMask = ~encryptedBlockLength;
            }

            mask = newMask;

            // This rotates the bits of the decryption key to the right and then adds 1 to it. (This is equivalent to ror in asm.)
            currentDecryptionKey = (currentDecryptionKey >> rotateAmount | currentDecryptionKey << 0x20 - rotateAmount) + 1;

            // Should the above operation result in an empty key, we fall back to a default key.
            if (currentDecryptionKey == 0)
            {
                currentDecryptionKey = 0x5A3C96E7;
            }

            bytesLeftToRead--;
            currentReadPointer++;
        } while (bytesLeftToRead != 0);

        // Reset the pointer as we start from the beginning of the section again.
        currentReadPointer = encryptedDataPtr;

        // Two of the decryption steps rely on swapping bytes, which means that we need to count n / 2 steps.
        int amountOfByteSwaps = encryptedBlockLength / 2;

        if (amountOfByteSwaps > 0)
        {
            // In this pass, we both swap around two adjacent bytes and XOR them with static values.
            do
            {
                char currentByte = *currentReadPointer;
                char nextByte = *(currentReadPointer + 1);
                *currentReadPointer = nextByte ^ 0xAA;
                *(currentReadPointer + 1) = currentByte ^ 0x55;
                amountOfByteSwaps--;

                // Move forward to the next pair of bytes.
                currentReadPointer += 2;
            } while (amountOfByteSwaps != 0);
        }

        // Reset the pointer again, we start over once more.
        currentReadPointer = encryptedDataPtr;
        bytesLeftToRead = encryptedBlockLength;
        
        // This time, the pattern for bit shifts is derived from the plain block length.
        mask = encryptedBlockLength;

        // Reset the decryption key, we use a similiar mechanism to the first pass that used it.
        currentDecryptionKey = 0;

        if (encryptedBlockLength > 0)
        {
            do
            {
                // Read a byte and XOR it with the last byte of the decryption key.
                *currentReadPointer = *currentReadPointer ^ (currentDecryptionKey & 0xFF);

                // Will be either 11 or 17, depending on the last bit of the mask.
                int8_t rotateAmount = (-((mask & 1) != 0) & 6U) + 0xB;
                
                // Same procedure as above, we cycle through the mask and loop it around.
                uint newMask = mask >> 1;

                if (mask >> 1 == 0)
                {
                    newMask = encryptedBlockLength;
                }

                mask = newMask;

                // This rotates the bits of the decryption key to the left and then adds 1 to it. (This is equivalent to rol in asm.)
                currentDecryptionKey = (currentDecryptionKey << rotateAmount | currentDecryptionKey >> 0x20 - rotateAmount) + 1;

                // Fallback value for the decryption key.
                if (currentDecryptionKey == 0)
                {
                    currentDecryptionKey = 0x5A3C96E7;
                }

                bytesLeftToRead--;
                currentReadPointer++;
            } while (bytesLeftToRead != 0);
        }

        // In the last pass, we swap around bytes from the opposite side, converging to the middle bytes.
        payloadPtr += encryptedBlockLength;
        amountOfByteSwaps = encryptedBlockLength / 2;

        if (amountOfByteSwaps > 0)
        {
            do
            {
                char leftByte = *encryptedDataPtr;
                char rightByte = *payloadPtr;
                *encryptedDataPtr = rightByte ^ 0xF;
                *payloadPtr = leftByte ^ 0xF0;
                encryptedDataPtr++;
                payloadPtr--;
                amountOfByteSwaps--;
            } while (amountOfByteSwaps != 0);
        }
    }
}
