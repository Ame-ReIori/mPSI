if(NOT DEFINED MPSI_STD_VER)
  set(MPSI_STD_VER 20)
endif()

set(CMAKE_CXX_FLAGS "-maes -msse2 -msse3 -msse4.1 -mpclmul -mavx -mavx2 -march=native -fopenmp -lpthread")
