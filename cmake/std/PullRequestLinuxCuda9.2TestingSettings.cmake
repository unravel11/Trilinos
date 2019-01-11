# This file contains the options needed to both run the pull request testing
# for Trilinos for the CUDA 9.2 pull request testing builds, and to reproduce
# the errors reported by those builds. Prior to using this this file, the
# appropriate set of modules must be loaded and path must be augmented.
# (See the sems/PullRequestCuda9.2TestingEnv.sh files.)

# Usage: cmake -C PullRequestLinuxCUDA9.2TestingSettings.cmake

# Misc options typically added by CI testing mode in TriBITS

# Use the below option only when submitting to the dashboard
set (CTEST_USE_LAUNCHERS ON CACHE BOOL "Set by default for PR testing")

# Options necessary for CUDA build
set (TPL_ENABLE_MPI ON CACHE BOOL "Set by default for CUDA PR testing")
set (TPL_ENABLE_CUDA ON CACHE BOOL "Set by default for CUDA PR testing")
set (Kokkos_ENABLE_Cuda ON CACHE BOOL "Set by default for CUDA PR testing")
set (Kokkos_ENABLE_Cuda_UVM ON CACHE BOOL "Set by default for CUDA PR testing")
set (KOKKOS_ARCH Power8 CACHE STRING "Set by default for CUDA PR testing")

# TPL settings specific to CUDA build
set (TPL_BLAS_LIBRARIES "-L${BLAS_ROOT}/lib -lblas -lgfortran -lgomp -lm" CACHE STRING "Set by default for CUDA PR testing")
set (TPL_LAPACK_LIBRARIES "-L${LAPACK_ROOT}/lib -llapack -lgfortran -lgomp" CACHE STRING "Set by default for CUDA PR testing")
set (TPL_ENABLE_Scotch OFF CACHE BOOL "Set by default for CUDA PR testing")
#Parmetis is available on ride and could be enabled for the CUDA PR build
set (TPL_ENABLE_ParMETIS OFF CACHE BOOL "Set by default for CUDA PR testing")
include("${CMAKE_CURRENT_LIST_DIR}/PullRequestLinuxCommonTestingSettings.cmake")

