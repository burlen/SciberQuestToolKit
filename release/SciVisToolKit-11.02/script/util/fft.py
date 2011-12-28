
import pylab as mpl
import numpy as np

f=open('/home/bloring/ext/ParaView/SciVisToolKit/data/asym-2d/uex_292500.gda','rb')
uex=np.fromfile(file=f, dtype=np.float32).reshape(5118,1279)

fuex=np.fft.fftn(uex)
pfuex=np.abs(np.fft.fftshift(fuex))**2

f=open('/home/bloring/ext/ParaView/SciVisToolKit/data/asym-2d/fuex_292500.gda','wb')
f.write(fuex.tostring())
f.close()

_pfuex=pfuex.astype('float32')
f=open('/home/bloring/ext/ParaView/SciVisToolKit/data/asym-2d/pfuex_292500.gda','wb')
f.write(_pfuex.tostring())
f.close()



