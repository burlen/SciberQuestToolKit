#!/bin/bash

if [[ $# < 2 ]]
then
  echo
  echo "usage:"
  echo "pvd.sh /path/to/name.pvd vtk-file-ext(eg.pvtp)"
  echo
  exit 1
fi

FNAME=$1
PEXT=$2
TIME=0

echo "FNAME=$FNAME"
echo "PEXT=$PEXT"

echo "<?xml version=\"1.0\"?>" > $FNAME
echo "<VTKFile type=\"Collection\">" >> $FNAME
echo "<Collection>" >> $FNAME

for f in `find ./ -name '*.'$PEXT | sort`
do
  t=`echo $f | cut -d/ -f2`;
  t=`printf "%0.f\n" $t`;

  echo "  <DataSet timestep=\"$t\" group=\"\" part=\"0\" file=\"$f\"/>" >> $FNAME
  let TIME=$TIME+1
done

echo "</Collection>" >> $FNAME
echo "</VTKFile>" >> $FNAME

exit 0
