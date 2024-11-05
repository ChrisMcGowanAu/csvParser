/***************************************
MIT License
See LICENCE at https://github.com/ChrisMcGowanAu/csvParser
Copyright (c) 2024 Chris McGowan
***************************************/

#include "csvParser.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
#include <cstdio>
#include <cstring>
#endif

#define LINEMAX 32 * 1024
// DEBUGME can be 0 1 2 3 4 5
#define DEBUGME 0

const uint32_t excelStartDQ = 0xE2809C;
const uint32_t excelEndDQ = 0xE2809D;
const uint8_t squote = 0x27;
const uint8_t dquote = 0x22;
const uint8_t altDquote = 0xe2;

typedef struct CellType {
  CsvCellType cell;
  struct CellType *next;
} CellType;

typedef struct RowType {
  uint32_t rowId;
  uint32_t numCols;
  // List of cells in this row
  struct CellType *first;
  // linked list of Rows
  struct RowType *prev; // may not be needed
  struct RowType *next;
} RowType;

void freeCell(CellType *cellPtr) {
  if (cellPtr == nullptr) {
    return;
  }
  if (cellPtr->cell.cellContents != nullptr) {
    free(cellPtr->cell.cellContents);
  }
  free(cellPtr);
}

void freeRow(RowType *rowPtr) {
  if (rowPtr == nullptr) {
    return;
  }
  CellType *colPtr = rowPtr->first;
  CellType *nextPtr = nullptr;
  while (colPtr != nullptr) {
    nextPtr = colPtr->next;
    freeCell(colPtr);
    colPtr = nextPtr;
  }
  free(rowPtr);
}

void freeMem(CsvType *csv) {
  RowType *rowPtr = csv->firstRow;
  RowType *nextPtr = nullptr;
  if (rowPtr == nullptr) {
    return;
  }
  while (rowPtr->next != nullptr) {
    nextPtr = rowPtr->next;
    freeRow(rowPtr);
    rowPtr = nextPtr;
  }
  freeRow(rowPtr);
}

uint32_t countCols(RowType *rowPtr) {
  uint32_t nCols = 1;
  if (rowPtr == nullptr) {
    return 0;
  }
  CellType *colPtr = rowPtr->first;
  CellType *nextPtr = nullptr;
  while (colPtr != nullptr) {
    nextPtr = colPtr->next;
    colPtr = nextPtr;
    nCols++;
  }
  return (nCols);
}

void countRowsAndCols(CsvType *csv) {
  RowType *rowPtr = csv->firstRow;
  RowType *nextPtr = nullptr;
  uint32_t rowCount = 1;
  if (rowPtr == nullptr) {
    csv->numRows = 0;
    csv->numCols = 0;
    return;
  }
  while (rowPtr->next != nullptr) {
    nextPtr = rowPtr->next;
    uint32_t colCount = countCols(rowPtr);
    if (colCount > csv->numCols) {
      csv->numCols = colCount;
    }
    rowPtr = nextPtr;
    rowCount++;
  }
  csv->numRows = rowCount;
}

// rows and cols use C convention, the first is indexed '0'
// If comparing with excel or libre office rows and cols, they use 1 .. n
// not 0 .. (n-1)
CsvCellType getCell(CsvType *csv, uint32_t row, uint32_t col) {
  CsvCellType cell;
  cell.status = emptyCell;
  cell.lastCellInRow = true;
  cell.bytes = 0;
  cell.cellContents = nullptr;
  RowType *rowPtr = csv->rowLookup[row];
  uint32_t rowNumber = 0;
  if (rowPtr == nullptr) {
    if (DEBUGME > 1)
      fprintf(stderr, "No Rows defined\n");
    cell.status = missingRow;
    return (cell);
  }
  rowNumber = rowPtr->rowId;
  if (rowNumber != row) {
    if (DEBUGME > 0)
      fprintf(stderr, "row %u not found\n", row);
    cell.status = missingRow;
    return (cell);
  }
  // Find the Column
  CellType *colCellPtr = rowPtr->first;
  if (colCellPtr == nullptr) {
    rowPtr->numCols = 0;
    if (DEBUGME > 1)
      fprintf(stderr, "No columns for row %u\n", row);
    cell.status = missingCol;
    return (cell);
  }
  uint32_t colNumber = 0;
  while (colCellPtr->next != nullptr && colNumber != col) {
    colCellPtr = colCellPtr->next;
    colNumber++;
  }
  if (colNumber != col) {
    if (DEBUGME > 4)
      fprintf(stderr, "columns %u for row %u not found\n", col, row);
    cell.status = missingCol;
    return (cell);
  } else {
    cell = colCellPtr->cell;
  }
  // Is this the last cell in the row?
  if (colCellPtr->next == nullptr) {
    cell.lastCellInRow = true;
  } else {
    cell.lastCellInRow = false;
  }
  // cell is a copy of the cell in the csv tree
  if (cell.bytes == 0) {
    cell.status = emptyCell;
  } else {
    cell.status = normalCell;
  }
  return (cell);
}

void parseLine(CsvType *csv, char *buffer, char sep) {
  // search for a seperator
  // This tries to identify some 8 bit double quote excel generates.
  // There is two, a start and end type. macros -- excelStartDQ and excelEndDQ
  // This is not part of the csv standard
  //
  // the static currLastRow is to remember the current last row added
  // It gets reset when
  static RowType *currLastRow = nullptr;

  RowType *row = (RowType *)malloc(sizeof(RowType));
  memset((void *)row, 0, sizeof(RowType));
  row->first = nullptr;
  // Note: when a new csv file is being read,
  // If the first row is nullptr this resets currLastRow
  if (csv->firstRow == nullptr) {
    csv->firstRow = row;
    row->next = nullptr;
    row->prev = nullptr;
    row->rowId = 0;
    // currLastRow gets reset
    currLastRow = row;
  } else {
    RowType *lastRow = currLastRow;
    lastRow->next = row;
    row->next = nullptr;
    row->prev = lastRow;
    row->rowId = lastRow->rowId + 1;
    currLastRow = row;
  }

  uint32_t pos = 0;
  char cellBuf[LINEMAX];
  bool insideExcelDQ = false;
  bool insideDquote = false;
  for (uint32_t i = 0; (i < strlen(buffer)); i++) {
    uint8_t thisCh = buffer[i];
    if (thisCh == altDquote) {
      uint32_t excelCode = (uint8_t)buffer[i] << 16 |
                           (uint8_t)buffer[i + 1] << 8 | (uint8_t)buffer[i + 2];
      if (excelCode == excelStartDQ) {
        insideExcelDQ = true;
      }
      if (excelCode == excelEndDQ) {
        insideExcelDQ = false;
      }
    }
    if (thisCh == dquote) {
      if (insideDquote) {
        insideDquote = false;
      } else {
        insideDquote = true;
      }
    }
    if ((thisCh == sep || thisCh == '\n' || thisCh == '\r') &&
        (!insideDquote && !insideExcelDQ)) {
      uint32_t lastPos = pos;
      pos = i;
      if (pos == 0) {
        // This is a weird case. the first ch is the seperator
        continue;
      }
      if (i > 0) {
        uint8_t lastCh = buffer[i - 1];
        // This test detects DOS/WINDOWS cr-lf
        // The previous cell was the last
        if (lastCh == '\n' || lastCh == '\r') {
          continue;
        }
      }
      // Extract the string between lastPas and Pos
      CellType *cellPtr = (CellType *)malloc(sizeof(CellType));
      cellBuf[0] = 0;
      cellPtr->cell.bytes = 0;
      int start = lastPos + 1;
      int finish = pos;
      if (lastPos == 0) {
        start = 0;
      }
      if (finish < 0 || i == 0) {
        start = 0;
        finish = 0;
      }
      int bpos = 0;
      for (int p = start; p < finish; p++) {
        cellBuf[bpos] = buffer[p];
        bpos++;
      }
      cellBuf[bpos] = 0;
      cellPtr->next = nullptr;
      uint32_t len = strlen(cellBuf);
      cellPtr->cell.bytes = len;
      if (len == 0) {
        cellPtr->cell.status = emptyCell;
        cellPtr->cell.bytes = 0;
        cellPtr->cell.cellContents = nullptr;
      } else {
        cellPtr->cell.status = normalCell;
        cellPtr->cell.cellContents =
            (char *)malloc(len + 4); // The extra 4 is just insurance.
        strncpy(cellPtr->cell.cellContents, cellBuf, len);
        if (DEBUGME > 1)
          fprintf(stderr, "cellBuf %u %s\n", len, cellBuf);
      }
      // Use this Cell as the start of the list
      if (row->first == nullptr) {
        row->first = cellPtr;
        // or add the cell to the end of the Cell list
      } else {
        CellType *currRow = row->first;
        while (currRow->next != nullptr) {
          currRow = currRow->next;
        }
        currRow->next = cellPtr;
      }
    }
  }
}

////////////////////////////////////////////////////
// An even count (even 0) implies that the start and end of the quoted text
// is in this buffer. An odd number implies that a string carries over to the
// next line(s)
////////////////////////////////////////////////////
uint32_t countCharacter(char *buffer, uint8_t ch) {
  uint32_t nCharacters = 0;
  uint32_t len = strlen(buffer);
  for (int i = 0; i < (int)len; i++) {
    if ((uint8_t)buffer[i] == ch) {
      nCharacters++;
    }
  }
  return nCharacters;
}

////////////////////////////////////////////////////
// Count dquote's
///////////////////////////////////////////////////
uint32_t countDquotes(char *buffer) { return countCharacter(buffer, dquote); }

uint32_t countAltDquotes(char *buffer) {
  // Thank you Microsoft, you have complicated a very simple idea.
  // A pox on you and your editors and overpriced office tools.
  //
  // Rather than count, it returns an even number if no excelStartDQ was found
  // or it found excelEndDQ last in the line. If excelStartDQ was the last
  // found, an odd number is returned
  //
  // Thus an odd count implies that the matching quote is on the next (or later
  // ) line.
  uint32_t nCharacters = 0;
  uint32_t len = strlen(buffer);
  for (int i = 0; i < (int)len; i++) {
    if ((uint8_t)buffer[i] == altDquote) {
      uint32_t excelCode = (uint8_t)buffer[i] << 16 |
                           (uint8_t)buffer[i + 1] << 8 | (uint8_t)buffer[i + 2];
      if (excelCode == excelStartDQ) {
        nCharacters = 1;
      }
      if (excelCode == excelEndDQ) {
        nCharacters = 2;
      }
    }
  }
  return nCharacters;
}

////////////////////////////////////////////////////
// Create and array of Row pointers to make finding
// the correct row fast. This make a big difference
// on very large csv files.
////////////////////////////////////////////////////
void buildRowIndex(CsvType *csv) {
  csv->rowLookup = (RowType **)malloc((csv->numRows + 1) * sizeof(RowType*));
  RowType *rowPtr = csv->firstRow;
  RowType *nextPtr = nullptr;
  uint32_t rowIndex = 0;
  if (rowPtr == nullptr) {
    return;
  }
  csv->rowLookup[rowIndex] = rowPtr;
  while (rowPtr->next != nullptr) {
    nextPtr = rowPtr->next;
    rowPtr = nextPtr;
    rowIndex++;
    csv->rowLookup[rowIndex] = rowPtr;
  }
}

#define SAFELINEMAX (7 * LINEMAX / 8)
////////////////////////////////////////////////////
// Read the csv file
////////////////////////////////////////////////////
CsvType *readCsv(char *filename, char sep) {
  FILE *fp = nullptr;
  CsvType *csv = (CsvType *)malloc(sizeof(CsvType));
  memset((void *)csv, 0, sizeof(struct CsvType));
  fp = fopen(filename, "r");
  if (fp != nullptr) {
    uint32_t lines = 0;
    char buffer[LINEMAX];
    memset((void *) buffer, 0, sizeof(buffer));
    uint32_t startIdx = 0;
    while (fgets(&buffer[startIdx], LINEMAX, fp) != nullptr) {
      if (startIdx > 0) {
        fprintf(stderr, "startIdx %d line %d\n", startIdx, lines);
      }
      if (((lines % 1000) == 0) && (DEBUGME > 0)) {
        fprintf(stderr, "line %d\n", lines);
      }
      uint32_t nDquotes = countDquotes(buffer);
      uint32_t nAltDquotes = countAltDquotes(buffer);
      // if number of double quotes is odd, append next line to string
      if ((nDquotes % 2 == 1) || (nAltDquotes % 2 == 1)) {
        // Basically append the next line to this one to cope with multi line
        // cells
        startIdx = strlen(buffer);
        continue;
      } else {
        parseLine(csv, buffer, sep);
        startIdx = 0;
      }
      // Just in case. Dont let the buffer grow beyond 7*LINEMAX/8
      if (startIdx > SAFELINEMAX) {
        fprintf(stderr, "Mismatched double quotes in %s. Around line %d\n",
                filename, lines);
        parseLine(csv, buffer, sep);
        startIdx = 0;
      }
      if (DEBUGME > 4) {
        fprintf(stderr, "Parsing %s", buffer);
      }
      lines++;
    }
    fclose(fp);
  } else {
    fprintf(stderr, "Unable to read %s\n", filename);
  }
  countRowsAndCols(csv);
  buildRowIndex(csv);
  return csv;
}

uint32_t numRows(CsvType *csv) { return csv->numRows; }
uint32_t numCols(CsvType *csv) { return csv->numCols; }

#ifdef __cplusplus
CsvClass::CsvClass() { csv = nullptr; }
//////////////////////////
CsvClass::~CsvClass() {
  if (csv != nullptr) {
    freeMem(csv);
  }
}

//////////////////////////
uint32_t CsvClass::NumRows() {
  uint32_t count = 0;
  if (csv != nullptr) {
    count = csv->numRows;
  }
  return count;
}

//////////////////////////
uint32_t CsvClass::NumCols() {
  uint32_t count = 0;
  if (csv != nullptr) {
    count = csv->numCols;
  }
  return count;
}
//////////////////////////
bool CsvClass::ReadCsv(char *filename, char sep) {
  bool result = false;
  csv = readCsv(filename, sep);
  if (csv != nullptr) {
    result = true;
  }
  return (result);
}

//////////////////////////
CsvCellType CsvClass::GetCell(uint32_t row, uint32_t col) {
  CsvCellType cell = {};
  cell.status = missingRow;
  if (csv != nullptr) {
    cell = getCell(csv, row, col);
  }
  return (cell);
}
#endif
