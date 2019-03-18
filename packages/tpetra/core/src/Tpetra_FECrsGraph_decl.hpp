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

#ifndef TPETRA_FECRSGRAPH_DECL_HPP
#define TPETRA_FECRSGRAPH_DECL_HPP

/// \file Tpetra_FECrsGraph_decl.hpp
/// \brief Declaration of the Tpetra::FECrsGraph class
///
/// If you want to use Tpetra::FECrsGraph, include "Tpetra_FECrsGraph.hpp"
/// (a file which CMake generates and installs for you).  If you only
/// want the declaration of Tpetra::FECrsGraph, include this file
/// (Tpetra_FECrsGraph_decl.hpp).

#include "Tpetra_CrsGraph_decl.hpp"
namespace Tpetra {

 
  /// \class FECrsGraph
  /// \brief A distributed graph accessed by rows (adjacency lists)
  ///   and stored sparsely.
  ///
  /// \tparam LocalOrdinal The type of local indices.  See the
  ///   documentation of Map for requirements.
  /// \tparam GlobalOrdinal The type of global indices.  See the
  ///   documentation of Map for requirements.
  /// \tparam Node The Kokkos Node type.  See the documentation of Map
  ///   for requirements.
  ///
  /// This class implements a distributed-memory parallel sparse
  /// graph.  It provides access by rows to the elements of the graph,
  /// as if the local data were stored in compressed sparse row format
  /// (adjacency lists, in graph terms).  (Implementations are
  /// <i>not</i> required to store the data in this way internally.)
  /// This class has an interface like that of Epetra_CrsGraph, but
  /// also allows insertion of data into nonowned rows, much like
  /// Epetra_FECrsGraph.
  ///
  /// \section Tpetra_FECrsGraph_prereq Prerequisites
  ///
  /// Before reading the rest of this documentation, it helps to know
  /// something about the Teuchos memory management classes, in
  /// particular Teuchos::RCP, Teuchos::ArrayRCP, and
  /// Teuchos::ArrayView.  You should also know a little bit about MPI
  /// (the Message Passing Interface for distributed-memory
  /// programming).  You won't have to use MPI directly to use
  /// CrsGraph, but it helps to be familiar with the general idea of
  /// distributed storage of data over a communicator.  Finally, you
  /// should read the documentation of Map.
  ///
  /// \section Tpetra_FECrsGraph_local_vs_global Local vs. global indices and nonlocal insertion
  ///
  /// Graph entries can be added using either local or global coordinates
  /// for the indices. The accessors isGloballyIndexed() and
  /// isLocallyIndexed() indicate whether the indices are currently
  /// stored as global or local indices. Many of the class methods are
  /// divided into global and local versions, which differ only in
  /// whether they accept/return indices in the global or local
  /// coordinate space. Some of these methods may only be used if the
  /// graph coordinates are in the appropriate coordinates.  For example,
  /// getGlobalRowView() returns a View to the indices in global
  /// coordinates; if the indices are not in global coordinates, then no
  /// such View can be created.
  ///
  /// The global/local distinction does distinguish between operation
  /// on the global/local graph. Almost all methods operate on the
  /// local graph, i.e., the rows of the graph associated with the
  /// local node, per the distribution specified by the row
  /// map. Access to non-local rows requires performing an explicit
  /// communication via the import/export capabilities of the CrsGraph
  /// object; see DistObject. However, the method
  /// insertGlobalIndices() is an exception to this rule, as non-local
  /// rows are allowed to be added via the local graph. These rows are
  /// stored in the local graph and communicated to the appropriate
  /// node on the next call to globalAssemble() or fillComplete() (the
  /// latter calls the former).
  template <class LocalOrdinal,
            class GlobalOrdinal,
            class Node>
  class FECrsGraph :
    public CrsGraph<LocalOrdinal, GlobalOrdinal, Node>
  {
    //! The specialization of DistObject that is this class' parent class.
    typedef DistObject<GlobalOrdinal, LocalOrdinal, GlobalOrdinal, Node> dist_object_type;

    template <class S, class LO, class GO, class N>
    friend class FECrsMatrix;
  public:
    //! Parent class
    typedef CrsGraph<LocalOrdinal, GlobalOrdinal, Node> crs_graph_type;

    //! This class' first template parameter; the type of local indices.
    typedef LocalOrdinal local_ordinal_type;
    //! This class' second template parameter; the type of global indices.
    typedef GlobalOrdinal global_ordinal_type;
    //! This class' Kokkos Node type.
    typedef Node node_type;

    //! The Kokkos device type.
    typedef typename CrsGraph<LocalOrdinal, GlobalOrdinal, Node>::device_type device_type;
    //! The Kokkos execution space.
    typedef typename CrsGraph<LocalOrdinal, GlobalOrdinal, Node>::execution_space execution_space;

    //! The type of the part of the sparse graph on each MPI process.
    typedef  typename CrsGraph<LocalOrdinal, GlobalOrdinal, Node>::local_graph_type local_graph_type;

    //! The Map specialization used by this class.
    using map_type = ::Tpetra::Map<LocalOrdinal, GlobalOrdinal, Node>;
    //! The Import specialization used by this class.
    using import_type = ::Tpetra::Import<LocalOrdinal, GlobalOrdinal, Node>;
    //! The Export specialization used by this class.
    using export_type = ::Tpetra::Export<LocalOrdinal, GlobalOrdinal, Node>;

    //! @name Constructor/Destructor Methods
    //@{

    /// \brief Constructorfor globally-indexed assembly specifying a single upper bound for the
    ///   number of entries in all rows on the calling process.
    ///
    /// \param ownedRowMap [in] Distribution of rows of the owned graph.
    ///
    /// \param ownedPlusSharedRowMap [in] ownedMap plus the list of shared rows to which off-processor insertion is allowed
    ///
    /// \param maxNumEntriesPerRow [in] Maximum number of graph
    ///   entries per row.  You cannot exceed this number of
    ///   entries in any row.
    ///
    /// \param ownedPlusSharedToOwnedimporter [in] Optional importer between the ownedMap and ownedPlusSharedMap
    ///   This will be calculated by FECrsGraph if it is not provided
    ///
    /// \param domainMap [in] Optional domainMap for the owned graph.  If this is not provided, then ownedMap 
    ///   will be used for the domainMap in the call to endFill()
    ///
    /// \param rangeMap [in] Optional domainMap for the owned graph.  If this is not provided, then rangeMap 
    ///   will be used for the domainMap in the call to endFill()
    ///
    /// \param params [in/out] Optional list of parameters.  If not
    ///   null, any missing parameters will be filled in with their
    ///   default values.
    FECrsGraph(const Teuchos::RCP<const map_type> & ownedRowMap,
               const Teuchos::RCP<const map_type> & ownedPlusSharedRowMap,
               const size_t maxNumEntriesPerRow,
               const Teuchos::RCP<const import_type> & ownedPlusSharedToOwnedimporter = Teuchos::null,
               const Teuchos::RCP<const map_type> & domainMap = Teuchos::null,
               const Teuchos::RCP<const map_type> & rangeMap = Teuchos::null,
               const Teuchos::RCP<Teuchos::ParameterList>& params = Teuchos::null);

    /// \brief Constructor for globally-indexed assembly specifying a (possibly different) upper
    ///   bound for the number of entries in each row.
    ///
    /// \param ownedRowMap [in] Distribution of rows of the owned graph.
    ///
    /// \param ownedPlusSharedRowMap [in] ownedMap plus the list of shared rows to which off-processor insertion is allowed
    ///
    /// \param numEntPerRow [in] Maximum number of graph entries to
    ///   allocate for each row.  You cannot exceed the allocated
    ///   number of entries for any row.
    ///
    /// \param ownedPlusSharedToOwnedimporter [in] Optional importer between the ownedMap and ownedPlusSharedMap
    ///   This will be calculated by FECrsGraph if it is not provided
    ///
    /// \param domainMap [in] Optional domainMap for the owned graph.  If this is not provided, then ownedMap 
    ///   will be used for the domainMap in the call to endFill()
    ///
    /// \param rangeMap [in] Optional domainMap for the owned graph.  If this is not provided, then rangeMap 
    ///   will be used for the domainMap in the call to endFill()
    ///
    /// \param params [in/out] Optional list of parameters.  If not
    ///   null, any missing parameters will be filled in with their
    ///   default values.
    FECrsGraph (const Teuchos::RCP<const map_type> & ownedRowMap,
                const Teuchos::RCP<const map_type> & ownedPlusSharedRowMap,
                const Kokkos::DualView<const size_t*, execution_space>& numEntPerRow,
                const Teuchos::RCP<const import_type> & ownedPlusSharedToOwnedimporter = Teuchos::null,
                const Teuchos::RCP<const map_type> & domainMap = Teuchos::null,
                const Teuchos::RCP<const map_type> & rangeMap = Teuchos::null,  
                const Teuchos::RCP<Teuchos::ParameterList>& params = Teuchos::null);

    
    /// \brief Constructor for locally-indexed assembly specifying a single upper bound for the
    ///   number of entries in all rows on the calling process.
    ///
    /// \param ownedRowMap [in] Distribution of rows of the owned graph.
    ///
    /// \param ownedPlusSharedRowMap [in] ownedMap plus the list of shared rows to which off-processor insertion is allowed
    ///
    /// \param ownedPlusSharedColMap [in] list of owned and shared columns into which assertion is allowed. 
    ///
    /// \param maxNumEntriesPerRow [in] Maximum number of graph
    ///   entries per row.  You cannot exceed this number of
    ///   entries in any row.
    ///
    /// \param ownedPlusSharedToOwnedimporter [in] Optional importer between the ownedMap and ownedPlusSharedMap
    ///   This will be calculated by FECrsGraph if it is not provided
    ///
    /// \param domainMap [in] Optional domainMap for the owned graph.  If this is not provided, then ownedMap 
    ///   will be used for the domainMap in the call to endFill()
    ///
    /// \param rangeMap [in] Optional domainMap for the owned graph.  If this is not provided, then rangeMap 
    ///   will be used for the domainMap in the call to endFill()
    ///
    /// \param params [in/out] Optional list of parameters.  If not
    ///   null, any missing parameters will be filled in with their
    ///   default values.
    FECrsGraph(const Teuchos::RCP<const map_type> & ownedRowMap,
               const Teuchos::RCP<const map_type> & ownedPlusSharedRowMap,
               const Teuchos::RCP<const map_type> & ownedPlusSharedColMap,
               const size_t maxNumEntriesPerRow,
               const Teuchos::RCP<const import_type> & ownedPlusSharedToOwnedimporter = Teuchos::null,
               const Teuchos::RCP<const map_type> & domainMap = Teuchos::null,
               const Teuchos::RCP<const map_type> & rangeMap = Teuchos::null,
               const Teuchos::RCP<Teuchos::ParameterList>& params = Teuchos::null);


    /// \brief Constructor for locally-indexed assembly specifying a (possibly different) upper
    ///   bound for the number of entries in each row.
    ///
    /// \param ownedRowMap [in] Distribution of rows of the owned graph.
    ///
    /// \param ownedPlusSharedRowMap [in] ownedMap plus the list of shared rows to which off-processor insertion is allowed
    ///
    /// \param numEntPerRow [in] Maximum number of graph entries to
    ///   allocate for each row.  You cannot exceed the allocated
    ///   number of entries for any row.
    ///
    /// \param ownedPlusSharedToOwnedimporter [in] Optional importer between the ownedMap and ownedPlusSharedMap
    ///   This will be calculated by FECrsGraph if it is not provided
    ///
    /// \param domainMap [in] Optional domainMap for the owned graph.  If this is not provided, then ownedMap 
    ///   will be used for the domainMap in the call to endFill()
    ///
    /// \param rangeMap [in] Optional domainMap for the owned graph.  If this is not provided, then rangeMap 
    ///   will be used for the domainMap in the call to endFill()
    ///
    /// \param params [in/out] Optional list of parameters.  If not
    ///   null, any missing parameters will be filled in with their
    ///   default values.
    FECrsGraph (const Teuchos::RCP<const map_type> & ownedRowMap,
                const Teuchos::RCP<const map_type> & ownedPlusSharedRowMap,
                const Teuchos::RCP<const map_type> & ownedPlusSharedColMap,
                const Kokkos::DualView<const size_t*, execution_space>& numEntPerRow,
                const Teuchos::RCP<const import_type> & ownedPlusSharedToOwnedimporter = Teuchos::null,
                const Teuchos::RCP<const map_type> & domainMap = Teuchos::null,
                const Teuchos::RCP<const map_type> & rangeMap = Teuchos::null,  
                const Teuchos::RCP<Teuchos::ParameterList>& params = Teuchos::null);


    //! Destructor.
    virtual ~FECrsGraph () = default;

    //@}
    //! @name Collective methods for changing the graph's global state
    //@{

    /// \brief Communicate nonlocal contributions to other processes.
    ///
    /// This method is called automatically by fillComplete().  Most
    /// users do not need to call this themselves.
    ///
    /// This method must be called collectively (that is, like any MPI
    /// collective) over all processes in the graph's communicator.
    void globalAssemble (){endFill();}

   //! Migrates data to the owned mode
    void endFill();

    //! Activates the owned+shared mode for assembly.  This can only be called once.
    void beginFill();

 
#if 0 // CMS: Do I need these?
    /// \brief Resume fill operations.
    ///
    /// After calling fillComplete(), resumeFill() must be called
    /// before initiating any changes to the graph.
    ///
    /// resumeFill() may be called repeatedly.
    ///
    /// \warning A CrsGraph instance does not currently (as of 23 Jul
    ///   2017) and never did support arbitrary structure changes
    ///   after the first fillComplete call on that instance.  The
    ///   safest thing to do is not to change structure at all after
    ///   first fillComplete.
    ///
    /// \post <tt>isFillActive() == true<tt>
    /// \post <tt>isFillComplete() == false<tt>
    ///
    /// This method must be called collectively (that is, like any MPI
    /// collective) over all processes in the graph's communicator.
    void
    resumeFill (const Teuchos::RCP<Teuchos::ParameterList>& params =
                  Teuchos::null);
#endif

    /// \brief Tell the graph that you are done changing its structure.
    ///
    /// This tells the graph to optimize its data structures for
    /// computational kernels, and to prepare (MPI) communication
    /// patterns.
    ///
    /// Off-process indices are distributed (via globalAssemble()),
    /// indices are sorted, redundant indices are eliminated, and
    /// global indices are transformed to local indices.
    ///
    /// This method must be called collectively (that is, like any MPI
    /// collective) over all processes in the graph's communicator.
    ///
    /// \warning The domain Map and row Map arguments to this method
    ///   MUST be one to one!  If you have Maps that are not one to
    ///   one, and you do not know how to make a Map that covers the
    ///   same global indices but <i>is</i> one to one, then you may
    ///   call Tpetra::createOneToOne() (see Map's header file) to
    ///   make a one-to-one version of your Map.
    ///
    /// \pre  <tt>   isFillActive() && ! isFillComplete() </tt>
    /// \post <tt> ! isFillActive() &&   isFillComplete() </tt>
    ///
    /// \param domainMap [in] The graph's domain Map.  MUST be one to
    ///   one!
    /// \param rangeMap [in] The graph's range Map.  MUST be one to
    ///   one!  May be, but need not be, the same as the domain Map.
    /// \param params [in/out] List of parameters controlling this
    ///   method's behavior.  See below for valid parameters.
    ///
    /// List of valid parameters in <tt>params</tt>:
    /// <ul>
    /// <li> "Optimize Storage" (\c bool): Default is false.  If true,
    ///      then isStorageOptimized() returns true after this method
    ///      finishes.  See isStorageOptimized() for consequences.
    /// </li>
    /// </ul>
    void
    fillComplete (const Teuchos::RCP<const map_type>& domainMap,
                  const Teuchos::RCP<const map_type>& rangeMap,
                  const Teuchos::RCP<Teuchos::ParameterList>& params = Teuchos::null) {
      domainMap_ = domainMap;
      rangeMap_ = rangeMap;
      endFill();
    }

    /// \brief Tell the graph that you are done changing its
    ///   structure; set default domain and range Maps.
    ///
    /// See above three-argument version of fillComplete for full
    /// documentation.  If the graph does not yet have domain and
    /// range Maps (i.e., if fillComplete has not yet been called on
    /// this graph at least once), then this method uses the graph's
    /// row Map (result of this->getRowMap()) as both the domain Map
    /// and the range Map.  Otherwise, this method uses the graph's
    /// existing domain and range Maps.
    ///
    /// This method must be called collectively (that is, like any MPI
    /// collective) over all processes in the graph's communicator.
    ///
    /// \warning It is only valid to call this overload of
    ///   fillComplete if the row Map is one to one!  If the row Map
    ///   is NOT one to one, you must call the above three-argument
    ///   version of fillComplete, and supply one-to-one domain and
    ///   range Maps.  If you have Maps that are not one to one, and
    ///   you do not know how to make a Map that covers the same
    ///   global indices but <i>is</i> one to one, then you may call
    ///   Tpetra::createOneToOne() (see Map's header file) to make a
    ///   one-to-one version of your Map.
    ///
    /// \param params [in/out] List of parameters controlling this
    ///   method's behavior.  See documentation of the three-argument
    ///   version of fillComplete (above) for valid parameters.
    void
    fillComplete (const Teuchos::RCP<Teuchos::ParameterList>& params = Teuchos::null) {endFill();}


  protected:
    /// \brief Migrate data from the owned+shared to the owned graph
    /// Since this is non-unique -> unique, we need a combine mode.
    /// Precondition: Must be FE_ACTIVE_OWNED_PLUS_SHARED mode
    void doOwnedPlusSharedToOwned(const CombineMode CM=Tpetra::ADD);

    /// \brief Migrate data from the owned to the owned+shared graph
    /// Precondition: Must be FE_ACTIVE_OWNED mode
    void doOwnedToOwnedPlusShared(const CombineMode CM=Tpetra::ADD);



  public:  // HAQ
    //! Switches which CrsGraph is active (without migrating data)
    void switchActiveCrsGraph();
    //@}

  private:

    // Common core guts of the constructor (the colMap argument is Teuchos::null if we're globally-indexed)
    void setup(const Teuchos::RCP<const map_type>  & ownedRowMap, const Teuchos::RCP<const map_type> & ownedPlusSharedRowMap,const Teuchos::RCP<const map_type> & ownedPlusSharedColMap, const Teuchos::RCP<Teuchos::ParameterList>& params);




    // We forbid assignment (operator=) by declaring this method
    // private and not implementing it.
    FECrsGraph<LocalOrdinal, GlobalOrdinal, Node>&
    operator= (const FECrsGraph<LocalOrdinal, GlobalOrdinal, Node>& rhs);


    // Enum for activity
    enum FEWhichActive
    {
      FE_ACTIVE_OWNED,
      FE_ACTIVE_OWNED_PLUS_SHARED
    };


    // This is whichever multivector isn't currently active
    Teuchos::RCP<CrsGraph<LocalOrdinal, GlobalOrdinal, Node> > inactiveCrsGraph_;
    // This is in RCP to make shallow copies of the FECrsGraph work correctly
    Teuchos::RCP<FEWhichActive> activeCrsGraph_;

    // The importer between the rowmaps of the two graphs
    Teuchos::RCP<const import_type> importer_;

    // The domainMap to use in endFill(), if provided
    Teuchos::RCP<const map_type> domainMap_;

    // The rangeMap to use in endFill(), if provided
    Teuchos::RCP<const map_type> rangeMap_;


  }; // class FECrsGraph

 
} // namespace Tpetra

#endif // TPETRA_FECRSGRAPH_DECL_HPP
