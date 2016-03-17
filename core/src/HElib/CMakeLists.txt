include_directories(../../include/)
include_directories(../../include/HElib)
set(LIB_SRC KeySwitching.cpp EncryptedArray.cpp FHE.cpp Ctxt.cpp CModulus.cpp
        FHEContext.cpp PAlgebra.cpp DoubleCRT.cpp NumbTh.cpp bluestein.cpp
        IndexSet.cpp timing.cpp replicate.cpp hypercube.cpp matching.cpp
        powerful.cpp BenesNetwork.cpp permutations.cpp PermNetwork.cpp
        OptimizePermutations.cpp eqtesting.cpp polyEval.cpp extractDigits.cpp
        EvalMap.cpp OldEvalMap.cpp recryption.cpp debugging.cpp matrix.cpp)
add_library(fhe STATIC ${LIB_SRC})
