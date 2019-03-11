// @HEADER
// ************************************************************************
//
//               Rapid Optimization Library (ROL) Package
//                 Copyright (2014) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
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
// Questions? Contact lead developers:
//              Drew Kouri   (dpkouri@sandia.gov) and
//              Denis Ridzal (dridzal@sandia.gov)
//
// ************************************************************************
// @HEADER


#ifndef ROL_PINTHIERARCHY_HPP
#define ROL_PINTHIERARCHY_HPP

#include "ROL_TimeStamp.hpp"
#include "ROL_PinTVector.hpp"
#include "ROL_StdVector.hpp"
#include "ROL_Constraint_SimOpt.hpp"
#include "ROL_SerialConstraint.hpp"
#include "ROL_SerialConstraint.hpp"
#include "ROL_PinTCommunicationUtilities.hpp"

#include "Teuchos_Time.hpp"
#include "Teuchos_TimeMonitor.hpp"
#include "Teuchos_StackedTimer.hpp"

namespace ROL {

template<typename Real> 
class PinTHierarchy {

  using V  = ROL::Vector<Real>;
  using PV = ROL::PartitionedVector<Real>;

  using size_type = typename std::vector<Real>::size_type;
  template<typename T> using Ptr = ROL::Ptr<T>;

private:

  // internal state members
  bool isInitialized_;

  Ptr<const std::vector<TimeStamp<Real>>>         userTimeStamps_;    // these are the untouched ones the user passes in
  Ptr<std::vector<TimeStamp<Real>>>               timeStamps_;        // these are used internally
  std::vector<Ptr<std::vector<TimeStamp<Real>>>>  stamps_;            // these are used internally on each level

  // preconditioner settings
  int maxLevels_;               // must turn on multigrid 

  std::vector<Ptr<const PinTCommunicators>> communicators_;
  Ptr<const PinTVectorCommunication<Real>> vectorComm_; // object for sending whole vectors between processors

  bool rebalance_;

  /**
   * \brief Get the time stamps for by level
   *
   * The current implementation is recursive and expensive, but its the easiest
   * way to do this right now. 
   */
  Ptr<std::vector<TimeStamp<Real>>> buildTimeStampsByLevel(int level,
                                                           const ROL::Ptr<const PinTCommunicators> & comm) const
  {
    assert(level>=0); // precondition

    // base case
    if(level==0)
      return timeStamps_;

    int timeRank = comm->getTimeRank();

    Ptr<std::vector<TimeStamp<Real>>> higherLevel = getTimeStampsByLevel(level-1);

    int myFirstIndex = 0;
    int myLength = int(higherLevel->size());
    MPI_Exscan(&myLength,&myFirstIndex,1,MPI_INT,MPI_SUM,comm->getTimeCommunicator());

    // send your end point to the right
    Real coarseTimeEnd = (higherLevel->end()-1)->t[0];
    if(timeRank!=comm->getTimeSize()-1 and ((myFirstIndex+higherLevel->size()) % 2)==0) {
      int tag = 0;
      MPI_Send(&coarseTimeEnd,1,MPI_DOUBLE,comm->getTimeRank()+1,tag,comm->getTimeCommunicator());
    }

    // recieve the left time node on the coarse grid
    Real coarseTimeStart = 0.0;
    if(timeRank!=0 and (myFirstIndex % 2)==0) {
      int tag = 0;
      MPI_Recv(&coarseTimeStart,1,MPI_DOUBLE,comm->getTimeRank()-1,tag,comm->getTimeCommunicator(),MPI_STATUS_IGNORE);
    }
    else {
      coarseTimeStart = higherLevel->begin()->t[0];
    }

    // get the size of the array
    int currentStamps = -1; 
    if(higherLevel->size() % 2 ==0) {
      currentStamps = higherLevel->size()/2;
    }
    else {
      currentStamps = (higherLevel->size()-1)/2+(myFirstIndex+1)%2;
    }

    Ptr<std::vector<TimeStamp<Real>>> currentLevel 
        = ROL::makePtr<std::vector<ROL::TimeStamp<Real>>>(currentStamps);

    // you offset based on your starting index (and if you are on rank 0
    int offset = -1;
    if(myFirstIndex % 2 ==0)
      offset = 0;
    else 
      offset = 1;

    for(size_t k=0;k<currentLevel->size();k++) {
      currentLevel->at(k).t.resize(2);

      if(k==0) {
        currentLevel->at(k).t.at(0) = coarseTimeStart;
        currentLevel->at(k).t.at(1) = higherLevel->at(2*k+offset).t.at(1);
      }
      else {
        currentLevel->at(k).t.at(0) = higherLevel->at(2*k+offset-1).t.at(0);
        currentLevel->at(k).t.at(1) = higherLevel->at(2*k+offset).t.at(1);
      }
    }

    return currentLevel;
  }

public:

  //! Default constructor
  PinTHierarchy()
    : isInitialized_(false)
    , maxLevels_(-1)
    , rebalance_(false)
  { }

  /**
   * \brief Constructor
   *
   * Build a parallel-in-time constraint with a specified step constraint. This specifies
   * any communication you might need between processors and steps using "stencils".
   * Multigrid in time preconditioning is disabled by default.
   * 
   * \param[in] stepConstraint Constraint for a single step.
   * \param[in] initialCond Initial condition
   */
  PinTHierarchy(const Ptr<std::vector<TimeStamp<Real>>> & timeStamps)
    : isInitialized_(false)
    , maxLevels_(-1)
    , rebalance_(false)
  { 
    initialize(timeStamps);
  }

  /**
   * Turn on multigrid preconditioning in time with a specified number of levels.
   *
   * \param[in] maxLevels Largest number of levels
   */
  void setMaxLevels(int maxLevels)
  {
    maxLevels_ = maxLevels;
  }

  /**
   * \brief Get the time stamps for by level
   *
   * The current implementation is recursive and expensive, but its the easiest
   * way to do this right now. 
   */
  Ptr<std::vector<TimeStamp<Real>>> getTimeStampsByLevel(int level) const
  {
    return stamps_[level];
  }


  /** \brief Initialize this class, setting up parallel distribution.
   
   */
  void initialize(const Ptr<std::vector<TimeStamp<Real>>> & timeStamps)
  {
    // initialize user member variables
    userTimeStamps_ = timeStamps;

    // build up the internally used time stamps
    ////////////////////////////////////////////////////////////////
   
    timeStamps_ = makePtr<std::vector<TimeStamp<Real>>>(timeStamps->size());

    for(size_type k=0;k<timeStamps->size();k++) {
      timeStamps_->at(k).t = timeStamps->at(k).t;
    }

    isInitialized_ = true;
  }

   // restriction and prolongation functions
   ///////////////////////////////////////////////////////////////////////////////////////////
   
   ROL::Ptr<const PinTCommunicators> getLevelCommunicators(int level) const
   { 
      if(not rebalance_) return communicators_[0]; 
      else               return communicators_[level]; 
   }

   /** 
    * \brief Builds up communicators and time stamps for each level
    *
    * This currently doubles the size of the time step as it coarsens. This calls
    * the recursive helper function buildLevelDataStructures to do most of the work
    *
    * \param[in] level_0_ref Reference vector that is used to pull out all the communication
    *                        devices.
    */
   void buildLevels(const ROL::Ptr<const ROL::PinTCommunicators> & pintComm,
                    const ROL::Ptr<const ROL::PinTVectorCommunication<Real>> & vectorComm,
                    bool rebalance=false)
   {
     rebalance_ = rebalance;

     // set the vector communicator
     vectorComm_ = vectorComm;

     // we allocate the communicators with null
     communicators_.resize(maxLevels_);
     communicators_[0] = pintComm;

     // we allocate the time stamps vectors with null
     stamps_.resize(maxLevels_);
     stamps_[0] = timeStamps_;

     buildLevelDataStructures(1);
   }

   //! Recursive function that builds up the communicators and time stamps on coarse grids
   void buildLevelDataStructures(int level)
   {
     // protect sanity
     assert(level>0);

     // don't go too deep (base case)
     if(level>=maxLevels_) {
       return;
     }

     auto comm = getLevelCommunicators(level-1);
 
     if(comm==ROL::nullPtr)
       return;

     int rank = comm->getTimeRank();

     stamps_[level] = buildTimeStampsByLevel(level,comm);

     if(rebalance_) {
       // this processor is no longer participating (base case)
       if(ROL::is_nullPtr(comm)) {
         return;
       }

       // this will subdivide the communicators by two
       communicators_[level] = comm->buildCoarseCommunicators();

       // rebalance the coarse stamps
       ROL::Ptr<std::vector<ROL::TimeStamp<Real>>> coarseStamps 
         = ROL::makePtr<std::vector<ROL::TimeStamp<Real>>>();
       bool value = ROL::PinT::exportToCoarseDistribution_TimeStamps(*stamps_[level],*coarseStamps,*comm,0);

       stamps_[level] = coarseStamps;
     }

     buildLevelDataStructures(level+1);
   }

   /**
    * \brief Allocate a simulation space vector at a particular multigrid level.
    */
   Ptr<Vector<Real>> allocateSimVector(const Vector<Real> & level_0_ref,int level) const
   {
     const PinTVector<Real> & pint_ref  = dynamic_cast<const PinTVector<Real>&>(level_0_ref);
     ROL::Ptr<const PinTCommunicators> comm = getLevelCommunicators(level);
     auto vectorComm = pint_ref.vectorCommunicationPtr();
   
     int totalSteps = 0;
     Ptr<std::vector<TimeStamp<Real>>> stamps = getTimeStampsByLevel(level);
     int mySteps = int(stamps->size());
     MPI_Allreduce(&mySteps,&totalSteps,1,MPI_INT,MPI_SUM,comm->getTimeCommunicator());

     return makePtr<PinTVector<Real>>(comm,vectorComm,pint_ref.getVectorPtr(0)->clone(),totalSteps,mySteps,1,2);
   }

   /**
    * \brief Allocate a optimization space vector at a particular multigrid level.
    */
   Ptr<Vector<Real>> allocateOptVector(const Vector<Real> & level_0_ref,int level) const
   {
     const PinTVector<Real> & pint_ref  = dynamic_cast<const PinTVector<Real>&>(level_0_ref);
     ROL::Ptr<const PinTCommunicators> comm = getLevelCommunicators(level);
     auto vectorComm = pint_ref.vectorCommunicationPtr();
     
     int totalSteps = 0;
     Ptr<std::vector<TimeStamp<Real>>> stamps = getTimeStampsByLevel(level);
     int mySteps = int(stamps->size());
     MPI_Allreduce(&mySteps,&totalSteps,1,MPI_INT,MPI_SUM,comm->getTimeCommunicator());

     return  makePtr<PinTVector<Real>>(comm,vectorComm,pint_ref.getVectorPtr(0)->clone(),totalSteps,mySteps,1,1);
   }
   
   /**
    * \brief Restrict a simulation space vector
    *
    * Currently doing the following. Let the coarse distribution be defined locally
    * to contain Np steps. The fine discretization virtual variable is denoted by a V, while the coarse
    * discretization is denoted by a v (similarly U and u for the primary variable). A superscript indicates the relative processor index.
    * If no index is included then it is assumed to be 0 (denoting no relative change in
    * processor index). The restriction for the sim vector thus computes
    *
    *   v_i = V_{2*i} + V_{2*i+1} + V_{2*i+2}      0 <= i < Np - 1
    *   u_i = U_{2*i} + U_{2*i+1} + U_{2*i+2}      0 <= i < Np - 1
    *
    * This ensures that for all interior points the coarse grid is the sum of the three neighbors. These
    * averages are dividecd by three.
    * At the boundaries of processors the same thing holds, but with the appropriate sharing. Finally,
    * at the origin for v and the terminus for u only the relavant two points are taken and the sum is divided
    * by two.
    *
    * \param[in] input The input vector to be "restricted" 
    * \param[in] output The output vector resulting from restriction, must be at level inputLevel+1
    * \param[in] inputLevel Multigrid level of the input vector (not currently used)
    */
   void restrictSimVector(const Vector<Real> & input,Vector<Real> & output,int inputLevel)
   {
     const PinTVector<Real> & pint_input  = dynamic_cast<const PinTVector<Real>&>(input);
     PinTVector<Real>       & pint_output = dynamic_cast<PinTVector<Real>&>(output);

     int numSteps = pint_input.numOwnedSteps();
     std::pair<int,int> fneRange = pint_input.ownedStepRange();

     int timeRank = pint_output.communicators().getTimeRank();

     int crs_i = 0;
     for(int i=0;i<numSteps;i++) {
       // only do evens
       if((i+fneRange.first) % 2 != 0) 
         continue;

       ROL::Vector<Real> & out_u = *pint_output.getVectorPtr(2*crs_i);
       out_u.axpy(1.0,*pint_input.getVectorPtr(2*i)); 

       ROL::Vector<Real> & out_v = *pint_output.getVectorPtr(2*crs_i+1);
       out_v.axpy(1.0,*pint_input.getVectorPtr(2*i+1)); 

       crs_i++;
     }
   }

   /**
    * \brief Restrict a control space vector
    *
    * Currently assumes a piecewise constant control
    *
    * \param[in] inputLevel Multigrid level of the input vector (not currently used)
    * \param[in] input The input vector to be "restricted" 
    * \param[in] output The output vector resulting from restriction, must be at level inputLevel+1
    */
   void restrictOptVector(const Vector<Real> & input,Vector<Real> & output,int inputLevel)
   {
     const PinTVector<Real> & pint_input  = dynamic_cast<const PinTVector<Real>&>(input);
     PinTVector<Real>       & pint_output = dynamic_cast<PinTVector<Real>&>(output);

     int timeRank = pint_output.communicators().getTimeRank();

     // communicate points on the left of this interval
     pint_input.boundaryExchangeLeftToRight();
     auto leftStart = pint_input.getRemoteBufferPtr(0)->clone();
     leftStart->set(*pint_input.getRemoteBufferPtr(0));

     std::pair<int,int> fneRange = pint_input.ownedStepRange();
     std::pair<int,int> crsRange = pint_output.ownedStepRange();

     // handle interior
     for(int k=0;k<pint_output.numOwnedSteps();k++) {
       int crsIndex = crsRange.first+k;
       if(crsIndex == 0) {
         pint_output.getVectorPtr(k)->set(*pint_input.getVectorPtr(0));
       }
       else {
         int fineLocal = 2*crsIndex-1-fneRange.first;
                          
         if(fineLocal>=0)
           pint_output.getVectorPtr(k)->set(     *pint_input.getVectorPtr(fineLocal));
         else
           pint_output.getVectorPtr(k)->set(     *leftStart);
         pint_output.getVectorPtr(k)->axpy(1.0,*pint_input.getVectorPtr(fineLocal+1));
         pint_output.getVectorPtr(k)->scale(1.0/2.0);
       }
     }
   }

   /**
    * \brief Prolong a simulation space vector
    *
    *   X_{2*i+1} = x_i                           -1 <= i < Np      (injection variables, including virtual variable)
    *   X_{2*i+2} = (x_i + x_{i+1})/2              0 <= i < Np - 1  (averaged variables, created at fine level)
    *
    * Note: Currently if the timeRank==0, the initial condition is assumed to be zero. 
    *
    * \param[in] input The input vector to be "restricted" 
    * \param[in] output The output vector resulting from restriction, must be at level inputLevel+1
    * \param[in] inputLevel Multigrid level of the input vector (not currently used)
    */
   void prolongSimVector(const Vector<Real> & input,Vector<Real> & output,int inputLevel)
   {
     const PinTVector<Real> & pint_input  = dynamic_cast<const PinTVector<Real>&>(input); // coarse vector
     PinTVector<Real>       & pint_output = dynamic_cast<PinTVector<Real>&>(output);

     std::pair<int,int> fneRange = pint_output.ownedStepRange();

     int timeRank = pint_output.communicators().getTimeRank();
     int fineSteps = pint_output.numOwnedSteps();

     // communicate points on the left of this interval
     pint_input.boundaryExchangeLeftToRight();
     auto u_fineStart = pint_input.getRemoteBufferPtr(0)->clone();
     auto v_fineStart = pint_input.getRemoteBufferPtr(1)->clone();
     u_fineStart->set(*pint_input.getRemoteBufferPtr(0));             // this is a little unpalatable since we are
     v_fineStart->set(*pint_input.getRemoteBufferPtr(1));             // allocating more memory. it might be better for
                                                                      // the user to pass in a buffer

     // communicate points on the right of this interval
     pint_input.boundaryExchangeRightToLeft();
     auto u_fineEnd = pint_input.getRemoteBufferPtr(0);
     auto v_fineEnd = pint_input.getRemoteBufferPtr(1);
     
     for(int fne_i=0;fne_i<fineSteps;fne_i++) {
       int fneIndex = fne_i+fneRange.first;

       // fine indices
       int u_fne_index = 2*fne_i;   
       int v_fne_index = 2*fne_i+1;   

       if(fneIndex % 2 == 0) {
         int crs_i = fne_i/2;   // here fne_i can be odd, but it still works out ... right?
         int u_crs_index = 2*crs_i;
         int v_crs_index = 2*crs_i+1;

         pint_output.getVectorPtr(u_fne_index)->set(*pint_input.getVectorPtr(u_crs_index));
         pint_output.getVectorPtr(v_fne_index)->set(*pint_input.getVectorPtr(v_crs_index));
       }
       else {
         
         int coarse_si_0 = (fne_i-1)/2;     // coarse step index
         int coarse_si_1 = coarse_si_0+1;
         if(fne_i-1 < 0) {
           coarse_si_0 = -1;
           coarse_si_1 = 0;
         }

         int u_crs_index = -1; 
         int v_crs_index = -1;
 
         u_crs_index = 2*coarse_si_0;
         v_crs_index = 2*coarse_si_0+1;

         // this special case is used when the first fine time step does not belong to the 
         // coarse grid
         if(coarse_si_0 < 0) {
           pint_output.getVectorPtr(u_fne_index)->set(*u_fineStart);
           pint_output.getVectorPtr(v_fne_index)->set(*v_fineStart);
         }
         else {
           pint_output.getVectorPtr(u_fne_index)->set(*pint_input.getVectorPtr(u_crs_index));
           pint_output.getVectorPtr(v_fne_index)->set(*pint_input.getVectorPtr(v_crs_index));
         }

         u_crs_index = 2*coarse_si_1;
         v_crs_index = 2*coarse_si_1+1;

         // this special case is used when the last fine time step does not belong to the 
         // coarse grid
         if(coarse_si_1 >= pint_input.numOwnedSteps()) {
           pint_output.getVectorPtr(u_fne_index)->axpy(1.0,*u_fineEnd);
           pint_output.getVectorPtr(v_fne_index)->axpy(1.0,*v_fineEnd);
         }
         else {
           pint_output.getVectorPtr(u_fne_index)->axpy(1.0,*pint_input.getVectorPtr(u_crs_index));
           pint_output.getVectorPtr(v_fne_index)->axpy(1.0,*pint_input.getVectorPtr(v_crs_index));
         }

         // average the two points
         pint_output.getVectorPtr(u_fne_index)->scale(0.5);
         pint_output.getVectorPtr(v_fne_index)->scale(0.5);
       }
     }
   }

   /**
    * \brief Prolong a control space vector
    *
    * Currently assumes a piecewise constant control
    *
    * \param[in] inputLevel Multigrid level of the input vector (not currently used)
    * \param[in] input The input vector to be "restricted" 
    * \param[in] output The output vector resulting from restriction, must be at level inputLevel+1
    */
   void prolongOptVector(const Vector<Real> & input,Vector<Real> & output,int inputLevel)
   {
     const PinTVector<Real> & pint_input  = dynamic_cast<const PinTVector<Real>&>(input);
     PinTVector<Real>       & pint_output = dynamic_cast<PinTVector<Real>&>(output);

     // communicate points on the right of this interval
     pint_input.boundaryExchangeRightToLeft();
     auto rightStart = pint_input.getRemoteBufferPtr(0)->clone();
     rightStart->set(*pint_input.getRemoteBufferPtr(0));

     int offset = 0;
     std::pair<int,int> crsRange = pint_input.ownedStepRange();
     std::pair<int,int> fneRange = pint_output.ownedStepRange();

     int timeRank = pint_output.communicators().getTimeRank();

     // handle interior
     for(int k=0;k<pint_output.numOwnedSteps();k++) {
       int fineIndex = fneRange.first+k;

       if(fineIndex==0) {
         pint_output.getVectorPtr(0)->set(*pint_input.getVectorPtr(0)); 
       }
       else {
         int crsIndex = (fineIndex+1)/2 - crsRange.first;
         if(crsIndex<0)
           throw std::logic_error("uh oh... didn't think this could happen");

         if(crsIndex<pint_input.numOwnedSteps())
           pint_output.getVectorPtr(k)->set(*pint_input.getVectorPtr(crsIndex)); 
         else
           pint_output.getVectorPtr(k)->set(*rightStart);
       }
     }
   }

}; // ROL::PinTConstraint


} // namespace ROL 

#endif // ROL_PINTCONSTRAINT_HPP
