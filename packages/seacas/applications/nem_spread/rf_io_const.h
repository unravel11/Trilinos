/*
 * Copyright (C) 2009-2017, 2020 National Technology & Engineering Solutions of
 * Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of NTESS nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef RF_IO_CONST_H
#define RF_IO_CONST_H

#include <string>
#include <vector>

/*********** rf_io_const.h -- constants for external IO purposes**************/

/* Restart structure */
template <typename T> struct Restart_Description
{
  Restart_Description() = default;

  int Flag{-1}; /* Indicates whether restart info is to be processed */

  int              Num_Times{-1}; /* The number of time indices to spread */
  std::vector<int> Time_Idx{};    /* Time indices to read, need to keep track of all */
  T                Time{};        /* time value */

  int NVar_Glob{-1}; /* Number of global variables read */
  int NVar_Elem{-1}; /* Number of elemental variables read */
  int NVar_Node{-1}; /* Number of nodal variables read */
  int NVar_Nset{-1}; /* Number of nodeset variables read */
  int NVar_Sset{-1}; /* Number of sideset variables read */

  std::vector<int> GElem_TT{}; /* Global Elemental variable truth table */
  std::vector<int> GNset_TT{}; /* Global Elemental variable truth table */
  std::vector<int> GSset_TT{}; /* Global Elemental variable truth table */

  /*
   * To be able to support single or double precision exodus files,
   * need to have both float and double pointers here.
   */
  std::vector<T> Glob_Vals{};            /* Global variable values, only one per variable *
                                          * and processor                                 */
  std::vector<std::vector<T>> Elem_Vals; /* Element variable values for each processor */
  std::vector<std::vector<T>> Node_Vals; /* Nodal variable values for each processor */
  std::vector<std::vector<T>> Nset_Vals; /* Nodeset variable values for each processor */
  std::vector<std::vector<T>> Sset_Vals; /* Sideset variable values for each processor */

  char **NV_Name{nullptr};  /* Names of the nodal variables */
  char **EV_Name{nullptr};  /* Names of the elemental variables */
  char **GV_Name{nullptr};  /* Names of the global variables */
  char **NSV_Name{nullptr}; /* Names of the nodeset variables */
  char **SSV_Name{nullptr}; /* Names of the sideset variables */
};

/*****************************************************************************/
/*      EXTERN STATEMENTS for GLOBALS USED IN I/O ROUTINES                   */
/*****************************************************************************/

/**Extern statements for parameters in rf_io.h */

extern std::string ExoFile;               /* Exodus II File containing problem definition.   */
                                          /* This name is the root name.                     */
extern std::string Output_File_Base_Name; /* Base name of output file. If it has a suffix, it will
                                               be stripped */

extern std::string Exo_LB_File;
/* Exodus II file containing the mesh load-balance */
/* information                                     */
extern std::string Exo_Res_File;
/* Exodus II file containing the mesh result       */
/* information                                     */
extern int Debug_Flag; /* Flag to specify debug info is to be printed out.
                          The value of this flag determines the level of
                          diagnostic info which is printed to stdout
                          Debug_Flag == 0       No output
                                        .
                                        .
                                        9       maximum output             */
extern int Gen_Flag;   /* Flag used by nem_join to determine if the user
                          wants to use an existing genesis file rather
                          than generating one from the parallel files */

extern int Num_Nod_Var;  /* The number of nodal variables to reserve */
                         /* space in the output file for. */
extern int Num_Elem_Var; /* The number of elemental variables to */
                         /* reserve space in the output file for. */

extern int Num_Glob_Var; /* The number of global variables to reserve */
                         /* space in the output file for. */
extern int Num_Nset_Var; /* The number of nodeset variables to reserve */
                         /* space in the output file for. */

extern int Num_Sset_Var; /* The number of sideset variables to reserve */
                         /* space in the output file for. */

#endif
