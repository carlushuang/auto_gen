#pragma once 

#include "BasicClass.h" 

/************************************************************************/
/* FLAT                                                                */
/************************************************************************/
typedef struct ExtFlatSolutionConfigTpye
{
}T_ExtFlatSolutionConfig;

typedef struct ExtFlatProblemConfigType
{
	size_t VectorSize;
	float *h_a, *h_b, *h_c, *c_ref;
}T_ExtFlatProblemConfig;

/************************************************************************/
/* DS	                                                                */
/************************************************************************/
typedef struct ExtDsSolutionConfigTpye
{
}T_ExtDsSolutionConfig;

typedef struct ExtDsProblemConfigType
{
	size_t VectorSize;
	float *h_a, *h_b, *h_c, *c_ref;
}T_ExtDsProblemConfig;

/************************************************************************/
/* SMEM                                                                 */
/************************************************************************/
typedef struct ExtSmemSolutionConfigTpye
{
}T_ExtSmemSolutionConfig;

typedef struct ExtSmemProblemConfigType
{
	size_t VectorSize;
	float *h_a, *h_b, *h_c, *c_ref;
}T_ExtSmemProblemConfig;

/************************************************************************/
/* MUBUF                                                                */
/************************************************************************/
typedef struct ExtMubufSolutionConfigTpye
{
}T_ExtMubufSolutionConfig;

typedef struct ExtMubufProblemConfigType
{
	size_t VectorSize;
	float *h_a, *h_b, *h_c, *c_ref;
}T_ExtMubufProblemConfig;
