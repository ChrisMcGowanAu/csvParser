#ifndef __CSV_PARSER__
#define __CSV_PARSER__

/*
**************************************
MIT License
See LICENCE at https://github.com/ChrisMcGowanAu/csvParser
Copyright (c) 2024 Chris McGowan
**************************************
*/

#include <stdbool.h>
#include <stdint.h>

//////////////////////////////////////
// The rows are a linked list
// Each row has a linked list of cells
// R - C - C
// |
// R - C -C - C - C - C
// |
// R - C -C - C - C
// |
// etc
//////////////////////////////////////

#ifndef __cplusplus
#  define nullptr NULL
#endif

typedef enum CellStatusType {
  emptyCell = 0,  // row and col ok, but cell had no data
  missingRow = 1, // row not found
  missingCol = 2, // col not found
  normalCell = 4  // cell has data
} CellStatusType;

typedef struct CsvCellType {
  // and empty cell has 0 bytes
  uint32_t bytes;
  CellStatusType status;
  bool lastCellInRow;
  char *cellContents;
} CsvCellType;

typedef struct RowType RowType; // Defined in the c file

typedef struct CsvType {
  RowType  **rowLookup;
  uint32_t numRows;
  uint32_t numCols;
  RowType *firstRow;
} CsvType;

////////////////////////
// Functions
////////////////////////
#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////
// Read the csv file into memory
///////////////////////////////////////////////////////
CsvType *readCsv(char *filename, char seperator);

///////////////////////////////////////////////////////
// Get the cell value at row,col
///////////////////////////////////////////////////////
CsvCellType getCell(CsvType *csv, uint32_t row, uint32_t col);

uint32_t numRows(CsvType *csv);
uint32_t numCols(CsvType *csv);

///////////////////////////////////////////////////////
// free up memory used in the csv tree
///////////////////////////////////////////////////////
void freeMem(CsvType *csv);

#ifdef __cplusplus
}
#endif
// For C++ only
// Class difinitions
#ifdef __cplusplus
class CsvClass {
    public:
        CsvClass(); 
        ~CsvClass(); 
        uint32_t NumRows();
        uint32_t NumCols();
        bool ReadCsv(char *filename, char seperator);
        CsvCellType GetCell(uint32_t row, uint32_t col);
    private:
        CsvType *csv;
};
#endif

#endif
