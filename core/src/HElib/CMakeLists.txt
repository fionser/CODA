include_directories(../../include/)
include_directories(../../include/HElib)
include_directories(${NTL_INCLUDE_PATH})
add_definitions(-DNO_HALF_SIZE_PRIME)
add_definitions(-DFHE_THREADS)
set(LIB_SRC KeySwitching.cpp EncryptedArray.cpp FHE.cpp Ctxt.cpp CModulus.cpp
        FHEContext.cpp PAlgebra.cpp DoubleCRT.cpp NumbTh.cpp bluestein.cpp
        IndexSet.cpp timing.cpp replicate.cpp hypercube.cpp matching.cpp
        powerful.cpp BenesNetwork.cpp permutations.cpp PermNetwork.cpp
        OptimizePermutations.cpp eqtesting.cpp polyEval.cpp extractDigits.cpp
        EvalMap.cpp OldEvalMap.cpp recryption.cpp debugging.cpp matrix.cpp)
add_library(fhe STATIC ${LIB_SRC})
