#include "csvParser.h"
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
  if ( argc < 2 ) {
    return 1;
  }
  CsvType *csv = readCsv(argv[argc-1], ',');
  CsvCellType cell = {};
  uint32_t nRows = csv->numRows;
  uint32_t nCols = csv->numCols;
  printf("Rows %d max columns %d\n", nRows, nCols);
  uint32_t r = 0;
  uint32_t c = 0;
  for (r = 0; r < nRows; r++) {
    for (c = 0; c < nCols; c++) {
      cell = getCell(csv, r, c);
      switch (cell.status) {
      case missingRow:
      case missingCol:
        // printf("?");
        break;
      case emptyCell:
        if (cell.lastCellInRow == false)
          printf(",");
        break;
      case normalCell:
        if (cell.bytes == 0) {
          printf("->Zero bytes for a normal Cell<-");
        }
        printf("%s", cell.cellContents);
        if (cell.lastCellInRow == false)
          printf(",");
        break;
      default:
        printf("->WTF? %d <-", (int)cell.status);
        break;
      }
    }
    printf("\n");
  }
  // sleep(1);
  freeMem(csv);
}
