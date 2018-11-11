#pragma once 

#include "BasicClass.h" 

/************************************************************************/
/* Õ®”√			                                                        */
/************************************************************************/
typedef struct ExtSolutionConfigTpye
{
	std::list<T_KernelArgu> * KernelArgus;
}T_ExtSolutionConfig;

typedef struct ExtProblemConfigType
{
	size_t VectorSize;
	float *h_a, *h_b, *h_c, *c_ref;
}T_ExtProblemConfig;

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
/* LDS	                                                                */
/************************************************************************/
typedef struct ExtLdsSolutionConfigTpye
{
}T_ExtLdsSolutionConfig;

typedef struct ExtLdsProblemConfigType
{
	size_t VectorSize;
	float *h_a, *h_b, *h_c, *c_ref;
}T_ExtLdsProblemConfig;

/************************************************************************/
/* GDS	                                                                */
/************************************************************************/
typedef struct ExtGdsSolutionConfigTpye
{
}T_ExtGdsSolutionConfig;

typedef struct ExtGdsProblemConfigType
{
	size_t VectorSize;
	float *h_a, *h_b, *h_c, *c_ref;
}T_ExtGdsProblemConfig;

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

/************************************************************************/
/* SOP	                                                                */
/************************************************************************/
typedef struct ExtSopSolutionConfigTpye
{
}T_ExtSopSolutionConfig;

typedef struct ExtSopProblemConfigType
{
	size_t VectorSize;
	float *h_a, *h_b, *h_c, *c_ref;
}T_ExtSopProblemConfig;

/************************************************************************/
/* Vector Add                                                           */
/************************************************************************/
typedef struct ExtVectAddSolutionConfigTpye
{
}T_ExtVectAddSolutionConfig;

typedef struct ExtVectAddProblemConfigType
{
	size_t VectorSize;
	float *h_a, *h_b, *h_c, *c_ref;
	std::list<T_KernelArgu> * KernelArgus;
}T_ExtVectAddProblemConfig;

/************************************************************************/
/* Reduction Add                                                           */
/************************************************************************/
typedef struct ExtReducAddSolutionConfigTpye
{
	int Methord;
	int Tile,TileGroup;
}T_ExtReducAddSolutionConfig;

typedef struct ExtReducAddProblemConfigType
{
	size_t ReducSize, VectorSize;
	size_t DataSize;
	float *h_a, *h_b, *h_c, *c_ref;
}T_ExtReducAddProblemConfig;

/************************************************************************/
/* Producer-Consumer                                                    */
/************************************************************************/
typedef struct ExtProducerConsumerSolutionConfigTpye
{
}T_ExtProducerConsumerSolutionConfig;

typedef struct ExtProducerConsumerProblemConfigType
{
	size_t VectorSize, DataSize;
	size_t SignalPerCU, SignalSize;
	float *h_a, *h_b, *h_c, *c_ref;
	float * h_sig;
}T_ExtProducerConsumerProblemConfig;
