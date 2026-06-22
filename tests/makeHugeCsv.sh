#!/bin/bash

if [ "$1" == "" ]
then
    echo Usage: $0 numberOfRows numberOfCols
    exit
fi

if [ "$2" == "" ]
then
    echo Usage: $0 numberOfRows numberOfCols
    exit
fi

r=0
rmax=$1

c=0
cmax=$2
file=hugeFile.csv
echo "" > $file

until [ $r -gt $rmax ]
do
    until [ $c -gt $cmax ]
    do
        echo "$c," >> $file
    done
  echo i: $i
  ((i=i+1))
done
