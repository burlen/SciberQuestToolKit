#!/usr/bin/python

#import pylab as mpl
import numpy as np

#fileName='/home/bloring/ext/ParaView/SciberQuestToolKit/data/asym-2d/uex_292500.gda'
dataPath='/home/bloring/ext/ParaView/SciberQuestToolKit/data/asym-2d-1024'
inputFile=dataPath+'/uex_292500.gda'
outputFilePower=dataPath+'/puex_292500.gda'
outputFileFilter=dataPath+'/fuex_292500.gda'
outputFilePFilter=dataPath+'/pfuex_292500.gda'

print inputFile
#print outputFile

nx=1024
ny=1024
cx=nx/2
w=640

f=open(inputFile,'rb')
data=np.fromfile(file=f, dtype=np.float32).reshape(nx,ny)
f.close()
print 'read data %s'%(inputFile)
print data.shape

fdata=np.fft.fftn(data)

pdata=np.log(np.abs(np.fft.fftshift(fdata))**2)
_pdata=pdata.astype('float32')
f=open(outputFilePower,'wb')
f.write(_pdata.tostring())
f.close()
print 'wrote power spectrum of data %s'%(outputFilePower)

i=0
nxy=nx*ny
_fdata=np.reshape(fdata,nxy)
while (i<nxy):
  v=_fdata[i]

  q=int(i)/int(nx)
  p=int(i)-int(q)*int(nx)
  r=np.sqrt((p-cx)**2+(q-cx)**2)

  if (r<=w):
    _fdata[i]=0.0;

#  if ((q<6)or(q>1017)):
#      _fdata[i]=v;
#
#  if ((p<6)or(p>1017)):
#      _fdata[i]=v;

#  if (((q<24)or(q>1000))and((p<536)and(p>488))):
#    _fdata[i]=0.0;
#
#  if (((p<24)or(p>1000))and((q<536)and(q>488))):
#    _fdata[i]=0.0;

  i+=1

ffdata=np.reshape(_fdata,(nx,ny))
iffdata=np.fft.ifftn(ffdata)
_iffdata=iffdata.real.astype('float32')
f=open(outputFileFilter,'wb')
f.write(_iffdata.tostring())
f.close()
print 'wrote filtered data %s'%(outputFileFilter)

pfdata=np.log(np.abs(np.fft.fftshift(ffdata))**2)
_pfdata=pfdata.astype('float32')
f=open(outputFilePFilter,'wb')
f.write(_pfdata.tostring())
f.close()
print 'wrote power spectrum of filtered data %s'%(outputFilePFilter)

#pfdata=np.abs(fdata)**2


#f=open('/home/bloring/ext/ParaView/SciberQuestToolKit/data/asym-2d/pfuex_292500.gda','wb')
#f.write(fdata.tostring())
#f.close()




