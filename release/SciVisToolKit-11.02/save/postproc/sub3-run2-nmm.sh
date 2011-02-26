#!/bin/bash

for f in `ls ../*.png`; 
do 
  ff=`echo $f | cut -d/ -f2`;
  n=`echo $f | cut -d. -f4`; 
  echo -ne "$n $f -> $ff\n"
  eval `echo "gimp -i -b '(sub3-run2-nmm \"$f\" \"$ff\" \"sub3/$n\")' -b '(gimp-quit 0)'"`
done
