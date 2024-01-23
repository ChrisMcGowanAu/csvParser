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

#define nullptr NULL

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

void freeMem(CsvType *csv);
CsvType *readCsv(char *filename, char seperator);
CsvCellType getCell(CsvType *csv, uint32_t row, uint32_t col);
uint32_t numRows(CsvType *csv);
uint32_t numCols(CsvType *csv);

int64_t getInt64FromCell(CsvCellType cell);
int32_t getInt32FromCell(CsvCellType cell);
float getFloatFromCell(CsvCellType cell);
double getDoubleFromCell(CsvCellType cell);

#ifdef __cplusplus
}
#endif

// If compiled with a c++ compiler, the class can be declared
#ifdef __cplusplus
class csvParser {
public:
  csvParser(char *filename, char sep) { csv = readCsv(filename, sep); }
  CsvCellType getCell(uint32_t row, uint32_t col) {
    return getCell(csv, row, col);
  }
  uint32_t numRows() { return numRows(csv); }
  uint32_t numCols() { return numCols(csv); }
  ~csvParser() { freeMem(csv); }

private:
  CsvType *csv;
}
#endif