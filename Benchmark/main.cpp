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
	// Sample
	// ----------------------------------------------------------------------
	//ProblemCtrlBase *vAdd = new VectorAddProblem("VectorAdd");
	//vAdd->RunProblem();
	//ProblemCtrlBase *pc = new ProducerConsumerProblem();
	//pc->RunProblem();

	// ----------------------------------------------------------------------
	// Instrucion
	// ----------------------------------------------------------------------
	//ProblemCtrlBase *ds = new DsProblem("ds instruction demo");
	//ds->RunProblem();
	ProblemCtrlBase *smem = new SmemProblem("smem instruction demo");
	smem->RunProblem();
	//ProblemCtrlBase *flat = new FlatProblem("flat instruction demo");
	//flat->RunProblem();
	//ProblemCtrlBase *mubuf = new MubufProblem("mubuf instruction demo");
	//mubuf->RunProblem();
	//ProblemCtrlBase *mubuf = new MubufProblem("mubuf instruction demo");
	//mubuf->RunProblem();
	//ProblemCtrlBase *global = new GlobalProblem();
	//global->RunProblem();
	//ProblemCtrlBase *lds = new DsProblem();
	//lds->RunProblem();
	//ProblemCtrlBase *slp = new SleepProblem();
	//slp->RunProblem();

	// ----------------------------------------------------------------------
	// TensileConv
	// ----------------------------------------------------------------------
	//ProblemCtrlBase *conv1x1 = new ConvFwd1x1Problem("DirConv1x1Fwd");
	//conv1x1->RunProblem();
	
	// ======================================================================
	// ======================================================================

	RuntimeCtrl::CleanRuntime();

	//getchar();

	return 0;
}
