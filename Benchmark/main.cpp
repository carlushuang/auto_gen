#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "RuntimeControl.h"
#include "ProblemControl.h"


#include "IsaFlat.h"
#include "IsaDs.h"
#include "IsaMubuf.h"
#include "IsaSmem.h"
#include "IsaSop.h"

int main(int argc, char *argv[])
{
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
	//ProblemCtrlBase *ds = new DsProblem("ds instruction demo");
	//ds->RunProblem();
	//ProblemCtrlBase *smem = new SmemProblem("smem instruction demo");
	//smem->RunProblem();
	//ProblemCtrlBase *sop = new SopProblem("sop instruction demo");
	//sop->RunProblem();
	ProblemCtrlBase *flat = new FlatProblem("flat instruction demo");
	flat->RunProblem();
	//ProblemCtrlBase *mubuf = new MubufProblem("mubuf instruction demo");
	//mubuf->RunProblem();

	// ----------------------------------------------------------------------
	// 示例
	// ----------------------------------------------------------------------
	//ProblemCtrlBase *vAdd = new VectorAddProblem("VectorAdd");
	//vAdd->RunProblem();
	//ProblemCtrlBase *pc = new ProducerConsumerProblem();
	//pc->RunProblem();
		
	// ======================================================================
	// ======================================================================

	RuntimeCtrl::CleanRuntime();

	//getchar();

	return 0;
}
