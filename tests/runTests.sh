#!/bin/bash
testFiles="
malformed_unclosed_quote.csv
basic.csv
blank.csv
empty_cells.csv
multiline_quoted.csv
no_final_newline.csv
bigFile500000.csv
quoted_commas.csv
trailing_empty_cells.csv"

gunzip bigFile500000.csv.gz

thisdir=`pwd`
cd ..
mkdir build 2> /dev/null
cd build
cmake ..
make
buildDir=`pwd`

echo ""
echo "These tests read the csv file and generate a copy. "
echo "The two files are diffed with 'diff -w' to ignore"
echo "possible difference in line endings. DOS/UNIX conventions"
echo ""
echo "If the diff finds no differences, the test passes"
echo ""
echo "Note: Expecting the malformed_unclosed_quote.csv test to fail"
echo "      I think this is reasonable because the csv is malformed"

cd ${thisdir}
for f in $testFiles
do
    echo ""
    echo "###### Running $f test ...."
    timestart=$(date +%s%3N)
    ${buildDir}/cParserTest $f > /tmp/$f
    timeend=$(date +%s%3N)
    timeElapsedMS=$(( timeend - timestart ))
    diff -w $f /tmp/$f
    if [ "$?" -eq 0 ]
    then
        echo "###### $f test passed it took ${timeElapsedMS}ms to execute."
    else
        echo "###### $f test failed !!"
    fi

done
