#!/bin/bash

if [[ $# -lt 3 ]]
then
  echo "usage:"
  echo "overlay.sh /path/to/input /path/to/overlay /path/to/output posX posY"
  exit -1
fi

INPUT=$1
OVERLAY=$2
OUTPUT=$3

if [[ $# -lt 4 ]]
then
  POSX=0
  POSY=0
else
  POSX=$4
  POSY=$5
fi

gimp -i -b "(overlay \"$INPUT\" \"$OVERLAY\" \"$OUTPUT\" \"$POSX\" \"$POSY\")" -b "(gimp-quit 0)"

