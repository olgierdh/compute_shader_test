#!/bin/sh

function remove_code() {
    INF="$1"
    OUTF="$1.out.tmp"
    sed -n '/#define FOR_STUDENTS_BEGIN/{:a;N;/#define FOR_STUDENTS_END/!ba;N;s/.*\n/#define STUDENTS_TODO\n/};p' $INF > $OUTF
    cp $OUTF $INF
    rm $OUTF
}

for f in $(find ./inc/examples -name '*.hpp' -or -name '*.cpp')
do
    remove_code $f
done

for f in $(find ./src/examples -name '*.hpp' -or -name '*.cpp')
do
    remove_code $f
done
