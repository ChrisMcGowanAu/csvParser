#include "csvParser.h"
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
  if (argc < 2) {
    return 1;
  }
  auto csvClass = CsvClass();
  bool ok = csvClass.ReadCsv(argv[argc - 1], ',');
  uint32_t nRows = csvClass.NumRows();
  uint32_t nCols = csvClass.NumCols();
  printf("Read ok %d Rows %u max columns %u\n", ok, nRows, nCols);
  for (uint32_t r = 0; r < nRows; r++) {
    for (uint32_t c = 0; c < nCols; c++) {
      CsvCellType cell = csvClass.GetCell(r, c);
      switch (cell.status) {
      case missingRow:
      case missingCol:
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
}
