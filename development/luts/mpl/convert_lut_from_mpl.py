#!/usr/bin/python
# -*- coding: utf-8 -*-
from matplotlib import pylab as mpl
import matplotlib as pl
from numpy import *
from scipy.interpolate import *

def write_cmap(cmap, N, name, f_all):
  cdict = cmap._segmentdata.copy()
  # N colors
  colors_i = linspace(0,1.,N)
  # N+1 indices
  indices = linspace(0,1.,N+1)

  lut=[[],[],[]]

  j=0
  for key in ('red','green','blue'):
    # Find the N colors
    D = array(cdict[key])
    I = interpolate.interp1d(D[:,0], D[:,1])
    colors = I(colors_i)
    lut[j]=colors
    j+=1

  n=len(lut[0])
  x0=0.0
  x1=1.0
  dx=(x1-x0)/(n-1.0)
  r=lut[0]
  g=lut[1]
  b=lut[2]
  f_lut=open('%s.xml'%(name),'w')
  f_lut.write('<ColorMap name="%s" space="RGB">\n'%(name))
  f_all.write('<ColorMap name="%s" space="RGB">\n'%(name))
  i=0
  while(i<n):
    x=x0+i*dx
    f_lut.write('  <Point x="%f"  o="%f" r="%f" g="%f" b="%f"/>\n'%(x,x,r[i],g[i],b[i]))
    f_all.write('  <Point x="%f"  o="%f" r="%f" g="%f" b="%f"/>\n'%(x,x,r[i],g[i],b[i]))
    i+=1
  f_lut.write('</ColorMap>\n')
  f_all.write('</ColorMap>\n')
  f_lut.close()
  return

f_names=open('mpl_lut_names.txt')
names=f_names.readlines()
f_names.close()

f_all=open('all_mpl_cmaps.xml','w')
f_all.write('<doc>\n')

for name in names:
  name=name.split()[0]
  print 'name=\"%s\"'%(name)
  m=mpl.get_cmap(name)
  write_cmap(m,256,name,f_all)

f_all.write('</doc>\n')
f_all.close();


