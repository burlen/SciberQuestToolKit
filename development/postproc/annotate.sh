#!/bin/bash
#
# crop.sh InFile OutFile Text FontSize X Y 
#
#  
if [ $#  != 6 ] ; then
  echo "Usage: $0 infile outfile text size xcoord ycoord"
  exit 1plug_in_script_fu_eval
fi
#infile=$1
#outfile=$2
#echo "#2 is $2"
#text=$3
#size=$4
#xcoord=$5
#ycoord=$6
gimp --batch-interpreter plug_in_script_fu_eval -i -b "(sciber-annotate \"$1\" \"$2\" \"$3\" $4 $5 $6 )" -b  '(gimp-quit 0)'

