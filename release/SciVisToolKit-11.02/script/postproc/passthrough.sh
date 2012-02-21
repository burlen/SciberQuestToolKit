#!/bin/bash

if [[ $# -lt 2 ]]
then
  echo "usage:"
  echo "passthrough.sh /path/to/input /path/to/output"
  exit -1
fi

INPUT=$1
OUTPUT=$2

gimp -i -b "(passthrough \"$INPUT\" \"$OUTPUT\")" -b "(gimp-quit 0)"

