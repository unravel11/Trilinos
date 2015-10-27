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

/*! \file  example_04.cpp
    \brief Shows how to solve a control problem governed by a nonlinear parabolic equation
           with bound constraints.
*/

#include "ROL_Algorithm.hpp"
#include "ROL_TrustRegionStep.hpp"
#include "ROL_PrimalDualActiveSetStep.hpp"
#include "ROL_CompositeStep.hpp"
#include "ROL_StatusTest.hpp"
#include "ROL_ConstraintStatusTest.hpp"
#include "ROL_Types.hpp"

#include "ROL_StdVector.hpp"
#include "ROL_Vector_SimOpt.hpp"
#include "ROL_EqualityConstraint_SimOpt.hpp"
#include "ROL_Objective_SimOpt.hpp"
#include "ROL_Reduced_Objective_SimOpt.hpp"
#include "ROL_StdBoundConstraint.hpp"

#include "Teuchos_oblackholestream.hpp"
#include "Teuchos_GlobalMPISession.hpp"
#include "Teuchos_XMLParameterListHelpers.hpp"
#include "Teuchos_LAPACK.hpp"

#include <iostream>
#include <algorithm>
#include <ctime>

template<class Real>
class EqualityConstraint_ParabolicControl : public ROL::EqualityConstraint_SimOpt<Real> {

  typedef std::vector<Real>    vector;
  typedef ROL::Vector<Real>    V;
  typedef ROL::StdVector<Real> SV;
  
  typedef typename vector::size_type uint; 

private:
  std::vector<Real> u0_;
  Real eps1_;
  Real eps2_;
  uint nx_;
  uint nt_;
  Real T_;
  Real dx_;
  Real dt_;

/***************************************************************/
/********** BEGIN PRIVATE MEMBER FUNCTION DECLARATION **********/
/***************************************************************/
  void apply_mass(std::vector<Real> &Mu, const std::vector<Real> &u ) {
    Mu.clear();
    Mu.resize(nx_,0.0);
    for (uint n = 0; n < nx_; n++) {
      if ( n < nx_-1 ) {
        Mu[n] += dx_/6.0*(2.0*u[n]+u[n+1]);
      }
      if ( n > 0 ) {
        Mu[n] += dx_/6.0*(u[n-1]+2.0*u[n]);
      }
    }
  }

  void compute_pde_jacobian(std::vector<Real> &d, std::vector<Real> &o, const std::vector<Real> &u) {
    // Get Diagonal and Off-Diagonal Entries of linear PDE Jacobian
    d.clear();
    d.resize(nx_,4.0*dx_/6.0 + dt_*eps1_*2.0/dx_);
    d[0]     = dx_/3.0 + dt_*eps1_/dx_;
    d[nx_-1] = dx_/3.0 + dt_*eps1_/dx_;
    o.clear();
    o.resize(nx_-1,dx_/6.0 - dt_*eps1_/dx_);
    // Contribution from nonlinearity
    d[nx_-1] += eps2_*dt_*4.0*std::pow(u[nx_-1],3.0);
  }

  void apply_pde_jacobian(std::vector<Real> &jv, const std::vector<Real> &v, const std::vector<Real> &u) {
    jv.clear();
    jv.resize(nx_,0.0);
    for (uint n = 0; n < nx_; n++) {
      if ( n < nx_-1 ) {
        jv[n] += dx_/6.0*(2.0*v[n]+v[n+1]) + dt_*eps1_/dx_*(v[n]-v[n+1]); // Mass & stiffness
      }
      if ( n > 0 ) {
        jv[n] += dx_/6.0*(v[n-1]+2.0*v[n]) + dt_*eps1_/dx_*(v[n]-v[n-1]); // Mass & stiffness
      }
    }
    // Nonlinearity
    jv[nx_-1] += eps2_*dt_*4.0*std::pow(u[nx_-1],3.0)*v[nx_-1];
  }

  void apply_pde_hessian(std::vector<Real> &r, const std::vector<Real> &u, const std::vector<Real> &p, 
                         const std::vector<Real> &s) {
    r.clear();
    r.resize(nx_,0.0);
    // Contribution from nonlinearity
    r[nx_-1] = eps2_*dt_*12.0*std::pow(u[nx_-1],2.0)*p[nx_-1]*s[nx_-1];
  }

  void compute_residual(std::vector<Real> &r, const std::vector<Real> &up, 
                        const std::vector<Real> &u, const Real z) {
    r.clear();
    r.resize(nx_,0.0);
    for (uint n = 0; n < nx_; n++) {
      if ( n < nx_-1 ) {
        r[n] += dx_/6.0*(2.0*u[n]+u[n+1]) + dt_*eps1_/dx_*(u[n]-u[n+1]); // Mass & stiffness
        r[n] -= dx_/6.0*(2.0*up[n]+up[n+1]); // Previous time step
      }
      if ( n > 0 ) {
        r[n] += dx_/6.0*(u[n-1]+2.0*u[n]) + dt_*eps1_/dx_*(u[n]-u[n-1]); // Mass & stiffness
        r[n] -= dx_/6.0*(2.0*up[n]+up[n-1]); // Previous time step
      }
    }
    r[nx_-1] -= dt_*z; // Control
    r[nx_-1] += eps2_*dt_*std::pow(u[nx_-1],4.0); // Nonlinearity
  }

  Real compute_norm(const std::vector<Real> &r) {
    Real norm = 0.0;
    for (uint i = 0; i < r.size(); i++) {
      norm += r[i]*r[i];
    }
    return std::sqrt(norm);
  }

  void update(std::vector<Real> &u, const std::vector<Real> &s, const Real alpha=1.0) {
    for (uint i = 0; i < u.size(); i++) {
      u[i] += alpha*s[i];
    }
  }

  void linear_solve(std::vector<Real> &u, std::vector<Real> &d, std::vector<Real> &o, 
              const std::vector<Real> &r) {
    u.assign(r.begin(),r.end());
    // Perform LDL factorization
    Teuchos::LAPACK<int,Real> lp;
    int nx = static_cast<int>(nx_);
    int info;
    int ldb  = nx;
    int nhrs = 1;
    lp.PTTRF(nx,&d[0],&o[0],&info);
    lp.PTTRS(nx,nhrs,&d[0],&o[0],&u[0],ldb,&info);
  }

  void run_newton(std::vector<Real> &u, const std::vector<Real> &up, const Real z) {
    // Set initial guess
    u.assign(up.begin(),up.end());
    // Compute residual and residual norm
    std::vector<Real> r(u.size(),0.0);
    compute_residual(r,up,u,z);
    Real rnorm = compute_norm(r);
    // Define tolerances
    Real tol   = 1.e2*ROL::ROL_EPSILON;
    Real maxit = 100;
    // Initialize Jacobian storage
    std::vector<Real> d(nx_,0.0);
    std::vector<Real> o(nx_-1,0.0);
    // Iterate Newton's method
    Real alpha = 1.0, tmp = 0.0;
    std::vector<Real> s(nx_,0.0);
    std::vector<Real> utmp(nx_,0.0);
    for (uint i = 0; i < maxit; i++) {
      // Get Jacobian
      compute_pde_jacobian(d,o,u);
      // Solve Newton system
      linear_solve(s,d,o,r);
      // Perform line search
      tmp = rnorm;
      alpha = 1.0;
      utmp.assign(u.begin(),u.end());
      update(utmp,s,-alpha);
      compute_residual(r,up,utmp,z);
      rnorm = compute_norm(r); 
      while ( rnorm > (1.0-1.e-4*alpha)*tmp && alpha > std::sqrt(ROL::ROL_EPSILON) ) {
        alpha /= 2.0;
        utmp.assign(u.begin(),u.end());
        update(utmp,s,-alpha);
        compute_residual(r,up,utmp,z);
        rnorm = compute_norm(r); 
      }
      // Update iterate
      u.assign(utmp.begin(),utmp.end());
      if ( rnorm < tol ) {
        break;
      }
    }
  }

  Teuchos::RCP<const vector> getVector( const V& x ) {
    using Teuchos::dyn_cast;
    return dyn_cast<const SV>(x).getVector();
  }

  Teuchos::RCP<vector> getVector( V& x ) {
    using Teuchos::dyn_cast;
    return dyn_cast<SV>(x).getVector(); 
  }



/*************************************************************/
/********** END PRIVATE MEMBER FUNCTION DECLARATION **********/
/*************************************************************/

public:

  EqualityConstraint_ParabolicControl(Real eps = 1.0, int nx = 128, int nt = 100, Real T = 1) 
    : eps1_(eps), eps2_(1.0), nx_(nx), nt_(nt), T_(T) {
    u0_.resize(nx_,0.0);
    dx_ = 1.0/((Real)nx-1.0);
    dt_ = T/((Real)nt-1.0);
  }

  void value(ROL::Vector<Real> &c, const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol) {

    using Teuchos::RCP; 

    RCP<vector> cp = getVector(c);
    RCP<const vector> up = getVector(u);
    RCP<const vector> zp = getVector(z);

    std::vector<Real> C(nx_,0.0);
    std::vector<Real> uold(u0_);
    std::vector<Real> unew(u0_);

    for (uint t = 0; t < nt_; t++) {
      // Copy state and control at t time step
      for (uint n = 0; n < nx_; n++) {
        unew[n] = (*up)[t*nx_+n];
      }
      // Evaluate residual at t time step
      compute_residual(C,uold,unew,(*zp)[t]);
      // Copy residual at t time step
      for (uint n = 0; n < nx_; n++) {
        (*cp)[t*nx_+n] = C[n];
      }
      uold.assign(unew.begin(),unew.end());
    }
  }

  void solve(ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol) {

    using Teuchos::RCP;

    RCP<vector> up = getVector(u);
    RCP<const vector> zp = getVector(z);

    // Initialize State Storage
    std::vector<Real> uold(u0_);
    std::vector<Real> unew(u0_);
    // Time Step Using Implicit Euler
    for ( uint t = 0; t < nt_; t++ ) {
      run_newton(unew,uold,(*zp)[t]);
      for( uint n = 0; n < nx_; n++) {
        (*up)[t*nx_+n] = unew[n];
      }
      uold.assign(unew.begin(),unew.end());
    }
    /* TEST SOLVE -- ||c(u,z)|| SHOULD BE ZERO */
    //Teuchos::RCP<ROL::Vector<Real> > c = u.clone();
    //value(*c,u,z,tol);
    //std::cout << c->norm() << "\n";
  }

  void applyJacobian_1(ROL::Vector<Real> &jv, const ROL::Vector<Real> &v, const ROL::Vector<Real> &u,
                       const ROL::Vector<Real> &z, Real &tol) {

    using Teuchos::RCP;
    
    RCP<vector> jvp = getVector(jv);
    RCP<const vector> vp = getVector(v);
    RCP<const vector> up = getVector(u);
    RCP<const vector> zp = getVector(z);

    std::vector<Real> J(u0_.size(),0.0);
    std::vector<Real> M(u0_.size(),0.0);
    std::vector<Real> vold(u0_);
    std::vector<Real> unew(u0_);
    std::vector<Real> vnew(u0_);

    for (uint t = 0; t < nt_; t++) {
      for (uint n = 0; n < nx_; n++) {
        unew[n] = (*up)[t*nx_+n];
        vnew[n] = (*vp)[t*nx_+n];
      }
      apply_pde_jacobian(J,vnew,unew);
      if ( t > 0 ) {
        apply_mass(M,vold);
      }
      for (uint n = 0; n < nx_; n++) {
        (*jvp)[t*nx_+n] = J[n] - M[n];
      }
      vold.assign(vnew.begin(),vnew.end());
    }
  }

  void applyInverseJacobian_1(ROL::Vector<Real> &jv, const ROL::Vector<Real> &v, const ROL::Vector<Real> &u,
                              const ROL::Vector<Real> &z, Real &tol) {

    using Teuchos::RCP;
 
    RCP<vector> jvp = getVector(jv);
    RCP<const vector> vp = getVector(v);
    RCP<const vector> up = getVector(u);
    RCP<const vector> zp = getVector(z);
   
    // Initialize State Storage
    std::vector<Real> M(u0_);
    std::vector<Real> sold(u0_);
    std::vector<Real> unew(u0_);
    std::vector<Real> vnew(u0_);
    std::vector<Real> snew(u0_);
    std::vector<Real> d(nx_,0.0);
    std::vector<Real> r(nx_,0.0);
    std::vector<Real> o(nx_-1,0.0);

    // Time Step Using Implicit Euler
    for (uint t = 0; t < nt_; t++) {
      for (uint n = 0; n < nx_; n++) {
        unew[n] = (*up)[t*nx_+n];
        vnew[n] = (*vp)[t*nx_+n];
      }
      // Get PDE Jacobian
      compute_pde_jacobian(d,o,unew);
      // Get Right Hand Side
      if ( t > 0 ) {
        apply_mass(M,sold); 
        update(vnew,M);
      }
      // Solve solve adjoint system at current time step
      linear_solve(snew,d,o,vnew);
      for(uint n = 0; n < nx_; n++) {
        (*jvp)[t*nx_+n] = snew[n];
      }
      sold.assign(snew.begin(),snew.end());
    }
  }

  void applyAdjointJacobian_1(ROL::Vector<Real> &jv, const ROL::Vector<Real> &v, const ROL::Vector<Real> &u,
                              const ROL::Vector<Real> &z, Real &tol) {


    using Teuchos::RCP;

    RCP<vector> jvp = getVector(jv);
    RCP<const vector> vp = getVector(v);
    RCP<const vector> up = getVector(u);
    RCP<const vector> zp = getVector(z);

    std::vector<Real> J(u0_.size(),0.0);
    std::vector<Real> M(u0_.size(),0.0);
    std::vector<Real> vold(u0_);
    std::vector<Real> unew(u0_);
    std::vector<Real> vnew(u0_);

    for (uint t = nt_; t > 0; t--) {
      for (uint n = 0; n < nx_; n++) {
        unew[n] = (*up)[(t-1)*nx_+n];
        vnew[n] = (*vp)[(t-1)*nx_+n];
      }
      apply_pde_jacobian(J,vnew,unew);
      if ( t < nt_ ) {
        apply_mass(M,vold);
      }
      for (uint n = 0; n < nx_; n++) {
        (*jvp)[(t-1)*nx_+n] = J[n] - M[n];
      }
      vold.assign(vnew.begin(),vnew.end());
    }
  }

  void applyInverseAdjointJacobian_1(ROL::Vector<Real> &jv, const ROL::Vector<Real> &v, 
                                     const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol) {
    using Teuchos::RCP;

    RCP<vector> jvp = getVector(jv);
    RCP<const vector> vp = getVector(v);
    RCP<const vector> up = getVector(u);
    RCP<const vector> zp = getVector(z);

    // Initialize State Storage
    std::vector<Real> M(u0_);
    std::vector<Real> sold(u0_);
    std::vector<Real> unew(u0_);
    std::vector<Real> vnew(u0_);
    std::vector<Real> snew(u0_);
    std::vector<Real> d(nx_,0.0);
    std::vector<Real> r(nx_,0.0);
    std::vector<Real> o(nx_-1,0.0);

    // Time Step Using Implicit Euler
    for (uint t = nt_; t > 0; t--) {
      for (uint n = 0; n < nx_; n++) {
        unew[n] = (*up)[(t-1)*nx_+n];
        vnew[n] = (*vp)[(t-1)*nx_+n];
      }
      // Get PDE Jacobian
      compute_pde_jacobian(d,o,unew);
      // Get Right Hand Side
      if ( t < nt_ ) {
        apply_mass(M,sold); 
        update(vnew,M);
      }
      // Solve solve adjoint system at current time step
      linear_solve(snew,d,o,vnew);
      for (uint n = 0; n < nx_; n++) {
        (*jvp)[(t-1)*nx_+n] = snew[n];
      }
      sold.assign(snew.begin(),snew.end());
    }
  }

  void applyJacobian_2(ROL::Vector<Real> &jv, const ROL::Vector<Real> &v, const ROL::Vector<Real> &u,
                       const ROL::Vector<Real> &z, Real &tol) {

    using Teuchos::RCP;

    RCP<vector> jvp = getVector(jv);
    RCP<const vector> vp = getVector(v);
    RCP<const vector> up = getVector(u);
    RCP<const vector> zp = getVector(z);

    jv.zero();
    for (uint t = 0; t < nt_; t++) {
      (*jvp)[t*nx_+(nx_-1)] = -dt_*(*vp)[t];
    }
  }

  void applyAdjointJacobian_2(ROL::Vector<Real> &jv, const ROL::Vector<Real> &v, const ROL::Vector<Real> &u,
                              const ROL::Vector<Real> &z, Real &tol) {

    using Teuchos::RCP;
 
    RCP<vector> jvp = getVector(jv);
    RCP<const vector> vp = getVector(v);
    RCP<const vector> up = getVector(u);
    RCP<const vector> zp = getVector(z);

    for (uint t = 0; t < nt_; t++) {
      (*jvp)[t] = -dt_*(*vp)[t*nx_+(nx_-1)];
    }
  }

  void applyAdjointHessian_11(ROL::Vector<Real> &hwv, const ROL::Vector<Real> &w, 
                              const ROL::Vector<Real> &v, const ROL::Vector<Real> &u,
                              const ROL::Vector<Real> &z, Real &tol) {

    using Teuchos::RCP;

    RCP<vector> hwvp = getVector(hwv);
    RCP<const vector> wp = getVector(w);
    RCP<const vector> vp = getVector(v);
    RCP<const vector> up = getVector(u);
    RCP<const vector> zp = getVector(z);

    // Initialize State Storage
    std::vector<Real> unew(u0_);
    std::vector<Real> wnew(u0_);
    std::vector<Real> vnew(u0_);
    std::vector<Real> snew(u0_);

    // Time Step Using Implicit Euler
    for (uint t = nt_; t > 0; t--) {
      for (uint n = 0; n < nx_; n++) {
        unew[n] = (*up)[(t-1)*nx_+n];
        vnew[n] = (*vp)[(t-1)*nx_+n];
        wnew[n] = (*wp)[(t-1)*nx_+n];
      }
      // Get PDE Hessian
      apply_pde_hessian(snew,unew,wnew,vnew);
      for(uint n = 0; n < nx_; n++) {
        (*hwvp)[(t-1)*nx_+n] = snew[n];
      }
    }
  }

  void applyAdjointHessian_12(ROL::Vector<Real> &hwv, const ROL::Vector<Real> &w, 
                              const ROL::Vector<Real> &v, const ROL::Vector<Real> &u,
                              const ROL::Vector<Real> &z, Real &tol) {
    hwv.zero();
  }

  void applyAdjointHessian_21(ROL::Vector<Real> &hwv, const ROL::Vector<Real> &w, 
                              const ROL::Vector<Real> &v, const ROL::Vector<Real> &u,
                              const ROL::Vector<Real> &z, Real &tol) {
    hwv.zero();
  }

  void applyAdjointHessian_22(ROL::Vector<Real> &hwv, const ROL::Vector<Real> &w, 
                              const ROL::Vector<Real> &v, const ROL::Vector<Real> &u,
                              const ROL::Vector<Real> &z, Real &tol) {
    hwv.zero();
  }
};

template<class Real>
class Objective_ParabolicControl : public ROL::Objective_SimOpt<Real> {

  typedef std::vector<Real>    vector;
  typedef ROL::Vector<Real>    V;
  typedef ROL::StdVector<Real> SV;
  
  typedef typename vector::size_type uint; 

private:
  Real alpha_;
  uint nx_;
  uint nt_;
  Real dx_;
  Real dt_;
  Real T_;

/***************************************************************/
/********** BEGIN PRIVATE MEMBER FUNCTION DECLARATION **********/
/***************************************************************/
  void apply_mass(std::vector<Real> &Mu, const std::vector<Real> &u ) {
    Mu.resize(nx_,0.0);
    for (uint n = 0; n < nx_; n++) {
      if ( n < nx_-1 ) {
        Mu[n] += dx_/6.0*(2.0*u[n] + u[n+1]);
      }
      if ( n > 0 ) {
        Mu[n] += dx_/6.0*(u[n-1] + 2.0*u[n]);
      }
    }
  }

  Real evaluate_target(Real x) {
    Real val = 0.0;
    int example = 2;
    switch (example) {
      case 1:  val = ((x<0.5) ? 0.5 : 0.0); break;
      case 2:  val = 0.5; break;
      case 3:  val = 0.5*std::abs(std::sin(8.0*M_PI*x)); break;
      case 4:  val = 0.5*std::exp(-0.5*(x-0.5)*(x-0.5)); break;
    }
    return val;
  }

  Real compute_dot(const std::vector<Real> &r, const std::vector<Real> &s) {
    Real ip = 0.0; 
    for (uint i = 0; i < r.size(); i++) {
      ip += r[i]*s[i];
    }
    return ip;
  }

  Real compute_norm(const std::vector<Real> &r) {
    return std::sqrt(compute_dot(r,r));
  }

  Real compute_weighted_dot(const std::vector<Real> &r, const std::vector<Real> &s) {
    std::vector<Real> Mr(nx_,0.0);
    apply_mass(Mr,r);
    return compute_dot(Mr,s);
  }

  Real compute_weighted_norm(const std::vector<Real> &r) {
    return std::sqrt(compute_weighted_dot(r,r));
  }

  void update(std::vector<Real> &u, const std::vector<Real> &s, const Real alpha=1.0) {
    for (uint i = 0; i < u.size(); i++) {
      u[i] += alpha*s[i];
    }
  }

  Teuchos::RCP<const vector> getVector( const V& x ) {
    using Teuchos::dyn_cast;
    return dyn_cast<const SV>(x).getVector();
  }

  Teuchos::RCP<vector> getVector( V& x ) {
    using Teuchos::dyn_cast;
    return dyn_cast<SV>(x).getVector(); 
  }

/*************************************************************/
/********** END PRIVATE MEMBER FUNCTION DECLARATION **********/
/*************************************************************/

public:

  Objective_ParabolicControl(Real alpha = 1.e-4, int nx = 128, int nt = 100, Real T = 1) 
    : alpha_(alpha), nx_(nx), nt_(nt), T_(T) {
    dx_ = 1.0/((Real)nx-1.0);
    dt_ = T/((Real)nt-1.0);
  }

  Real value( const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol ) {

    using Teuchos::RCP;

    RCP<const vector> up = getVector(u);
    RCP<const vector> zp = getVector(z);

    // Compute Norm of State
    std::vector<Real> uT(nx_,0.0);
    for (uint n = 0; n < nx_; n++) {
      uT[n] = (*up)[(nt_-1)*nx_ + n] - evaluate_target((Real)n*dx_);
    } 
    Real val = 0.5*compute_weighted_dot(uT,uT); 

    // Add Norm of Control
    for (uint t = 0; t < nt_; t++) {
      val += 0.5*alpha_*dt_*(*zp)[t]*(*zp)[t];
    }
    return val;
  }

  void gradient_1( ROL::Vector<Real> &g, const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol ) {
    g.zero();

    using Teuchos::RCP;
  
    RCP<vector> gp = getVector(g);
    RCP<const vector> up = getVector(u);
    RCP<const vector> zp = getVector(z);

    std::vector<Real> uT(nx_,0.0);
    for (uint n = 0; n < nx_; n++) {
      uT[n] = (*up)[(nt_-1)*nx_ + n] - evaluate_target((Real)n*dx_);
    } 
    std::vector<Real> M(nx_,0.0);
    apply_mass(M,uT);
    for (uint n = 0; n < nx_; n++) {
      (*gp)[(nt_-1)*nx_ + n] = M[n];
    }
  }

  void gradient_2( ROL::Vector<Real> &g, const ROL::Vector<Real> &u, const ROL::Vector<Real> &z, Real &tol ) {
    g.zero();

    using Teuchos::RCP;

    RCP<vector> gp = getVector(g);
    RCP<const vector> up = getVector(u);
    RCP<const vector> zp = getVector(z);
 
    // Compute gradient
    for (uint n = 0; n < nt_; n++) {
      (*gp)[n] = dt_*alpha_*(*zp)[n];
    }
  }

  void hessVec_11( ROL::Vector<Real> &hv, const ROL::Vector<Real> &v, const ROL::Vector<Real> &u, 
                   const ROL::Vector<Real> &z, Real &tol ) {

    using Teuchos::RCP;
    
    RCP<vector> hvp = getVector(hv);
    RCP<const vector> up = getVector(u);
    RCP<const vector> zp = getVector(z);
    RCP<const vector> vp = getVector(v);

    // Compute HessVec
    std::vector<Real> vT(nx_,0.0);
    for (uint n = 0; n < nx_; n++) {
      vT[n] = (*vp)[(nt_-1)*nx_ + n];
    } 
    std::vector<Real> M(nx_,0.0);
    apply_mass(M,vT);
    for (uint n = 0; n < nx_; n++) {
      (*hvp)[(nt_-1)*nx_ + n] = M[n];
    }
  }

  void hessVec_12( ROL::Vector<Real> &hv, const ROL::Vector<Real> &v, const ROL::Vector<Real> &u, 
                   const ROL::Vector<Real> &z, Real &tol ) {
    hv.zero();
  }

  void hessVec_21( ROL::Vector<Real> &hv, const ROL::Vector<Real> &v, const ROL::Vector<Real> &u, 
                   const ROL::Vector<Real> &z, Real &tol ) {
    hv.zero();
  }

  void hessVec_22( ROL::Vector<Real> &hv, const ROL::Vector<Real> &v, const ROL::Vector<Real> &u, 
                   const ROL::Vector<Real> &z, Real &tol ) {

    using Teuchos::RCP;

    RCP<vector> hvp = getVector(hv);
    RCP<const vector> up = getVector(u);
    RCP<const vector> zp = getVector(z);
    RCP<const vector> vp = getVector(v);

    // Compute HessVec
    for (uint n = 0; n < nt_; n++) {
      (*hvp)[n] = dt_*alpha_*(*vp)[n];
    }
  }
};

typedef double RealT;

int main(int argc, char *argv[]) {

  typedef std::vector<RealT>    vector;
  typedef ROL::Vector<RealT>    V;
  typedef ROL::StdVector<RealT> SV;
   
  typedef typename vector::size_type uint;

  using Teuchos::RCP;  using Teuchos::rcp;

  Teuchos::GlobalMPISession mpiSession(&argc, &argv);

  // This little trick lets us print to std::cout only if a (dummy) command-line argument is provided.
  int iprint     = argc - 1;

  RCP<std::ostream> outStream;
  Teuchos::oblackholestream bhs; // outputs nothing
  if (iprint > 0)
    outStream = rcp(&std::cout, false);
  else
    outStream = rcp(&bhs, false);

  int errorFlag  = 0;

  // *** Example body.

  try {
    // Initialize objective function.
    uint nx     = 40;    // Set spatial discretization.
    uint nt     = 40;    // Set temporal discretization.
    RealT T     = 1.0;   // Set end time.
    RealT alpha = 1.e-3; // Set penalty parameter.
    RealT eps   = 5.e-1; // Set conductivity 
    Objective_ParabolicControl<RealT> obj(alpha,nx,nt,T);
    EqualityConstraint_ParabolicControl<RealT> con(eps,nx,nt,T);

    // Initialize iteration vectors.
    RCP<vector> xz_rcp = rcp( new vector(nt, 1.0) );
    RCP<vector> xu_rcp = rcp( new vector(nx*nt, 1.0) );
    RCP<vector> gz_rcp = rcp( new vector(nt, 1.0) );
    RCP<vector> gu_rcp = rcp( new vector(nx*nt, 1.0) );
    RCP<vector> yz_rcp = rcp( new vector(nt, 1.0) );
    RCP<vector> yu_rcp = rcp( new vector(nx*nt, 1.0) );

    for (uint i=0; i<nt; i++) {
      (*xz_rcp)[i] = (RealT)rand()/(RealT)RAND_MAX;
      (*yz_rcp)[i] = (RealT)rand()/(RealT)RAND_MAX;
      for (uint n=0; n<nx; n++) {
        (*xu_rcp)[i*nx + n] = (RealT)rand()/(RealT)RAND_MAX;
        (*yu_rcp)[i*nx + n] = (RealT)rand()/(RealT)RAND_MAX;
      }
    }

    SV xz(xz_rcp);
    SV xu(xu_rcp);
    SV gz(gz_rcp);
    SV gu(gu_rcp);
    SV yz(yz_rcp);
    SV yu(yu_rcp);

    RCP<V> xzp = rcp(&xz,false);
    RCP<V> xup = rcp(&xu,false);
    RCP<V> gzp = rcp(&gz,false);
    RCP<V> gup = rcp(&gu,false);

    RCP<V> yzp = rcp(&yz,false);
    RCP<V> yup = rcp(&yu,false);

    ROL::Vector_SimOpt<RealT> x(xup,xzp);
    ROL::Vector_SimOpt<RealT> g(gup,gzp);
    ROL::Vector_SimOpt<RealT> y(yup,yzp);

    RCP<vector> c_rcp  = rcp( new vector(nt*nx, 0.0) );
    RCP<vector> l_rcp  = rcp( new vector(nt*nx, 0.0) );

    SV c(c_rcp);
    SV l(l_rcp);

    RCP<V> cp = rcp(&c,false);

    // Initialize reduced objective function
    RCP<ROL::Objective_SimOpt<RealT> > pobj = rcp(&obj,false);
    RCP<ROL::EqualityConstraint_SimOpt<RealT> > pcon = rcp(&con,false);
    ROL::Reduced_Objective_SimOpt<RealT> robj(pobj,pcon,xup,cp);

    // Check deriatives.
    obj.checkGradient(x,y,true,*outStream);
    obj.checkHessVec(x,y,true,*outStream);
    con.checkApplyJacobian(x,y,c,true,*outStream);

    //con.checkApplyAdjointJacobian(x,yu,c,x,true);
    con.checkApplyAdjointHessian(x,yu,y,x,true,*outStream);
    robj.checkGradient(xz,yz,true,*outStream);
    robj.checkHessVec(xz,yz,true,*outStream);

    // Initialize constraints -- these are set to -infinity and infinity.
    RCP<vector> lo_rcp = rcp( new vector(nt,-1.e16) );
    RCP<vector> hi_rcp = rcp( new vector(nt, 1.e16) );

    RCP<V> lo = rcp( new SV(lo_rcp) );
    RCP<V> hi = rcp( new SV(hi_rcp) );

    ROL::BoundConstraint<RealT> icon(lo,hi);

    // Primal dual active set.
    std::string filename = "input.xml";
    Teuchos::RCP<Teuchos::ParameterList> parlist = Teuchos::rcp( new Teuchos::ParameterList() );
    Teuchos::updateParametersFromXmlFile( filename, parlist.ptr() );
    // Krylov parameters.
    parlist->sublist("General").sublist("Krylov").set("Absolute Tolerance",1.e-8);
    parlist->sublist("General").sublist("Krylov").set("Relative Tolerance",1.e-4);
    parlist->sublist("General").sublist("Krylov").set("Iteration Limit",50);
    // PDAS parameters.
    parlist->sublist("Step").sublist("Primal Dual Active Set").set("Relative Step Tolerance",1.e-8);
    parlist->sublist("Step").sublist("Primal Dual Active Set").set("Relative Gradient Tolerance",1.e-6);
    parlist->sublist("Step").sublist("Primal Dual Active Set").set("Iteration Limit", 10);
    parlist->sublist("Step").sublist("Primal Dual Active Set").set("Dual Scaling",(alpha>0.0)?alpha:1.e-4);
    parlist->sublist("General").sublist("Secant").set("Use as Hessian",true);
    // Status test parameters.
    parlist->sublist("Status Test").set("Gradient Tolerance",1.e-12);
    parlist->sublist("Status Test").set("Step Tolerance",1.e-14);
    parlist->sublist("Status Test").set("Iteration Limit",100);
    // Define algorithm.
    RCP<ROL::Algorithm<RealT> > algo
      = rcp(new ROL::Algorithm<RealT>("Primal Dual Active Set",*parlist,false));
    // Run algorithm.
    xz.zero();
    std::clock_t timer_pdas = std::clock();
    algo->run(xz, robj, icon, true, *outStream);
    *outStream << "Primal Dual Active Set required " << (std::clock()-timer_pdas)/(RealT)CLOCKS_PER_SEC 
               << " seconds.\n";

    // Projected Newton.
    // re-load parameters
    Teuchos::updateParametersFromXmlFile( filename, parlist.ptr() );
    // Set algorithm.
    algo = rcp(new ROL::Algorithm<RealT>("Trust Region",*parlist,false));
    // Run Algorithm
    xz.zero();
    std::clock_t timer_tr = std::clock();
    algo->run(xz, robj, icon, true, *outStream);
    *outStream << "Projected Newton required " << (std::clock()-timer_tr)/(RealT)CLOCKS_PER_SEC 
               << " seconds.\n";

    // Composite step.
    parlist->sublist("Status Test").set("Gradient Tolerance",1.e-12);
    parlist->sublist("Status Test").set("Constraint Tolerance",1.e-10);
    parlist->sublist("Status Test").set("Step Tolerance",1.e-14);
    parlist->sublist("Status Test").set("Iteration Limit",100);
    // Set algorithm.
    algo = rcp(new ROL::Algorithm<RealT>("Composite Step",*parlist,false));
    x.zero();
    std::clock_t timer_cs = std::clock();
    algo->run(x, g, l, c, obj, con, true, *outStream);
    *outStream << "Composite Step required " << (std::clock()-timer_cs)/(RealT)CLOCKS_PER_SEC 
               << " seconds.\n";
  }
  catch (std::logic_error err) {
    *outStream << err.what() << "\n";
    errorFlag = -1000;
  }; // end try

  if (errorFlag != 0)
    std::cout << "End Result: TEST FAILED\n";
  else
    std::cout << "End Result: TEST PASSED\n";

  return 0;

}

