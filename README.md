[![Lint Code Base](https://github.com/ChrisMcGowanAu/csvParser/actions/workflows/super-linter.yml/badge.svg)](https://github.com/ChrisMcGowanAu/csvParser/actions/workflows/super-linter.yml)
[![C/C++ CI](https://github.com/ChrisMcGowanAu/csvParser/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/ChrisMcGowanAu/csvParser/actions/workflows/c-cpp.yml)
[![CodeQL](https://github.com/ChrisMcGowanAu/csvParser/actions/workflows/github-code-scanning/codeql/badge.svg)](https://github.com/ChrisMcGowanAu/csvParser/actions/workflows/github-code-scanning/codeql)

A parser for csv files in C, for C and C++

This is a RFC 4180  parser for reading csv files for C.
It also handles csv files created by Excel, including multi line cells.
It can handle csv files with multi-line cells, and handles different new line conventions.

There are just 3 functions, one to read the file and one to get access to the data in each cell, and one to free the memory used when done. It compiles fine using C or C++, so it can be used for C and C++. 

CsvType *csvPtr = readCsv(filename, csvSeperator);

cell = getCell(csvPtr, row, column);

void freeMem(csvPtr);


I have written several csv parsers over the years. These parsers used the troublesome C functions 'strsep' and 'strtok'
either I was using them wrong or they are buggy under a high load, they also behaved in different ways on different machines
and OS's. This csv parser does the tokenising itself, which proved to be easier than getting 'strtok' to work reliably.
The code reads the csv and into a tree like structure, a linked list of rows, each row has a linked list of cells. This makes it fast and easy to access each cell which can be accesses via row and column.

Whilst it is very fast, It should read files of any size, limited by how much memory you have. For extremely large files, it could be made more efficient at the expense of creating fast lookup of rows, if that was needed.

I used to roll my own linked lists before the C++ STL came along, and it was fun for me to write code using linked lists again. 

I have tried to make it RFC 4180 compliant. 
This repository consists of three files
csvParser.h
csvParser.c

The third file csvTest.c can be used as an example and for testing

The csv file is read by the this function

CsvType *csv = readCsv(filename, csvSeperator);

Cells can be acceses via this function
    
cell = getCell(csv, row, column);

The returned cell data structure has enough information to tell is the cell exists, if it is empty and if it the last cell in the list (the end of the line)
Anyway see csvParser.h and csvParser.c

See csvTest.c as an example

Memory Leaks? I could not measure any. :-) 

Possible improvements:
1) Allow single quotes as well as double quotes
2) Create an array of row pointers for fast finding of rows.
