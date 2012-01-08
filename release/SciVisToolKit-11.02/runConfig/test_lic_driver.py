##########################################################################
# Run Parameters
#

# output file prefix, including path, ends in _
outputBaseFileName  = '/home/01237/bloring/test_sq_lic_driver'

# image width in pixels
outputWidth         = 1024

# controls which arrays of the specified dataset are read.
#inputFileName         = '/work2/data/2d-asym/asym2d.bov'# '/work/ext/H3D-Data/rankine_xy/rankine.bov' 
inputFileName       = '/scratch/01237/bloring/2d-asym/asym2d.bov'
arraysToRead        = ['ui']
# controls the time range, if the start and end time are < 0 the last step
# is used
startTimeStep       = 133
endTimeStep         = 133

# controls the subset read in, if any value is < 0 the extent on disk is used
iSubset             = [-1, -1]
jSubset             = [-1, -1]
kSubset             = [-1, -1]

# write data
writeData           = False

# controls if Gaussian smoothing is applied and if so what stencil width
# is used (must greater or equal to 3)
useSmoothing        = False
smoothingWidth      = '19'

# controls what vortex detection techniques to use on what input array
# a 0 means don't compute while  1 means compute
vorticityInputArray      = 'ui'
vorticityPassInput       = 1 # deep copy input array
vorticitySplitComponents = 0 # results are split into component arrays
computeVorticity         = 0 # result is named rot-XX
computeHelicity          = 0 # result is named hel-XX
computeNHelicity         = 0 # result is named norm-hel-XX
computeLambda2           = 0 # result is named lam2-XX
computeDivergence        = 1 # result is named div-XX

# controls if LIC is computed , various parameters, and which array is
# used
computeLIC          = False
LICField            = 'ui'
LICSteps            = 15
LICStepSize         = 1.0
LICAlpha            = 0.75

# set the array to color by, and 1(scalar) or 3(vector)
colorByArray        = 'ui'
colorByArrayComps   = 1

# controls which LUT is used. 0=blue-to-red diverging, 1=blue to red HSV, 2=black body, 3=gray scale
lutMode             = 3
rescaleLut          = 0
lutRange            = [0.0, 0.02]

# controls the zoom of the camera. Must be > 0 and values > 1 zoom out.
camFac              = 1.0

# path to ParaView install
pvPath              = '/home/01237/bloring/apps/PV3-3.10.0-icc-ompi-R/lib/paraview-3.10'
##########################################################################


