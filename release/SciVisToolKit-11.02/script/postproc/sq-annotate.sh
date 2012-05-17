#!/bin/bash
#   ____    _ __           ____               __    ____
#  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
# _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
#/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)
#
#Copyright 2010 SciberQuest Inc.

if [ $# -lt 3 ]
then
  exe=`basename $0`
  echo "Error: $exe /path/to/input /path/to/output text [xpos:ll,lr,ur,ul,x] [ypos] [size] [color]" 1>&2
  exit -1
fi

INPUT=$1
OUTPUT=$2
TEXT=$3
XPOS=$4
YPOS=$5
FONT_SIZE=$6
FONT_COLOR=$7
FONT_FACE=$8

if [[ -z "$XPOS" ]]
then
  XPOS="lr"
fi

if [[ -z "$YPOS" ]]
then
  YPOS=""
fi


if [[ -z "$FONT_SIZE" ]]
then
  FONT_SIZE=25
fi

if [[ -z "$FONT_COLOR" ]]
then
  FONT_COLOR="w"
fi

if [[ -z "$FONT_FACE" ]]
then
  FONT_FACE="DejaVu Sans Bold"
fi

#"Nimbus Mono L Bold Oblique"

gimp -i -b "(sq-annotate \"$INPUT\" \"$OUTPUT\" \"$TEXT\" \"$XPOS\" \"$YPOS\" \"$FONT_SIZE\" \"$FONT_COLOR\" \"$FONT_FACE\")" -b "(gimp-quit 0)"
