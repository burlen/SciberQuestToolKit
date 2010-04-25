#!/bin/bash

if [ -z "$1" ]
then
  echo "Error \$1 is not set to a search term."
  exit
fi

if [ -z "$2" ]
then
  echo "Error \$2 is not set to a replace term."
  exit
fi


for f in `grep "$1" ./ -rInl --exclude-dir=.svn`;
do
  echo -n "Replace $1 with $2 in $f (y/n)?";
  read PMT; 
  case "$PMT" in 
    "y" )
      sed -i "s/$1/$2/g" $f
      ;;

    * )
      echo  skip;
       ;; 
  esac;
done


