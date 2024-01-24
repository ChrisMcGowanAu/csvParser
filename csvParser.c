/*
**************************************
MIT License
See LICENCE at https://github.com/ChrisMcGowanAu/csvParser
Copyright (c) 2024 Chris McGowan
**************************************
*/

#include "csvParser.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINEMAX 8096
// DEBUGME can be 0 1 2 3 4 5
#define DEBUGME 0

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
  uint32_t numCols = 1;
  if (rowPtr == nullptr) {
    return 0;
  }
  CellType *colPtr = rowPtr->first;
  CellType *nextPtr = nullptr;
  while (colPtr != nullptr) {
    nextPtr = colPtr->next;
    colPtr = nextPtr;
    numCols++;
  }
  return (numCols);
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
  CsvCellType cell = {};
  cell.status = emptyCell;
  cell.lastCellInRow = true;
  cell.bytes = 0;
  RowType *rowPtr = csv->firstRow;
  uint32_t rowNumber = 0;
  if (rowPtr == nullptr) {
    if (DEBUGME > 0)
      printf("No Rows defined\n");
    cell.status = missingRow;
    return (cell);
  }
  while (rowPtr->next != nullptr && rowNumber != row) {
    rowPtr = rowPtr->next;
    rowNumber++;
  }
  if (rowNumber != row) {
    if (DEBUGME > 0)
      printf("row %d not found\n", row);
    cell.status = missingRow;
    return (cell);
  }
  // Find the Column
  CellType *colCellPtr = rowPtr->first;
  if (colCellPtr == nullptr) {
    rowPtr->numCols = 0;
    if (DEBUGME > 0)
      printf("No columns for row %d\n", row);
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
      printf("columns %d for row %d not found\n", col, row);
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
  char dquote = '"';
  // This is an attempt to identify some 8 bit double quote excel generates
  // It does not seem to work. More to be done here to identify these
  // use uint8_t ?
  char wierdDquote = 0xe2;
  RowType *row = (RowType *)malloc(sizeof(RowType));
  memset((void *)row, 0, sizeof(RowType));
  row->first = nullptr;
  if (csv->firstRow == nullptr) {
    csv->firstRow = row;
    row->next = nullptr;
    row->prev = nullptr;
    row->rowId = 0;
  } else {
    RowType *lastRow = csv->firstRow;
    while (lastRow->next != nullptr) {
      lastRow = lastRow->next;
    }
    lastRow->next = row;
    row->next = nullptr;
    row->prev = lastRow;
    row->rowId = lastRow->rowId + 1;
  }

  uint32_t pos = 0;
  uint32_t lastPos = 0;
  char cellBuf[LINEMAX];
  bool insideWierdDquote = false;
  bool insideDquote = false;
  for (uint32_t i = 0; i < strlen(buffer); i++) {
    // See comment above on 0xE2 ( '"' + 0xC0 ? )
    if (buffer[i] == wierdDquote) {
      if (insideWierdDquote) {
        insideWierdDquote = false;
      } else {
        insideWierdDquote = true;
      }
    }
    if (buffer[i] == dquote) {
      if (insideDquote) {
        insideDquote = false;
      } else {
        insideDquote = true;
      }
    }
    if ((buffer[i] == sep && !insideDquote && !insideWierdDquote) ||
        buffer[i] == '\n' || buffer[i] == '\r') {
      lastPos = pos;
      pos = i;
      // Extract the string between lastPas and Pos
      CellType *cellPtr = (CellType *)malloc(sizeof(CellType));
      cellBuf[0] = 0;
      cellPtr->cell.bytes = 0;
      int start = lastPos + 1;
      int finish = pos;
      if (lastPos == 0) {
        start = 0;
      }
      if (finish < 0) {
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
        cellPtr->cell.cellContents = (char *)malloc(len + 4); // hack
        strncpy(cellPtr->cell.cellContents, cellBuf, len);
        if (DEBUGME > 1)
          printf("cellBuf %d %s\n", len, cellBuf);
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
      // detect the end of a line
      if (buffer[i] == '\n' || buffer[i] == '\r') {
        break;
      }
    }
  }
}

CsvType *readCsv(char *filename, char sep) {
  FILE *fp = NULL;
  CsvType *csv = (CsvType *)malloc(sizeof(CsvType));
  bzero((void *)csv, sizeof(CsvType));
  uint32_t lines = 0;
  char line0[LINEMAX];
  char buffer[LINEMAX];
  fp = fopen(filename, "r");
  if (fp != NULL) {
    bzero((void *)line0, sizeof(line0));
    bzero((void *)buffer, sizeof(buffer));
    while (fgets(line0, LINEMAX, fp) != nullptr) {
      strncpy(buffer, line0, sizeof(buffer));
      parseLine(csv, buffer, sep);
      if (DEBUGME > 1)
        printf("Parsing %s", buffer);
      lines++;
    }
    fclose(fp);
  } else {
    printf("Unable to read %s\n", filename);
  }
  countRowsAndCols(csv);
  return csv;
}

uint32_t numRows(CsvType *csv) { return csv->numRows; }
uint32_t numCols(CsvType *csv) { return csv->numCols; }


