#!/bin/bash
tests="basic.csv
doubled_quotes.csv
empty_cells.csv
malformed_unclosed_quote.csv
multiline_quoted.csv
no_final_newline.csv
quoted_commas.csv
trailing_empty_cells.csv"

for c in $tests 
do
    echo $c
    echo "---file----" 
    cat $c
    echo "---output---" 
    ../build/cParserTest $c
    echo "---------" 
done
