#!/bin/bash


for f in `grep "$1" ./ -rInl --exclude-dir=.svn`;
do
  echo -n "Replace $1 in $f ?";
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
