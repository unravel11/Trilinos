/*
// @HEADER
// ***********************************************************************
//
//          Tpetra: Templated Linear Algebra Services Package
//                 Copyright (2008) Sandia Corporation
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
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ************************************************************************
// @HEADER
*/

#include "Tpetra_TestingUtilities.hpp"
#include "Tpetra_Util.hpp"
#include "Kokkos_View.hpp"
#include <vector>

namespace { // (anonymous)
  using std::endl;

  //
  // UNIT TESTS
  //

  TEUCHOS_UNIT_TEST_TEMPLATE_1_DECL( CrsGraph, findRelOffset, Node )
  {
    using Tpetra::Details::findRelOffset;
    typedef int LO;
    typedef typename Node::device_type DT;
    typedef Kokkos::View<const LO*, DT> IVT;

    Teuchos::OSTab tab0 (out);
    out << "Test findRelOffset" << endl;
    Teuchos::OSTab tab1 (out);

    Node node; // just to start the Kokkos execution space

    out << "Test empty arrays" << endl;

    // Test with zero indices to search, using a raw array.
    {
      LO numEnt = 0;
      const LO* indsToSearch = NULL;
      const LO indToFind = 42;
      for (LO hint = 0; hint < 3; ++hint) {
	// Length-zero array is trivially sorted, but try the unsorted
	// case just to make sure that branch of the code is right.
	LO offset = 
	  findRelOffset<LO, const LO* > (indsToSearch, numEnt, 
					 indToFind, hint, true);
	TEST_EQUALITY( offset, numEnt ); // not in the array
	offset = findRelOffset<LO, const LO* > (indsToSearch, numEnt, 
						indToFind, hint, false);
	TEST_EQUALITY( offset, numEnt ); // not in the array
      }
    }  

    out << "Test the sorted, nonempty array case" << endl;

    // Test the sorted case, with a raw array.
    {
      LO numEnt = 7;
      const LO indsToSearch[7] = {1, 1, 2, 3, 5, 8, 13};
      const bool isSorted = true;

      for (LO hint = 0; hint < 10; ++hint) {
	// Test an index that is not in the array.
	// This one is in [min, max].
	LO indNotThere = 4;
	LO offset = 
	  findRelOffset<LO, const LO* > (indsToSearch, numEnt, 
					 indNotThere, hint, isSorted);
	TEST_EQUALITY( offset, numEnt ); // not in the array

	// Test another index that is not in the array.
	// This one is _not_ in [min, max].
	indNotThere = 42;
	offset = findRelOffset<LO, const LO* > (indsToSearch, numEnt, 
						indNotThere, hint, isSorted);
	TEST_EQUALITY( offset, numEnt ); // not in the array

	// Test all indices that are in the array.
	for (LO k = 0; k < numEnt; ++k) {

	  const LO indToFind = indsToSearch[k]; // in the array
	  offset = findRelOffset<LO, const LO* > (indsToSearch, numEnt, 
						  indToFind, hint, isSorted);
	  if (indToFind == static_cast<LO> (1)) {
	    // 1 is a duplicate in this example.  Treat it as a special
	    // case.  We don't specify which instance of duplicates the
	    // function must return, so either one is fine.
	    TEST_ASSERT( offset == static_cast<LO> (0) || 
			 offset == static_cast<LO> (1) );
	  }
	  else {
	    TEST_EQUALITY( offset, k );
	  }
	}
      }
    }  

    // Test the sorted case, with a Kokkos::View.
    {
      LO numEnt = 7;
      Kokkos::View<LO*, DT> indsToSearch ("indsToSearch", numEnt);
      // This assumes UVM.
      indsToSearch[0] = 1;
      indsToSearch[1] = 1;
      indsToSearch[2] = 2;
      indsToSearch[3] = 3;
      indsToSearch[4] = 5;
      indsToSearch[5] = 8;
      indsToSearch[6] = 13;
      const bool isSorted = true;

      for (LO hint = 0; hint < 10; ++hint) {
	// Test an index that is not in the array.
	// This one is in [min, max].
	LO indNotThere = 4;
	LO offset = findRelOffset<LO, IVT> (indsToSearch, numEnt, 
					    indNotThere, hint, isSorted);
	TEST_EQUALITY( offset, numEnt ); // not in the array

	// Test another index that is not in the array.
	// This one is _not_ in [min, max].
	indNotThere = 42;
	offset = findRelOffset<LO, IVT> (indsToSearch, numEnt, 
					 indNotThere, hint, isSorted);
	TEST_EQUALITY( offset, numEnt ); // not in the array

	// Test all indices that are in the array.
	for (LO k = 0; k < numEnt; ++k) {
	  const LO indToFind = indsToSearch[k]; // in the array
	  offset = findRelOffset<LO, IVT> (indsToSearch, numEnt, 
					   indToFind, hint, isSorted);
	  if (indToFind == static_cast<LO> (1)) {
	    // 1 is a duplicate in this example.  Treat it as a special
	    // case.  We don't specify which instance of duplicates the
	    // function must return, so either one is fine.
	    TEST_ASSERT( offset == static_cast<LO> (0) || 
			 offset == static_cast<LO> (1) );
	  }
	  else {
	    TEST_EQUALITY( offset, k );
	  }
	}
      }
    }

    out << "Test the unsorted, nonempty array case" << endl;

    // Test the unsorted case, with a raw array.
    {
      LO numEnt = 7;
      const LO indsToSearch[7] = {8, 1, 13, 1, 3, 2, 5};
      const bool isSorted = false;

      for (LO hint = 0; hint < 10; ++hint) {
	// Test an index that is not in the array.
	// This one is in [min, max].
	LO indNotThere = 4;
	LO offset = 
	  findRelOffset<LO, const LO* > (indsToSearch, numEnt, 
					 indNotThere, hint, isSorted);
	TEST_EQUALITY( offset, numEnt ); // not in the array

	// Test another index that is not in the array.
	// This one is _not_ in [min, max].
	indNotThere = 42;
	offset = findRelOffset<LO, const LO* > (indsToSearch, numEnt, 
						indNotThere, hint, isSorted);
	TEST_EQUALITY( offset, numEnt ); // not in the array

	// Test all indices that are in the array.
	for (LO k = 0; k < numEnt; ++k) {
	  const LO indToFind = indsToSearch[k]; // in the array
	  offset = findRelOffset<LO, const LO* > (indsToSearch, numEnt, 
						  indToFind, hint, isSorted);
	  if (indToFind == static_cast<LO> (1)) {
	    // 1 is a duplicate in this example.  Treat it as a special
	    // case.  We don't specify which instance of duplicates the
	    // function must return, so either one is fine.
	    TEST_ASSERT( offset == static_cast<LO> (1) || 
			 offset == static_cast<LO> (3) );
	  }
	  else {
	    TEST_EQUALITY( offset, k );
	  }
	}
      }
    }

    // Test the unsorted case, with a Kokkos::View.
    {
      LO numEnt = 7;
      Kokkos::View<LO*, DT> indsToSearch ("indsToSearch", numEnt);
      // This assumes UVM.
      indsToSearch[0] = 8;
      indsToSearch[1] = 1;
      indsToSearch[2] = 13;
      indsToSearch[3] = 1;
      indsToSearch[4] = 3;
      indsToSearch[5] = 2;
      indsToSearch[6] = 5;
      const bool isSorted = false;

      for (LO hint = 0; hint < 10; ++hint) {
	// Test an index that is not in the array.
	// This one is in [min, max].
	LO indNotThere = 4;
	LO offset = findRelOffset<LO, IVT> (indsToSearch, numEnt, 
					    indNotThere, hint, isSorted);
	TEST_EQUALITY( offset, numEnt ); // not in the array

	// Test another index that is not in the array.
	// This one is _not_ in [min, max].
	indNotThere = 42;
	offset = findRelOffset<LO, IVT> (indsToSearch, numEnt, 
					 indNotThere, hint, isSorted);
	TEST_EQUALITY( offset, numEnt ); // not in the array

	// Test all indices that are in the array.
	for (LO k = 0; k < numEnt; ++k) {
	  const LO indToFind = indsToSearch[k]; // in the array
	  offset = findRelOffset<LO, IVT> (indsToSearch, numEnt, 
					   indToFind, hint, isSorted);
	  if (indToFind == static_cast<LO> (1)) {
	    // 1 is a duplicate in this example.  Treat it as a special
	    // case.  We don't specify which instance of duplicates the
	    // function must return, so either one is fine.
	    TEST_ASSERT( offset == static_cast<LO> (1) || 
			 offset == static_cast<LO> (3) );
	  }
	  else {
	    TEST_EQUALITY( offset, k );
	  }
	}
      }
    }
  }


  // Test findRelOffset with a longer array.  This ensures that even
  // if findRelOffset optimizes for short arrays by using linear
  // search, we'll still get test coverage for longer arrays.
  //
  // This test doesn't need to exercise all the Kokkos device types.
  // Even if the aforementioned short-array optimization has different
  // constants for different Kokkos device types, a sufficiently long
  // array should exercise all cases.  Thus, this is not a templated
  // test, so we don't need to add it to the list of instantiations
  // for templated tests at the bottom of this file.
  TEUCHOS_UNIT_TEST( findRelOffset, LongArray )
  {
    using Tpetra::Details::findRelOffset;
    typedef long LO; // just for a change

    Teuchos::OSTab tab0 (out);
    out << "Test findRelOffset with a long array" << endl;
    Teuchos::OSTab tab1 (out);

    // Start with the array [0, 1, 2, ..., 2n], where the number of
    // entries N = 2n+1 for natural numbers n.  Permute every other
    // entry symmetrically about the middle entry (which exists
    // because the number of entries is odd).  For example, for n = 4:
    // [0 1 2 3 4 5 6 7 8] gets permuted to [8 1 6 3 4 5 2 7 0].  Use
    // this to test findRelOffset.  (We don't just reverse x, in case
    // implementations optimize for reverse contiguous order.)

    const LO n = 100;
    const LO N = 2*n + 1;
    std::vector<LO> indsToSearch (N);

    for (LO k = 0; k < n; ++k) {
      indsToSearch[2*k] = 2*(n - k);
      indsToSearch[2*k + 1] = 2*k + 1;
    }

    // We don't need to test all possible hints, just two per search
    // value: the correct hint and some wrong hint.
    for (LO k = 0; k < N; ++k) {
      // We use std::vector<LO> in as the template parameter of
      // findRelOffset, because the function should work just fine
      // with anything that acts like a 1-D raw array.
      {
	const LO indToFind = indsToSearch[k];
	const LO expectedOffset = k;
	const LO correctHint = expectedOffset;
	// Add some number not 1 to make the "wrong hint," in case
	// there is a "search nearest" optimization (unlikely -- too
	// many branches).
	const LO wrongHint = expectedOffset + 7;

	const LO offset0 = 
	  findRelOffset<LO, std::vector<LO> > (indsToSearch, N, indToFind,
					       correctHint, false);
	TEST_EQUALITY( offset0, expectedOffset );
	const LO offset1 =
	  findRelOffset<LO, std::vector<LO> > (indsToSearch, N, indToFind,
					       wrongHint, false);
	TEST_EQUALITY( offset1, expectedOffset );      
      }
      {
	// This is the "index not in array" case.  We only need to
	// test one hint here, since all hints are wrong.
	const LO indToFind = N + 1; // not in the array
	const LO hint = 0;
	const LO offset0 =
	  findRelOffset<LO, std::vector<LO> > (indsToSearch, N, indToFind,
					       hint, false);
	TEST_EQUALITY( offset0, N );
      }
    }
  }

//
// Instantiations of templated unit tests
//

#define UNIT_TEST_GROUP( NODE ) \
  TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT( CrsGraph, findRelOffset, NODE )

  TPETRA_ETI_MANGLING_TYPEDEFS()

  TPETRA_INSTANTIATE_N( UNIT_TEST_GROUP )

} // namespace (anonymous)


