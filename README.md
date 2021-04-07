# mha-vdrv-unpacker

Unpacker for the "virtual drive" of the point-and-click adventure [Moorhuhn Adventure 2 - Der Fluch des Goldes](https://en.wikipedia.org/wiki/Moorhuhn) from 2005.

In an effort to learn more about the inner workings of the games from the early 2000s, I decided to poke around in the game files of some of my favorite games back then. This one proved to be surprisingly challenging, as for some unknown reason, there was some effort put in to obfuscate/encrypt part of the game data. This is unlike anything this studio did at the time, so it piqued my interest.

I reverse engineered the entire VDRV format (mha2.dat) and wrote an unpacker for it to preserve some of that knowledge. This will output the entire file tree stored in the .dat file.

## Usage 

This project can be compiled via Visual Studio, as you would normally do.

The resulting binary can be executed as follows:

```
.\mha-vdrv-unpacker.exe X:\mha2.dat X:\result
```

## VDRV Format

Here follows a brief summary of how the file format works.

The VDRV Format basically consists of three sections:

1. File header
2. Compressed game files
3. Metadata section

Generally, all game files are stored unencrypted after the header, but they are compressed via zlib.

To know where the individual files lie within that compressed section, what the file names are and how they are organized into a directory structure, you need to read the metadata section.
This section is located at the bottom of the file and can be found by reading a pointer from the header.

Here's where the encryption comes into play:
The metadata entries are composed of an unencrypted and encrypted portion.

* The unencrypted portion basically just forms a linked list and tells you whether the following entry is a directory or a file, and where the adjacent entries will be located.
* The encrypted portion contains the file name, location within the VDRV, compressed size, as well as more metadata (like a pointer to the parent directory).

As far as I can tell, the encryption algorithm is a completely custom implementation. It is composed of some XOR operations and byte swaps, nothing out of the ordinary.

All files that are unpacked from the drive are then again custom formats, some easier to understand than others (snd/mus is basically just an OGG with some additional header, for example).

## File hashes (SHA-256)

```
mha2.dat                    64b5d4902f7257241e2ebda1fe66aac4a0c381b9fcf87138c5ca0c99282d361f
mha-vdrv-unpacker.exe       10f45529248662c06fc24e9d3970f3db877d481454b820f8cffe9336c7208744
zlibd.dll                	1ccd8b5c23ebae6a88d05d8dab5012f9f3fe728b28eee0727b960d8af2ad4090
```

## License

By contributing, you agree that your contributions will be licensed under its [Unlicense](http://unlicense.org/).