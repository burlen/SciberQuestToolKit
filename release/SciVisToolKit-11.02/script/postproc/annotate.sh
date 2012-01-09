#!/bin/bash


if [ $# -lt 3 ]
then
  echo "Error: $0 /path/to/input /path/to/output text" 1>&2
  exit -1
fi

INPUT=$1
OUTPUT=$2
TEXT=$3
POS=$4
FONT_SIZE=$5
FONT_COLOR=$6

# default font size
if [[ -z "$POS" ]]
then
  POS="lr"
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

gimp -i -b "(annotate \"$INPUT\" \"$OUTPUT\" \"$TEXT\" \"$POS\" \"$FONT_SIZE\" \"$FONT_COLOR\")" -b "(gimp-quit 0)"
