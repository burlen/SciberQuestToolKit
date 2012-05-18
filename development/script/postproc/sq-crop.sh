#!/bin/bash
#   ____    _ __           ____               __    ____
#  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
# _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
#/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)
#
#Copyright 2010 SciberQuest Inc.

if [ $# != 6 ] ; then
  echo "Usage: $0 infile outfile wifth height offx offy"
  exit 1
fi

gimp -i -b "(sq-crop \"$1\" \"$2\" \"$3\" \"$4\" \"$5\" \"$6\")" -b '(gimp-quit 0)'
