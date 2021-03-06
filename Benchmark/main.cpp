﻿#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "RuntimeControl.h"
#include "ProblemControl.h"


//#include "IsaFlat.h"
//#include "IsaGds.h"
//#include "IsaLds.h"
//#include "IsaMubuf.h"
//#include "IsaSmem.h"
//#include "IsaSop.h"
//
#include "VectorAdd.h"
//#include "ReducAdd.h"
//#include "ProducerConsumer.h"

 
int main(int argc, char *argv[])
{
	//CmdArgs * ca = new CmdArgs(argc, argv);

	RuntimeCtrl::InitRuntime(argc, argv);
	 
	// ======================================================================
	// ======================================================================
	// ----------------------------------------------------------------------
	// 测试用例
	// ----------------------------------------------------------------------
	//ProblemCtrlBase *test = new TestProblem();
	//test->RunProblem();

	// ----------------------------------------------------------------------
	// ISA 示例
	// ----------------------------------------------------------------------
	//ProblemCtrlBase * lds = new LdsProblem("lds instruction demo");
	//lds->RunProblem();
	//ProblemCtrlBase *gds = new GdsProblem("gds instruction demo");
	//gds->RunProblem();
	//ProblemCtrlBase *flat = new FlatProblem("flat instruction demo");
	//flat->RunProblem();
	//ProblemCtrlBase *mubuf = new MubufProblem("mubuf instruction demo");
	//mubuf->RunProblem();
	//ProblemCtrlBase *smem = new SmemProblem("smem instruction demo");
	//smem->RunProblem();
	//ProblemCtrlBase *sop = new SopProblem("sop instruction demo");
	//sop->RunProblem();

	// ----------------------------------------------------------------------
	// 示例
	// ----------------------------------------------------------------------
	ProblemCtrlBase *vAdd = new VectAddProblem("VectorAdd");
	vAdd->RunProblem();
	//ProblemCtrlBase *rdcAdd = new ReducAddProblem("ReducAdd");
	//rdcAdd->RunProblem();
	//ProblemCtrlBase *pc = new ProducerConsumerProblem("Producer-Consumer");
	//pc->RunProblem();
		
	// ======================================================================
	// ======================================================================

	RuntimeCtrl::CleanRuntime();

	//getchar();

	return 0;
}
