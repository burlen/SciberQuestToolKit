import sys
import os
import imp
from math import sqrt

# we expect a config file that is passed by shell environment variable.
configFileName = os.getenv('SQ_DRIVER_CONFIG')

if (configFileName is None):
  print 'Usage:'
  print 'export SQ_DRIVER_CONFIG=/path/to/config.py'
  print '%s'%(sys.argv[0])

config = imp.load_source('module.name',configFileName)

try: paraview.simple
except: from paraview.simple import *
paraview.simple._DisableFirstRenderCameraReset()

# record the run parameters
print 'config.outputBaseFileName'
print config.outputBaseFileName
print 'config.outputWidth'
print config.outputWidth
print 'config.inputFileName'
print config.inputFileName
print 'config.arraysToRead'
print config.arraysToRead
print 'config.startTimeStep'
print config.startTimeStep  
print 'config.endTimeStep'
print config.endTimeStep    
print 'config.subset'
print config.iSubset
print config.jSubset
print config.kSubset
print 'config.useSmoothing'
print config.useSmoothing
print 'config.smoothingWidth'
print config.smoothingWidth
print 'config.computeVorticity'
print config.computeVorticity
print 'config.computeHelicity'
print config.computeHelicity
print 'config.computeNHelicity'
print config.computeNHelicity
print 'config.computeLambda2'
print config.computeLambda2
print 'config.vorticityInputArray'
print config.vorticityInputArray
print 'config.computeLIC'
print config.computeLIC
print 'config.LICField'
print config.LICField
print 'config.LICSteps'
print config.LICSteps
print 'config.LICStepSize'
print config.LICStepSize
print 'config.LICAlpha'
print config.LICAlpha
print 'config.colorByArray'
print config.colorByArray
print 'config.colorByArrayComps'
print config.colorByArrayComps
print 'config.lutMode'
print config.lutMode
print 'config.pvPath'
print config.pvPath
print 'config.camFac'
print config.camFac

# load the pugins
licPath = '%s/libSurfaceLIC.so'%(config.pvPath)
LoadPlugin(licPath,False,globals())
LoadPlugin(licPath,True,globals())

svtkPath = '%s/libSciVisToolKit.so'%(config.pvPath)
LoadPlugin(svtkPath,False,globals())
LoadPlugin(svtkPath,True,globals())

# read the dataset
bovr         = SQBOVReader(FileName=config.inputFileName)

iExtent = bovr.GetProperty('ISubsetInfo')
jExtent = bovr.GetProperty('JSubsetInfo')
kExtent = bovr.GetProperty('KSubsetInfo')

print 'whole extent'
print iExtent 
print jExtent 
print kExtent 

if (config.iSubset[0] < 0):
  config.iSubset[0] = iExtent[0]
if (config.iSubset[1] < 0):
  config.iSubset[1] = iExtent[1]

if (config.jSubset[0] < 0):
  config.jSubset[0] = jExtent[0]
if (config.jSubset[1] < 0):
  config.jSubset[1] = jExtent[1]

if (config.kSubset[0] < 0):
  config.kSubset[0] = kExtent[0]
if (config.kSubset[1] < 0):
  config.kSubset[1] = kExtent[1]

bovr.ISubset = config.iSubset 
bovr.JSubset = config.jSubset
bovr.KSubset = config.kSubset
bovr.Arrays  = config.arraysToRead

rep = Show(bovr)
rep.Representation = 'Outline'

# run the pipeline here to get the bounds
Render()

nSteps = 0
steps = bovr.TimestepValues
try:
  nSteps = len(steps)
except:
  nSteps = 1
  steps = [steps]
 
print "steps"
print steps
print "nStep"
print nSteps

bounds = bovr.GetDataInformation().GetBounds()
bounds_dx = bounds[1] - bounds[0]
bounds_dy = bounds[3] - bounds[2]
bounds_dz = bounds[5] - bounds[4]
bounds_cx = (bounds[0] + bounds[1])/2.0
bounds_cy = (bounds[2] + bounds[3])/2.0
bounds_cz = (bounds[4] + bounds[5])/2.0

if (bounds_dx == 0):
  # yz
  dimMode = 2
  aspect = bounds_dz/bounds_dy

elif (bounds_dy == 0):
  # xz
  dimMode = 1
  aspect = bounds_dz/bounds_dx

elif (bounds_dz == 0):
  #xy
  dimMode = 0
  aspect = bounds_dy/bounds_dx

else:
  #3d
  dimMode = 3
  aspect = 1.0 # TODO


print 'extent'
print config.iSubset
print config.jSubset
print config.kSubset
print 'bounds'
print bounds
print 'dx'
print (bounds_dx, bounds_dy, bounds_dz)
print 'cx'
print (bounds_cx, bounds_cy, bounds_cz)
print 'dimMode'
print dimMode


lastObj = bovr

ghosts1 = None
conv = None
ghosts2 = None
vortex = None

# use smoothing
if (config.useSmoothing):
  ghosts1 = SQImageGhosts()

  rep = Show()
  rep.Representation = 'Outline'

  conv = SQKernelConvolution()
  conv.Width = config.smoothingWidth

  lastObj = conv

# compute vorticity
if (config.computeVorticity or config.computeHelicity or config.computeNHelicity or config.computeLambda2):
  ghosts2 = SQImageGhosts()

  vortex = SQVortexDetect()
  vortex.VectorField = ['POINTS', config.vorticityInputArray]
  vortex.Rotation = config.computeVorticity
  vortex.Normalizedhelicity = config.computeNHelicity
  vortex.Helicity = config.computeHelicity
  vortex.Lambda2 = config.computeLambda2

  lastObj = vortex

# loop over requested step range
step = 0
if (config.startTimeStep >= 0):
  step = config.startTimeStep
#else:
#  step = nSteps-1

endStep = step
if (config.endTimeStep >= 0):
  endStep = min(config.endTimeStep, nSteps-1)
else:
  endStep = nSteps-1

print 'step'
print step
print 'endStep'
print endStep

while (step <= endStep):

  print '====================================='
  print 'step'
  print step
  print 'time'
  print steps[step]

  # run the pipeline to get array information 
  anim = GetAnimationScene()
  anim.PlayMode = 'Snap To TimeSteps'
  anim.AnimationTime = steps[step]
  
  view = GetRenderView()
  view.ViewTime = steps[step]
  view.UseOffscreenRenderingForScreenshots = 0
  
  rep = Show(lastObj)
  rep.Representation = 'Outline'
  Render()
  
  print 'arrays'
  nArrays = lastObj.PointData.GetNumberOfArrays()
  i = 0
  while (i<nArrays):
    print lastObj.PointData.GetArray(i).Name
    i = i + 1
  
  # set the lut
  colorByArrayRange = [0.0, 1.0]
  if (config.colorByArrayComps == 1):
    colorByArrayRange = lastObj.PointData.GetArray(config.colorByArray).GetRange()
  else:
    # TODO - this could be larger than the range of the magnitude array
    rx = lastObj.PointData.GetArray(config.colorByArray).GetRange(0)
    ry = lastObj.PointData.GetArray(config.colorByArray).GetRange(1)
    rz = lastObj.PointData.GetArray(config.colorByArray).GetRange(2)
    colorByArrayRange = [0.0,
         sqrt(rx[1]*rx[1]+ry[1]*ry[1]+rz[1]*rz[1])]  
    #colorByArrayRange = [sqrt(rx[0]*rx[0]+ry[0]*ry[0]+rz[0]*rz[0]),
    #     sqrt(rx[1]*rx[1]+ry[1]*ry[1]+rz[1]*rz[1])]  
  
  print 'config.colorByArray'
  print config.colorByArray
  print 'lut range'
  print colorByArrayRange
  
  if (config.lutMode == 0):
    lut = GetLookupTableForArray(config.colorByArray,
               config.colorByArrayComps,
               RGBPoints=[colorByArrayRange[0], 0.0, 0.0, 1.0, colorByArrayRange[1], 1.0, 0.0, 0.0],
               ColorSpace='Diverging',
               VectorMode='Magnitude',
               ScalarRangeInitialized=1.0)
  
  elif (config.lutMode == 1):
    lut = GetLookupTableForArray(config.colorByArray,
              config.colorByArrayComps,
              RGBPoints=[colorByArrayRange[0], 0.0, 0.0, 1.0, colorByArrayRange[1], 1.0, 0.0, 0.0],
              ColorSpace='HSV',
              VectorMode='Magnitude',
              ScalarRangeInitialized=1.0)
  
  rep = Show(lastObj)
  if (config.computeLIC):
    rep.Representation = 'Surface LIC'
    rep.SelectLICVectors = ['POINTS', config.LICField]
    rep.LICIntensity = config.LICAlpha
    rep.LICStepSize = config.LICStepSize
    rep.LICNumberOfSteps = config.LICSteps
    rep.InterpolateScalarsBeforeMapping = 0
  else:
    rep.Representation = 'Slice'
  rep.ColorArrayName = config.colorByArray
  rep.LookupTable = lut
  
  # position the camera
  
  far  = config.camFac
  near = 0
  
  if (dimMode == 0):
    # xy
    pos = max(bounds_dx, bounds_dy)
    camUp = [0.0, 1.0, 0.0]
    camPos = [bounds_cx, bounds_cy,  pos*far]
    camFoc = [bounds_cx, bounds_cy, -pos*near]
  
  elif (dimMode == 1):
    # xz
    pos = max(bounds_dx, bounds_dz)
    camUp = [0.0, 0.0, 1.0]
    camPos = [bounds_cx, -pos*far,  bounds_cz]
    camFoc = [bounds_cx,  pos*near, bounds_cz]
  
  elif (dimMode == 2):
    # yz
    pos = max(bounds_dy, bounds_dz)
    camUp = [0.0, 0.0, 1.0]
    camPos = [ pos*far, bounds_cy, bounds_cz]
    camFoc = [-pos*near, bounds_cy, bounds_cz]
  
  else:
    # 3d
    print '3d cam position is yet TODO'
  
  view = GetRenderView()
  view.CameraViewUp = camUp
  view.CameraPosition = camPos
  view.CameraFocalPoint = camFoc
  view.UseOffscreenRenderingForScreenshots = 0
  view.CenterAxesVisibility = 0
  
  print 'camUp'
  print camUp
  print 'camPos'
  print camPos
  print 'camFoc'
  print camFoc
  
  # take input objs out of the scene
  if (ghosts1 is not None):
    Hide(ghosts1)
  
  if ((conv is not None) and (conv != lastObj)):
    Hide(conv)
  
  if (ghosts2 is not None):
    Hide(ghosts2)
  
  if ((vortex is not None) and (vortex != lastObj)):
    Hide(vortex)
  
  if ((conv is not None) or (vortex is not None)):
    Hide(bovr)
  
  ren = Render()
  
  width = int(config.outputWidth)
  height = int(config.outputWidth*aspect)
  
  ren.ViewSize = [width, height] 
  
  outputFileName = '%s%06d.png'%(config.outputBaseFileName, step)
  WriteImage(outputFileName)
  print 'output file'
  print outputFileName
  print 'width'
  print width
  print 'height'
  print height

  step = step + 1


