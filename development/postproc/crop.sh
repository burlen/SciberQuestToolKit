#!/bin/bash
#
# crop INFILE OUTFILE
#  
if [ $#  != 2 ] ; then
  echo "Usage: $0 infile outfile"
  exit 1
fi

gimp -i -b "(sciber-crop \"$1\" \"$2\" )" -b '(gimp-quit 0)'
