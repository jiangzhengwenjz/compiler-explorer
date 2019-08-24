#!/bin/bash

temp=temporaryfile.someextension
basepath=$(cd `dirname $0`; pwd)
for source; do true; done

cc -E -I ${basepath}/include/include -I ${basepath}/include -nostdinc ${source} -o ${temp}
rm ${source}
mv ${temp} ${source}
${basepath}/agbcc/agbcc $*
