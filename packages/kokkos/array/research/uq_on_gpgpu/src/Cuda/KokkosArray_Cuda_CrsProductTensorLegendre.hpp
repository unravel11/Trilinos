/*
//@HEADER
// ************************************************************************
// 
//    KokkosArray: Manycore Performance-Portable Multidimensional Arrays
//              Copyright (2012) Sandia Corporation
// 
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact H. Carter Edwards (hcedwar@sandia.gov)
// 
// ************************************************************************
//@HEADER
*/

#ifndef KOKKOSARRAY_CUDA_CRSPRODUCTTENSORLEGENDRE_HPP
#define KOKKOSARRAY_CUDA_CRSPRODUCTTENSORLEGENDRE_HPP

#include <utility>
#include <sstream>
#include <stdexcept>
#include <KokkosArray_BlockCrsMatrix.hpp>
#include <KokkosArray_CrsProductTensorLegendre.hpp>
#include <Cuda/KokkosArray_Cuda_Parallel.hpp>
#include <Cuda/KokkosArray_Cuda_Texture.hpp>

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

KokkosArray::Impl::CudaTexture<double> 
  multiply_crs_product_tensor_A ,
  multiply_crs_product_tensor_X ;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace KokkosArray {
namespace Impl {

template< typename MatrixScalar ,
          typename VectorScalar >
class Multiply<
  BlockCrsMatrix< CrsProductTensorLegendre< MatrixScalar , Cuda > ,
                  MatrixScalar , Cuda > ,
  View< VectorScalar** , LayoutLeft , Cuda > ,
  View< VectorScalar** , LayoutLeft , Cuda > >
{
public:

  typedef Cuda                    device_type ;
  typedef device_type::size_type  size_type ;

  typedef CrsProductTensorLegendre< MatrixScalar , device_type >    tensor_type ;
  typedef BlockCrsMatrix< tensor_type, MatrixScalar, device_type >  matrix_type ;
  typedef View< VectorScalar** , LayoutLeft , Cuda >                vector_type ;

private:

  const matrix_type m_A ;
  const vector_type m_x ;
  const vector_type m_y ;

public:

  __device__
  void operator()(void) const
  {
    // Number of bases in the stochastic system and padding to warp size.
    const size_type tensor_dim = m_A.block.dimension();
    const size_type tensor_dim_align = tensor_dim ;

    // Shared memory:
    //   sh_work[ CudaTraits::WarpSize * blockDim.y ]
    //   sh_y[ tensor_dim_align ]
    //   sh_x[ tensor_dim_align ]
    //   sh_A[ tensor_dim_align ]
    //   sh_offset[ 2 * tensor_dim_align + 1 ]

    volatile VectorScalar * const sh_work = kokkos_impl_cuda_shared_memory<VectorScalar>();
    VectorScalar * const sh_y = (VectorScalar *)( sh_work + blockDim.x * blockDim.y );
    VectorScalar * const sh_x = (VectorScalar *)( sh_y + tensor_dim_align );
    MatrixScalar * const sh_A = (MatrixScalar *)( sh_x + tensor_dim_align );
    unsigned     * const sh_offset = (unsigned *)( sh_A + tensor_dim_align );

    // Block size and thread id within the entire block:

    const size_type nid = CudaTraits::WarpSize * blockDim.y ;
    const size_type tid = threadIdx.x + CudaTraits::WarpSize * threadIdx.y ;

    { // Load the tensor offsets into shared memory
      const size_type n = 2 * tensor_dim + 1 ;
      for ( size_type i = tid ; i < n ; ++i ) {
        sh_offset[i] = m_A.block.m_entry_offset(i);
      }
    }

    // Loop over columns in the discrete (finite element) system.

    // blockIdx.x == row in the deterministic (finite element) system
    const size_type iBlockEntryEnd = m_A.graph.row_map[ blockIdx.x + 1 ];
          size_type iBlockEntry    = m_A.graph.row_map[ blockIdx.x ];

    for ( ; iBlockEntry < iBlockEntryEnd ; ++iBlockEntry ) {

      { // Read stochastic coefficients from the vector and matrix into shared memory.
        const size_type iBlockColumn = m_A.graph.entries( iBlockEntry );

        const VectorScalar * const x = & m_x(        0 , iBlockColumn );
        const MatrixScalar * const A = & m_A.values( 0 , iBlockEntry );

        // Wait for X and A to be used in the previous iteration before reading new values.
        __syncthreads();

        // Coalesced read by the whole block from global memory:
        for ( size_type i = tid ; i < tensor_dim ; i += nid ) {
          sh_x[i] = x[i] ; // m_x(        i , iBlockColumn );
          sh_A[i] = A[i] ; // m_A.values( i , iBlockEntry );
        }

        __syncthreads();
        // Wait for X and A to be read before using these values in the next iteration.
      }

      // Each warp computes a row of the stochastic block 
      // for coalesced reads and no need for explicit synchronization.

      for ( size_type iyInner = threadIdx.y ; iyInner < tensor_dim ; iyInner += blockDim.y ) {

        VectorScalar y = 0 ;

        // Product tensor entries which this warp will iterate:
        //
        const size_type iBeg        = sh_offset[ 2 * iyInner ];
        const size_type iBegOffDiag = sh_offset[ 2 * iyInner + 1 ];
        const size_type iEnd        = sh_offset[ 2 * iyInner + 2 ];

        size_type i = iBeg + threadIdx.x ;

        // Loop through sparse tensor diagonal contributions:

        for ( ; i < iBegOffDiag ; i += blockDim.x ) {
          const unsigned j = m_A.block.m_coordinate(i);
          y += m_A.block.m_value(i) * sh_A[j] * sh_x[j] ;
        }

        // Loop through sparse tensor off-diagonal contributions:

        for ( ; i < iEnd ; i += blockDim.x ) {
          const unsigned kj = m_A.block.m_coordinate(i);
          const unsigned j  = kj & 0x0ffff ;
          const unsigned k  = kj >> 16 ;
          y += m_A.block.m_value(i) * ( sh_A[j] * sh_x[k] + sh_A[k] * sh_x[j] );
        }

        // Reduction of 'y' within the warp

        sh_work[ tid ] = y ;

        if ( threadIdx.x + 16 < CudaTraits::WarpSize ) sh_work[tid] += sh_work[tid+16];
        if ( threadIdx.x +  8 < CudaTraits::WarpSize ) sh_work[tid] += sh_work[tid+ 8];
        if ( threadIdx.x +  4 < CudaTraits::WarpSize ) sh_work[tid] += sh_work[tid+ 4];
        if ( threadIdx.x +  2 < CudaTraits::WarpSize ) sh_work[tid] += sh_work[tid+ 2];
        if ( threadIdx.x +  1 < CudaTraits::WarpSize ) sh_work[tid] += sh_work[tid+ 1];

        if ( 0 == threadIdx.x ) { // One thread in the warp saves to shared memory
          sh_y[ iyInner ] = sh_work[ tid ];
        }
      }

      // Coalesced write by the whold block to global memory
      for ( size_type i = tid ; i < tensor_dim ; i += nid ) {
        m_y( i , blockIdx.x ) = sh_y[i];
      }
    }
  }

  //------------------------------------

  Multiply( const matrix_type & A ,
            const vector_type & x ,
            const vector_type & y )
  : m_A( A ), m_x( x ), m_y( y )
  {
    const size_type row_count        = A.graph.row_map.dimension(0) - 1 ;
    const size_type tensor_dimension = A.block.dimension();
    const size_type tensor_dim_align = tensor_dimension ;

    // Shared memory:
    //   sh_work[ CudaTraits::WarpSize * blockDim.y ]
    //   sh_y[ tensor_dim_align ]
    //   sh_x[ tensor_dim_align ]
    //   sh_A[ tensor_dim_align ]
    //   sh_offset[ 2 * tensor_dim_align + 1 ]

    const int nsh_warp = sizeof(VectorScalar) * CudaTraits::WarpSize ; // sh_work

    const int nsh_base = sizeof(VectorScalar) * tensor_dim_align             // sh_y
                       + sizeof(VectorScalar) * tensor_dim_align             // sh_x
                       + sizeof(MatrixScalar) * tensor_dim_align             // sh_A
                       + sizeof(unsigned int) * ( 2 * tensor_dim_align + 1 ) // sh_offset
                       ;

    enum { ShCapacity = KokkosArray::Impl::CudaTraits::SharedMemoryCapacity };

    int nWarp = ( ShCapacity - nsh_base ) / nsh_warp ;

    if ( nWarp < 1 ) {
      std::ostringstream msg ;
      msg << "Multiply< BlockCrsMatrix< CrsProductTensor ... > >::apply "
          << " FAILED to meet shared memory need: "
          << nsh_base << " + " << nsh_warp
          << " * nWarp <= " << ShCapacity ;
      throw std::runtime_error( msg.str() );
    }

    nWarp = std::min( nWarp , (int) cuda_internal_maximum_warp_count() );
    nWarp = std::min( nWarp , (int) tensor_dimension );

    const size_type shmem = nsh_base + nsh_warp * nWarp ;

    const dim3 dBlock( CudaTraits::WarpSize , nWarp , 1 );

    const dim3 dGrid( row_count , 1 , 1 );

#if 0

    std::cout << "Multiply< BlockCrsMatrix< CrsProductTensorLegendre ... > >"
              << std::endl 
              << "  grid(" << dGrid.x << "," << dGrid.y << ")" << std::endl
              << "  block(" << dBlock.x << "," << dBlock.y << ")" << std::endl
              << "  shmem(" << shmem << ")" << std::endl
              << "  row_count(" << row_count << ")" << std::endl
              << "  tensor_dimension(" << tensor_dimension << ")" << std::endl
              << "  tensor_dim_align(" << tensor_dim_align << ")" << std::endl
              ;
#endif

    cuda_parallel_launch_local_memory< Multiply ><<< dGrid , dBlock , shmem >>>( *this );
    // Impl::CudaParallelLaunch< Multiply >( *this, dGrid , dBlock , shmem );
  }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

} // namespace Impl
} // namespace KokkosArray

#endif /* #ifndef KOKKOSARRAY_CUDA_CRSPRODUCTTENSORLEGENDRE_HPP */

