#!/usr/bin/python
# -*- coding: utf-8 -*-
from matplotlib import pylab as mpl
import matplotlib as pl
from numpy import *
from scipy.interpolate import *

#def cmap_discretize(cmap, N):
  #"""Return a discrete colormap from the continuous colormap cmap.

      #cmap: colormap instance, eg. cm.jet.
      #N: Number of colors.

  #Example
      #x = resize(arange(100), (5,100))
      #djet = cmap_discretize(cm.jet, 5)
      #imshow(x, cmap=djet)
  #"""
  #cdict = cmap._segmentdata.copy()
  ## N colors
  #colors_i = linspace(0,1.,N)
  ## N+1 indices
  #indices = linspace(0,1.,N+1)
  #for key in ('red','green','blue'):
    ## Find the N colors
    #D = array(cdict[key])
    #I = interpolate.interp1d(D[:,0], D[:,1])
    #colors = I(colors_i)
    ## Place these colors at the correct indices.
    #A = zeros((N+1,3), float)
    #A[:,0] = indices
    #A[1:,1] = colors
    #A[:-1,2] = colors
    #print A[:,1]
    ## Create a tuple for the dictionary.
    ##L = []
    ##for l in A:
    ##    L.append(tuple(l))
    ##cdict[key] = tuple(L)
  ## Return colormap object.
  #return #matplotlib.colors.LinearSegmentedColormap('colormap',cdict,1024)

def write_cmap(cmap, N, name):
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
  x0=-1.0
  x1=1.0
  dx=(x1-x0)/(n-1.0)
  r=lut[0]
  g=lut[1]
  b=lut[2]
  f_lut=open('%s.xml'%(name),'w')
  f_lut.write('<ColorMap name="%s" space="RGB">\n'%(name))
  i=0
  while(i<n):
    x=x0+i*dx
    f_lut.write('  <Point x="%f"  o="1" r="%f" g="%f" b="%f"/>\n'%(x,r[i],g[i],b[i]))
    i+=1
  f_lut.write('</ColorMap>\n')
  f_lut.close()
  return

f_names=open('mpl_lut_names.txt')
names=f_names.readlines()
f_names.close()
for name in names:
  name=name.split()[0]
  print 'name=\"%s\"'%(name)
  m=mpl.get_cmap(name)
  print m
  write_cmap(m,256,name)





