/*====================================================================
 * ------------------------
 * | CVS File Information |
 * ------------------------
 *
 * $RCSfile$
 *
 * $Author$
 *
 * $Date$
 *
 * $Revision$
 *
 *====================================================================*/
#ifndef __LB_CONST_H
#define __LB_CONST_H

#ifndef lint
static char *cvs_lbconsth_id = "$Id$";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>

#include "lbi_const.h"

/*
 *  See bottom for other included files.
 */

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*
 *  Type definitions.
 */

/*
 * Strings to define the library name, and the version number
 * so that it is easier to keep track of what code each user
 * has.
 */
#define UTIL_NAME "zoltan"
#define LB_VER   1.02


/*
 * Type used to store linked list of new values for parameters.
 */
   
typedef struct LB_Param {
  char *name;
  char *new_val;
  struct LB_Param *next;
} LB_PARAM;
	  


#ifndef TRUE
#define FALSE (0)
#define TRUE  (1)
#endif /* !TRUE */

typedef struct LB_Struct LB;
typedef struct LB_Tag_Struct LB_TAG;

typedef int LB_FN(LB *, int*, LB_GID**, LB_LID**, int **);

/*
 *  Define the possible load balancing methods allowed.
 */

typedef enum LB_Method {
  NONE = -1,
  RCB,
  OCTPART,
  WHEAT,
  PARMETIS_PART,
  PARMETIS_REPART,
  PARMETIS_REFINE,
  LB_MAX_METHODS                  /*  This entry should always be last.      */
} LB_METHOD;

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

/*
 *  Define a data structure for object information.
 */

struct LB_Tag_Struct {
  LB_GID Global_ID;               /* Global ID for the object; provided by 
                                     the application.                        */
  LB_LID Local_ID;                /* Local ID for the object; the application
                                     determines what is meant by "local ID";
                                     the load-balancer stores this field only
                                     so the application can take advantage of
                                     local indexing in the query routines.   */
  int   Proc;                     /* A processor ID for the tag.  Could be 
                                     the destination of an object to be 
                                     exported or the source of an object to
                                     be imported.                            */
};

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

/* 
 *  Define a communication structure for load balancing results.  This
 *  structure will be used by Steve Plimpton's and Bruce Hendrickson's
 *  communication library.
 */

struct LB_Migrate_Struct {
  int Help_Migrate;                   /*  Flag indicating whether the load
                                          balancer should help the application
                                          migrate data.  Some applications may
                                          prefer to do it themselves.        */
  /*
   *  Pointers to routines that depend on the application.
   */

  LB_PRE_MIGRATE_FN *Pre_Process;      /* Function that performs application
                                          specific pre-processing.  Optional
                                          for help-migration.                */
  void *Pre_Process_Data;              /* Ptr to user defined data to be
                                          passed to Pre_Process()            */
  LB_OBJ_SIZE_FN *Get_Obj_Size;        /* Function that returns the size of
                                          contiguous memory needed to store
                                          the data for a single object for
                                          migration.                         */
  void *Get_Obj_Size_Data;             /* Ptr to user defined data to be
                                          passed to Get_Obj_Size()           */
  LB_PACK_OBJ_FN *Pack_Obj;            /* Routine that packs object data for
                                          a given object into contiguous 
                                          memory for migration.              */
  void *Pack_Obj_Data;                 /* Ptr to user defined data to be
                                          passed to Pack_Obj()               */
  LB_UNPACK_OBJ_FN *Unpack_Obj;        /* Routine that unpacks object data for
                                          a given object from contiguous 
                                          memory after migration.            */
  void *Unpack_Obj_Data;               /* Ptr to user defined data to be
                                          passed to Unpack_Obj()             */
};

typedef struct LB_Migrate_Struct LB_MIGRATE;

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


/*
 *  Define a load balancing object.  It will contain pointers to the
 *  appropriate functions for interfacing with applications and 
 *  pointers to the data structure used for load balancing.
 */

struct LB_Struct {
  MPI_Comm Communicator;          /*  The MPI Communicator.                  */
  int Proc;                       /*  The processor's ID within the MPI
                                      Communicator.                          */
  int Num_Proc;                   /*  The number of processors in the MPI
                                      Communicator.                          */
  int Debug;                      /*  Debug level for this instance of
                                      load balancing.                        */
  LB_METHOD Method;               /*  Method to be used for load balancing.  */
  LB_FN *LB_Fn;                   /*  Pointer to the function that performs
                                      the load balancing; this ptr is set
                                      based on the method used.              */
  LB_PARAM *Params;               /*  List of parameter names & new vals */
  double Tolerance;               /*  Tolerance to which to load balance;
                                      tolerance = 0.9 implies 10% imbalance
                                      is acceptable.                         */
  void *Data_Structure;           /*  Data structure used by the load 
                                      balancer; cast by the method routines
                                      to the appropriate data type.          */
  LB_NUM_EDGES_FN *Get_Num_Edges;              /* Fn ptr to get an object's
                                                  number of edges.           */
  void *Get_Num_Edges_Data;                    /* Ptr to user defined data
                                                  to be passed to
                                                  Get_Num_Edges()            */
  LB_EDGE_LIST_FN *Get_Edge_List;              /* Fn ptr to get an object's
                                                  edge list.                 */
  void *Get_Edge_List_Data;                    /* Ptr to user defined data
                                                  to be passed to
                                                  Get_Edge_List()            */
  LB_NUM_GEOM_FN *Get_Num_Geom;                /* Fn ptr to get an object's
                                                  number of geometry values. */
  void *Get_Num_Geom_Data;                     /* Ptr to user defined data
                                                  to be passed to
                                                  Get_Num_Geom()             */
  LB_GEOM_FN *Get_Geom;                        /* Fn ptr to get an object's
                                                  geometry values.           */
  void *Get_Geom_Data;                         /* Ptr to user defined data
                                                  to be passed to
                                                  Get_Geom()                 */
  LB_NUM_OBJ_FN *Get_Num_Obj;                  /* Fn ptr to get a proc's  
                                                  number of local objects.   */
  void *Get_Num_Obj_Data;                      /* Ptr to user defined data
                                                  to be passed to
                                                  Get_Num_Obj()              */
  LB_OBJ_LIST_FN *Get_Obj_List;                /* Fn ptr to get all local
                                                  objects on a proc.         */
  void *Get_Obj_List_Data;                     /* Ptr to user defined data
                                                  to be passed to
                                                  Get_Obj_List()             */
  LB_FIRST_OBJ_FN *Get_First_Obj;              /* Fn ptr to get the first   
                                                  local obj on a proc.       */
  void *Get_First_Obj_Data;                    /* Ptr to user defined data
                                                  to be passed to
                                                  Get_First_Obj()            */
  LB_NEXT_OBJ_FN *Get_Next_Obj;                /* Fn ptr to get the next   
                                                  local obj on a proc.       */
  void *Get_Next_Obj_Data;                     /* Ptr to user defined data
                                                  to be passed to
                                                  Get_Next_Obj()             */
  LB_NUM_BORDER_OBJ_FN *Get_Num_Border_Obj;    /* Fn ptr to get a proc's 
                                                  number of border objs wrt
                                                  a given processor.         */
  void *Get_Num_Border_Obj_Data;               /* Ptr to user defined data
                                                  to be passed to
                                                  Get_Num_Border_Obj()       */
  LB_BORDER_OBJ_LIST_FN *Get_Border_Obj_List;  /* Fn ptr to get all objects
                                                  sharing a border with a
                                                  given processor.           */
  void *Get_Border_Obj_List_Data;              /* Ptr to user defined data
                                                  to be passed to
                                                  Get_Border_Obj_List()      */
  LB_FIRST_BORDER_OBJ_FN *Get_First_Border_Obj;/* Fn ptr to get the first 
                                                  object sharing a border 
                                                  with a given processor.    */
  void *Get_First_Border_Obj_Data;             /* Ptr to user defined data
                                                  to be passed to
                                                  Get_First_Border_Obj()     */
  LB_NEXT_BORDER_OBJ_FN *Get_Next_Border_Obj;  /* Fn ptr to get the next 
                                                  object sharing a border 
                                                  with a given processor.    */
  void *Get_Next_Border_Obj_Data;              /* Ptr to user defined data
                                                  to be passed to
                                                  Get_Next_Border_Obj()      */
  LB_MIGRATE Migrate;                          /* Struct with info for helping
                                                  with migration.            */
  void *Migrate_Data;                          /* Ptr to user defined data
                                                  to be passed to
                                                  Migrate()                  */
};

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/* MACROS  */

/*
 *  Test whether the processor is in the given load-balancing object's
 *  communicator.  Used to exit from balancing routines for processors
 *  that are not included in the load-balancing communicator.
 */

#define LB_PROC_NOT_IN_COMMUNICATOR(lb) ((lb)->Proc == -1) 

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/* PROTOTYPES */

extern LB_FN LB_rcb;
extern LB_FN lb_wheat;
extern LB_FN LB_octpart;
extern LB_FN LB_ParMetis_Part;

#include "par_const.h"

#endif
