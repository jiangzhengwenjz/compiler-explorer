#!/bin/bash

temp=temp.txt
basepath=$(cd `dirname $0`; pwd)
for source; do true; done

cc -E -I ${basepath}/agbcc -I ${basepath}/include/include -iquote ${basepath}/include -nostdinc ${source} -o ${temp}
rm ${source}
mv ${temp} ${source}
${basepath}/agbcc/agbcc $*
