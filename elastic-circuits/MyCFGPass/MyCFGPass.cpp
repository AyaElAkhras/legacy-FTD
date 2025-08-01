/**
 * Elastic Pass
 *
 * Forming a netlist of dataflow components out of the LLVM IR
 *
 */

#include <cassert>

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/RandomNumberGenerator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "ElasticPass/CircuitGenerator.h"
#include "ElasticPass/Utils.h"
#include "ElasticPass/Head.h"
#include "ElasticPass/Nodes.h"
#include "ElasticPass/Pragmas.h"
#include "OptimizeBitwidth/OptimizeBitwidth.h"


#include <sys/time.h>
#include <time.h>

static cl::opt<bool> opt_useLSQ("use-lsq", cl::desc("Emit LSQs where applicable"), cl::Hidden,
                                cl::init(true), cl::Optional);

static cl::opt<std::string> opt_cfgOutdir("cfg-outdir", cl::desc("Output directory of MyCFGPass"),
                                          cl::Hidden, cl::init("."), cl::Optional);

static cl::opt<bool> opt_buffers("simple-buffers", cl::desc("Naive buffer placement"), cl::Hidden,
                                cl::init(false), cl::Optional);

static cl::opt<std::string> opt_serialNumber("target", cl::desc("Targeted FPGA"), cl::Hidden,
                                cl::init("default"), cl::Optional);

struct timeval start, time_end;

std::string fname;
static int indx_fname = 0;

void set_clock() { gettimeofday(&start, NULL); }

double elapsed_time() {
    gettimeofday(&time_end, NULL);

    double elapsed = (time_end.tv_sec - start.tv_sec);
    elapsed += (double)(time_end.tv_usec - start.tv_usec) / 1000000.0;
    return elapsed;
}

using namespace llvm;

//class ControlDependencyGraph;

namespace {
class MyCFGPass : public llvm::FunctionPass {

public:
    static char ID;

    MyCFGPass() : llvm::FunctionPass(ID) {}

    virtual void getAnalysisUsage(AnalysisUsage& AU) const {
        // 			  AU.setPreservesCFG();
        AU.addRequired<OptimizeBitwidth>();
		OptimizeBitwidth::setEnabled(false);  // pass true to it to enable OB	

        AU.addRequired<MemElemInfoPass>();

		AU.addRequired<LoopInfoWrapperPass>();

    }

    void compileAndProduceDOTFile(Function& F , LoopInfo& LI) {
        fname = F.getName().str();

        // main is used for frequency extraction, we do not use its dataflow graph
        if (fname != "main") {

            bool done = false;

            auto M = F.getParent();

            pragmas(0, M->getModuleIdentifier());

            std::vector<BBNode*>* bbnode_dag = new std::vector<BBNode*>;
            std::vector<ENode*>* enode_dag   = new std::vector<ENode*>;
            OptimizeBitwidth* OB             = &getAnalysis<OptimizeBitwidth>();
            MemElemInfo& MEI                 = getAnalysis<MemElemInfoPass>().getInfo();

            //----- Internally constructs elastic circuit and adds elastic components -----//

            // Naively building circuit
            CircuitGenerator* circuitGen = new CircuitGenerator(enode_dag, bbnode_dag, OB, MEI);//, CDGP);//, CDG);



	    // calling functions implemented in `AddComp.cpp`
            circuitGen->buildDagEnodes(F);
            circuitGen->fixBuildDagEnodes();
            circuitGen->removeRedundantBeforeElastic(bbnode_dag, enode_dag);
            circuitGen->setGetelementPtrConsts(enode_dag);


#if 1
			circuitGen->buildPostDomTree(F);
			circuitGen->constructDomTree(F);

			circuitGen->Fix_LLVM_PhiPreds();

			//circuitGen->addSourceForConstants();   

	    	circuitGen->addStartC();


	        //printDotCFG(bbnode_dag, (opt_cfgOutdir + "/" + fname + "_bbgraph.dot").c_str());


	    	// Added a flag to choose a mode of operation for the CntrlOrder network for interfacing with the LSQ:
			/*
				- If false: Make the CntrlOrder network strictly follow the sequential control flow,
				Or - If True: Make the CntrlOrder network implement the FPGA'23 algorithm
			*/
			// depending on the experiment I want, I manually change the flag to true or false.
			bool is_smart_cntrlOrder_flag = true;

			bool lazy_fork_flag = false;

			circuitGen->printDebugDominatorTree();

			
			if(!is_smart_cntrlOrder_flag) {
				circuitGen->buildRedunCntrlNet();  
			}


			circuitGen->identifyBBsForwardControlDependency();

			circuitGen->FindLoopDetails(LI);


			// This function (implemented in Memory.cpp) takes care of all the memory stuff; mainly operates on the CntrlOrder network
			circuitGen->addMemoryInterfaces(opt_useLSQ, is_smart_cntrlOrder_flag, lazy_fork_flag);

			circuitGen->connectReturnToVoid_irredundant();

			// the following function is responsible for adding Merges for re-generation!
			circuitGen->checkLoops(F, LI, CircuitGenerator::data);
			circuitGen->checkLoops(F, LI, CircuitGenerator::constCntrl);

			if(is_smart_cntrlOrder_flag) {
				circuitGen->checkLoops(F, LI, CircuitGenerator::memDeps);
			}
			
			circuitGen->removeExtraPhisWrapper(CircuitGenerator::data);
			circuitGen->removeExtraPhisWrapper(CircuitGenerator::constCntrl);
			if(is_smart_cntrlOrder_flag) {
				circuitGen->removeExtraPhisWrapper(CircuitGenerator::memDeps);
			}
			
		
			circuitGen->Fix_my_PhiPreds(LI, CircuitGenerator::data);  // to fix my phi_
			circuitGen->Fix_my_PhiPreds(LI, CircuitGenerator::constCntrl);  // to fix my irredundant phi_c

			if(is_smart_cntrlOrder_flag) {
				circuitGen->Fix_my_PhiPreds(LI, CircuitGenerator::memDeps);
			}

			circuitGen->addSuppress_with_loops(CircuitGenerator::data); // connect producers and consumers of the datapath
			circuitGen->addSuppress_with_loops(CircuitGenerator::constCntrl);  // trigger constants from START through the data-less network

			if(is_smart_cntrlOrder_flag) {
				circuitGen->addSuppress_with_loops(CircuitGenerator::memDeps);
			}

			circuitGen->removeExtraBranchesWrapper(CircuitGenerator::data);
			circuitGen->removeExtraBranchesWrapper(CircuitGenerator::constCntrl);
			if(is_smart_cntrlOrder_flag) {
				circuitGen->removeExtraBranchesWrapper(CircuitGenerator::memDeps);
			}
			
			// Aya: TODO: Note that the following setMuxes_nonLoop() function crashes when a BB in the CFG is fed by three inputs

			// Aya: 16/06/2022: final version of setMuxes that converts only the Merges at the confluence points not at the loop headers into MUXes
				// this is why internally it does not need to operate on Phi_c because those are never inserted except at loop headers (for regeneration)
			circuitGen->setMuxes_nonLoop(); 

			// Aya: 13/09/2022: added the following function (implemented in Memory.cpp) to optimize the Bx components that are having no succs thus fed to a sink
	      	if(is_smart_cntrlOrder_flag)
	      		circuitGen->removeUselessBxs(lazy_fork_flag);

			// FOR THIS FLAG TO BE FALSE EFFECTIVELY, MAKE SURE 1) YOU ARE NOT RAISING THE TAG FLAG, 2) YOU HAVE ONLY 1 THREAD IN YOUR CODE
			bool loopMux_flag = false;
			if(loopMux_flag) {
				// 26/02/2023: The following functions is meant to modify the graphs to fit the new loop-related ideas
			// 1st remove all INITs with their constants
			// 2nd change the type of the MUX to map to the loop-mux in the backend
			// 3rd change the inputs order of the MUX is needed to comply with the convention that the 0th input should always come from outside
				circuitGen->convert_to_special_mux();
				// Forcing the insertion of a Synchronizer component that gets fed with all in0 of all LoopMUXes belonging to 1 loop
				circuitGen->synch_loopMux();
			}

			// the purpose of this function is to convert one of the Muxes of every loop into a CMerge and let its condition output serve as the select of all other MUxes of this loop
            std::string tag_info_path;
            //get_tag_info_file_path(tag_info_path); 
            bool loop_cmerge_flag = false;//is_tag_from_input(tag_info_path);
			bool ignore_outer_most_loop = true; 
			if(loop_cmerge_flag) {
				circuitGen->convert_loop_cmerge(tag_info_path, ignore_outer_most_loop);  // implemented in newLoops_management.cpp: needs to tweak the condition of picking the master Mux based on the example
																		// currenly applies the logic of finding the master Cmerge for every loop in the circuit 
			}

			// Added the following function to make sure that the inputs of the INIT are "false" always at in0 and "true" at in1 except one "false" in the end to take us back to the initial state
			bool fix_init_inputs_flag = true;
			     // Remember that the above flag MUST be TRUE if you are using the cleaned up INIT that does not have a leftover token in elastic_components.vhd
                    // This is the case for both the fast token delivery witb the cleaned up INIT or the multithreading tagging of FPGA'24
            if(fix_init_inputs_flag)
				circuitGen->fix_INIT_inputs();

			bool if_else_cmerge_flag = loop_cmerge_flag;
			if(if_else_cmerge_flag) {
				circuitGen->convert_if_else_cmerge(tag_info_path);
			}
			if(loop_cmerge_flag || if_else_cmerge_flag)
				circuitGen->insert_tagger_untagger_wrapper(tag_info_path); // implemented in AddTags.cpp: 


			// IMP Note: this function is also important to connect the predecessors of all branches
			circuitGen->Aya_addFork();
			
			// call the following function to label the clustered nodes as tagged
			if(loop_cmerge_flag || if_else_cmerge_flag) {
				circuitGen->tag_cluster_nodes(tag_info_path); // implemented in AddTags.cpp: 

                bool flag_1 = false;
                bool flag_2 = false;
                for(int uu = 0; uu < enode_dag->size(); uu++) {
                    if(enode_dag->at(uu)->is_if_else_cmerge)
                        flag_1 = true;
                    if(enode_dag->at(uu)->is_data_loop_cmerge) {
                        flag_2 = true;
                    }
                }
                //if(!(flag_1 && flag_2))
				    circuitGen->addMissingAlignerInputs(tag_info_path);  // this function (i) inserts the ALIGNER, (ii) identifies the DIRTY nodes, marks them and passes their clean inputs through the TAGGER/UNTAGGER
			}

			// added the following function to make sure to transfer any fork feeding the LSQ to a LazyFork
				// this is needed only if we use the naive interface with the LSQ through the ordered network of control merges because in case of the smart network, the type of the fork is defined from the moment we insert it inside the Memory.cpp
			if(!is_smart_cntrlOrder_flag && lazy_fork_flag) {
				circuitGen->convert_LSQ_fork_to_lazy();  // implemented in Memory.cpp
			}

			// added the following function that loops over Branch_c nodes that still do not have their succs connected and adds them..
				// This is because due to weird LLVM errors, I had to comment this part from Aya_addFork() function, so I'm tryin to do it here
			circuitGen->extraCheck_AddBranchSuccs();

			circuitGen->remove_SUPP_COND_Negation();

	      	circuitGen->addExitC();

	      	/* 
		      	It is IMP to note that to make the new ST and MC designs work, you need to do the following steps:
			      	1) Make the fix_mc_st_interfaces_flag true and compile
			      	2) Make the same flag true in write_components() in vhdl_writer.cpp and compile
			      	3) Make sure you give the "mc_store_op" name to the correct VHDL module in MemCont.vhd (and same for the MemCont module itself)
	      	*/
			bool fix_mc_st_interfaces_flag = false;
			bool fix_mc_st_interfaces_flag_yes_extra_ST_output = false;

			if(fix_mc_st_interfaces_flag) {
	      		circuitGen->fix_ST_MC_interfaces(fix_mc_st_interfaces_flag_yes_extra_ST_output);  // implemented in AddCtrl.cpp

	      		circuitGen->deliver_ACK_tokens_wrapper();  // this function adds the necessary steering components between any ST and END
	      						// In particular, it takes care of 1) compensating for a token if ST will not execute, 2) Discarding the extra ACKs if the ST is inside a loop
			}

			circuitGen->deleteLLVM_Br();
	      	
			if (opt_buffers)
				circuitGen->addBuffersSimple_OLD();

			bool tag_some_components_flag = false; 
			if(tag_some_components_flag){
				circuitGen->tag_some_components();
			}

       		if (OptimizeBitwidth::isEnabled()) {
				circuitGen->setSizes();   // called in Bitwidth.cpp
			}


	// this is important in filling the frequency field of BBs needed later in buffers
			circuitGen->setFreqs(F.getName());
						
	        aya_printDotDFG(enode_dag, bbnode_dag, opt_cfgOutdir + "/" + fname + "_graph.dot", opt_serialNumber, fix_mc_st_interfaces_flag, tag_info_path);
	        printDotCFG(bbnode_dag, (opt_cfgOutdir + "/" + fname + "_bbgraph.dot").c_str());
#endif

        }
    }

    bool runOnFunction(Function& F) override {
		LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
		this->compileAndProduceDOTFile(F, LI); 
    }

    void print(llvm::raw_ostream& OS, const Module* M) const override {}
};
} // namespace

char MyCFGPass::ID = 1;

static RegisterPass<MyCFGPass> Z("mycfgpass", "Creates new CFG pass",
                                 false, // modifies the CFG!
                                 false);

/* for clang pass registration
 */
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

static void registerClangPass(const PassManagerBuilder&, legacy::PassManagerBase& PM) {
    PM.add(new MyCFGPass());
}

static RegisterStandardPasses RegisterClangPass(PassManagerBuilder::EP_EarlyAsPossible,
                                                registerClangPass);

bool fileExists(const char* fileName) {
    FILE* file = NULL;
    if ((file = fopen(fileName, "r")) != NULL) {
        fclose(file);

        return true;

    } else {

        return false;
    }
}