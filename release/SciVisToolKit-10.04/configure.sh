ccmake ../SciVisToolKit \
-DCMAKE_C_COMPILER=/nasa/intel/Compiler/11.0/083/bin/intel64/icc \
-DCMAKE_CXX_COMPILER=/nasa/intel/Compiler/11.0/083/bin/intel64/icpc \
-DCMAKE_LINKER=/nasa/intel/Compiler/11.0/083/bin/intel64/icpc \
-DBUILD_GENTP=ON \
-DMPI_INCLUDE_PATH=/nasa/sgi/mpt/1.25/include \
-DMPI_LIBRARY=/nasa/sgi/mpt/1.25/lib/libmpi.so \
-DParaView_DIR=/u/hkarimab/ParaView/PV3-3.7-2010-04-02/ \
-DEigen_DIR=/u/hkarimab/apps/eigen-2.0.12/
