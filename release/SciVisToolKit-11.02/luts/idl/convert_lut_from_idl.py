#!/usr/bin/python
# -*- coding: utf-8 -*-

f_dat=open('idl_luts.txt')
dat=f_dat.readlines()
f_dat.close()

f_names=open('idl_lut_names.txt')
names=f_names.readlines()
f_names.close()

q=0
for name in names:
  name=name.split()[0]
  lut=[[],[],[]]
  j=0
  while(j<3):
    i=0
    while(i<14):
      print q
      vals=dat[q].split()
      for val in vals:
        lut[j].append(float(val)/255.0)
      i+=1
      q+=1
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
