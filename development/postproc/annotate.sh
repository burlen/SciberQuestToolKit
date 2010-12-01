#!/bin/bash

INPUT=$1
shift 1

OUTPUT=$1
shift 1

TEXT=$1
shift 1

FONT_SIZE=$1
shift 1

FONT_COLOR=$1
shift 1

if [[ -z "$INPUT" || -z "$OUTPUT" || -z "$TEXT" ]]
then
  echo "Error: $0 /path/to/input /path/to/output text" 1>&2
  exit
fi

# default font size
if [[ -z "$FONT_SIZE" ]]
then
  FONT_SIZE=25
fi

# default font color
if [[ -z "$FONT_COLOR" ]]
then
  FONT_COLOR="w"
fi

gimp -i -b "(annotate \"$INPUT\" \"$OUTPUT\" \"$TEXT\" \"$FONT_SIZE\" \"$FONT_COLOR\")" -b "(gimp-quit 0)"

exit

# crop.sh InFile OutFile Text FontSize X Y 
#
# 1. Check if the fonttype is not defined, if so
#    Use a default font type for this 
if [ $#  -lt  6 ] ; then
  echo "Usage: $0 infile outfile text size xcoord ycoord fonttype"
  exit 1
fi
#infile=$1
#outfile=$2
#echo "#2 is $2"
#text=$3
#size=$4
#xcoord=$5
#ycoord=$6

#
#gimp -i -b "(sciber-crop \"$1\" \"$2\" )" -b '(gimp-quit 0)'


gimp -i -b "(sciber-annotate \"$1\" \"$2\" \"$3\"  $4 $5 $6 \"$7\" )" -b '(gimp-quit 0)' 



