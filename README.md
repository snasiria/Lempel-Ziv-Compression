# Assignment 6: Lempel-Ziv Compression

## Short Description:

This program will perform Lempel-Ziv Compression. The user will be able to call on a file to be compressed using the executable 'encode' and decompress it by using the executable 'decode'. 

## Build:

In order to build, run '$make', '$make all' to create the executable files 'encode' and 'decode' in a command prompt terminal. In order to individually make each of the executable files, type 'make encode' or 'make decode' in the command prompt terminal. This will create all the necessary object files for each executable file, which the user can run.

## Cleaning:

To clean the directory after building all the object files and executable file, type '$make clean' to remove all the executable files and all the object files from the directory.

## User Inputs:

In order to get a list of valid inputs for each exectuable, simply type the exectuable followed with a '-h' which will produce a usage message that will guide the user to the potential options and what they each do. For example, to get the list of user inputs for the exectuable 'encode', type './encode -h'. Invalid user inputs will bring the user back to the usage message.

## Potential Bugs/Known Errors:

There are no known bugs in the program and there is no memory leakage from any of the executables. There were also no bugs found when I ran scan-build for each of the 2 executable files.

## Running:

In order to run, type '$./encode' or '$./decode', followed by a valid argument(s) that is listed in their respective usage messages. The 'encode' executable must be ran first, followed by the 'decode' executable in order to produce a compressed file and a file that decompresses the compressed file.
