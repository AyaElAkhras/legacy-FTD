#pragma once
#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/RandomNumberGenerator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"

//-------------------------------------------------------//

using namespace llvm;

enum node_type {

    Inst_     = 0,  // instruction node
    Argument_ = 2,  // argument node
    Phi_      = 3,  // original LLVM Phi
    Branch_   = 4,  // branch condition
    Buffera_  = 5,  // argument buffer
    Bufferi_  = 6,  // instruction buffer
    Phi_n     = 7,  // added phi node
    Fork_     = 8,  // eager fork node
    Branch_n  = 9,  // branch node
    Branch_c  = 10, // control branch node
    Phi_c     = 11, // control phi node
    LSQ_      = 12, // memory controller
    Cst_      = 13, // node for constant inputs
    Start_    = 14,
    End_      = 15,
    Fork_c    = 16,
    Fifoa_    = 17, // argument buffer
    Fifoi_    = 18, // instruction buffer
    Sink_   = 19,
    Source_ = 20,
    MC_     = 21, 
    dummy_ =22,

	Inj_n=23,   // AYA: 25/02/2022: the new component responsible for token regeneration

    Bx_Buffer_wrapper_ = 24,   // AYA: 14/09/2022: a new component to act as a wrapper for the Buffer that is used inside the Bx (sequentializer of the smart LSQ interface)
    Bx_Join_wrapper_ = 25,

    LazyFork_ = 26,  // Aya: data-less lazy fork used in the LSQs!!

    Loop_Phi_n = 27,  // 24/02/2023: Aya added this new type that should be used to represent any MUX at the loop header so that we do not insert an INIT!
    Loop_Phi_c = 28,

    TMFO = 29,  // needed in the control path in the transformation of REGEN_SUPP to SUPP_REGEN 

    LoopMux_Synch = 30,

    Tagger = 31,  // AYA: 17/09/2023
    Un_Tagger = 32, // AYA: 18/09/2023

    ROB = 33,   // AYA: 07/08/2023

    Free_Tags_Fifo = 34,

    Aligner_Branch = 35,
    Aligner_Mux = 36,

    // AYA: 08/01/2025: Supporting new components produced by the Lean theorem prover
    Split = 37,
    Concat = 38,
    // Aya: 11/1/2025: WIll most likely not need the two addition *_tag types because the bitwidth will be referenced from the Lean framework
    Split_tag = 39,
    Concat_tag = 40,
};

class BBNode; 

class ENode {

public:
    // AYA: 26/12/2023: added the following fields that should take a value for ENodes of type loop_cmerge or if_else_cmerge!!
    ENode* tagger = nullptr;
    ENode* un_tagger = nullptr;
    int tagging_count = 0;
    // AYA: 26/12/2023:
    int tagger_id = -1; // this field is relevant only for ENodes of type aligner_mux and aligner_branch and un_tagger

    bool is_tagged = false;

    bool is_st_ack_to_end = false; // true only for branches and phis that are added between a ST and END in delivering the ST acks

	// AYA: 23/02/2022:
	// I only set this input for a subset of branches and Muxes that are added inside newSetMuxes
		// and for those special nodes, I check this is_negated in printDot
	std::vector<bool>* is_negated_input; 	
	bool is_advanced_component;  // initialized to false and set to true for a branch or mux inserted inside newSetMuxes

	// AYA: 15/03/2022: the following field is set to true for enodes of type Const_ that are added to mimic the intialization token that is needed to break deadlocks in loops
	bool is_init_token_const; 

	// AYA: 19/03/2022: the following field is set to true for enodes of type Phi_n that are added in the process of mimicing the intialization token that is needed to break deadlocks in loops
	bool is_init_token_merge;

    int capacity;
    char* compType               = nullptr;
    ENode* branchFalseOutputSucc = nullptr;
    ENode* branchTrueOutputSucc  = nullptr;
    std::vector<ENode*>* parameterNodesReferences[3];
    int cstValue   = 0;       // Leonardo, inserted line, used only with Cst_ type of nodes
    BBNode* bbNode = nullptr; // Leonardo, inserted line // Aya: 23/05/2022: most likely the field is not written anywhere!
    node_type type;
    std::string Name;
    Instruction* Instr = nullptr;
    ENode* Mem         = nullptr; // For memory instr, store relevant LSQ/MC enode
    BasicBlock* BB     = nullptr;
    Argument* A        = nullptr;
    ConstantInt* CI    = nullptr;
    ConstantFP* CF     = nullptr;
    std::vector<ENode*>* CntrlPreds;     // naming isnt very good, this is a normal predecessor that
                                         // carries both data and control signals
    std::vector<ENode*>* CntrlSuccs;     // naming isnt very good, this is a normal successor that
                                         // carries both data and control signals
    std::vector<ENode*>* JustCntrlPreds; // predecessors that only carry control signals
    std::vector<ENode*>* JustCntrlSuccs; // successors that only carry control signals

	// Aya: Introducing a new network to assure correct order!!
	std::vector<ENode*>* CntrlOrderPreds; // predecessors that only carry control signals specifically to assure order!!
    std::vector<ENode*>* CntrlOrderSuccs; // successors that only carry control signals specifically to assure order!!

	////// Aya:to keep track of backward edges
	std::vector<bool>* isBackwardPreds_Cntrl;
	std::vector<bool>* isBackwardPreds;

	/////// Aya: the following variable identifies nodes that are part of the new control path
	bool is_redunCntrlNet = false;

	////////////////// AYA: 16/11/2021: ADDED A FIELD TO DISTINGUISH THE ENODES OF TYPE CONSTANT THAT ARE FEEDING THE CONDITION OF A BRANCH!! AND ANOTHER ONE TO DENOTE THAT THE CONSTANT IS FEEDING A MC OR LSQ
	bool is_const_br_condition = false;
	bool is_const_feed_memory = false;
	

	// Aya: added the following arrays to keep track of a branch's potentially many true and/or false sucessors
	// for the data path 
	std::vector<ENode*>* true_branch_succs;
	std::vector<ENode*>* false_branch_succs;
	// for the control path responsible for constant triggering 
	std::vector<ENode*>* true_branch_succs_Ctrl;
	std::vector<ENode*>* false_branch_succs_Ctrl;
    
    //Aya: 19/08/2022: for the control path responsible for interfacing with the LSQ
    std::vector<ENode*>* true_branch_succs_Ctrl_lsq;
    std::vector<ENode*>* false_branch_succs_Ctrl_lsq;

    // AYA: 17/09/2023: added the following field for the CMerges that I add in loop headers and will carry data
    std::vector<ENode*>* cmerge_data_succs;
    std::vector<ENode*>* cmerge_control_succs;
    bool is_data_loop_cmerge = false;

	ENode* producer_to_branch = nullptr;  // branch in a bridge BB carries data from which producer
  
    std::vector<unsigned> sizes_preds; // sizes of the signals to predecessors
    std::vector<unsigned> sizes_succs; // sizes of the signals to successors

    bool is_live_in  = false;
    bool is_live_out = false;
    bool visited     = false;
    bool inthelist   = false;
    int id           = -1;
    int counter      = 0;
    bool controlNode; // true if this ENode is in the Cntrl_nodes list of a BBNode

    ENode(node_type nd, const char* name, Instruction* inst, BasicBlock* bb);
    ENode(node_type nd, BasicBlock* bb);
    ENode(node_type nd, const char* name, Argument* a, BasicBlock* bb);
    ENode(node_type nd, const char* name, BasicBlock* bb);
    ENode(node_type nd, const char* name);

    bool isLoadOrStore() const;

    int bbId;
    int memPortId;
    bool memPort = false; // LD or ST port connected to MC or LSQ
    int bbOffset;
    bool lsqToMC = false;

    bool isMux = false;
    bool isCntrlMg = false;
    std::string argName;
    int lsqMCLoadId = 0; 
    int lsqMCStoreId = 0; 

    // Aya: 15/06/2022: added the following field that is true only for enodes of type Phi_n that are added in Shannon's expansion to generate a cond out of a boolean expression 
    bool is_mux_for_cond = false;
    bool is_const_for_cond = false; // true for constants that are added as inputs to Muxes or to Merges used to implement INIT 
    bool is_merge_init = false;   // true for Merges used to mimic the INIT

    // the following field is relevant ONLY for enodes of type LD or ST that need to be connected to a LSQ!!
    // AYA: 12/09/2022: added a field to hold the index 
    int group_idx = -1;

    // AYA: 13/09/2022: added the following field to identify the fork, buffer or Phi_c enodes added in the smart_LSQ_interface
    bool is_mem_dep_component = false;

    bool is_non_loop_gsa_mux = false;

    bool is_store_to_end_gsa = false; //AYA: 21/07/2023: added the following flag to mark the phic that will be added to compensate for conditionally executing stores

    // AYA: 17/09/2022: the following field is applicable omly if the ENODE is of type Bx_Buffer_wrapper_ because it stores the first_bb_idx of each group that is a producer to my Bx_Buffer_wrapper_
    std::vector<int> producer_group_indices;

    // AYA: 17/10/2022: I no longer need this field because I decided to not care for area-optimizing by checking if a Bx is already present in a specific BB by a previous LSQ object analysis
    // std::vector<bool> handled_loop_gsa_with_producer_group;  // flag to indicate whether you already applied GSA between this Bx and the group corresponding to the index or not.. This is useful when one Bx is used by more than 1 LSQ just because the LSQ has the same Bx with the same producers!

    // relevant only for branches and phis added during applying gsa on memory deps!
    ENode* orig_bx_buff_producer = nullptr;
    ENode* orig_bx_fork_consumer = nullptr;

    // 19/09/2022
    ENode* my_lsq_enode = nullptr;  // this field is relevant ONLY for buffer_bx components to store the LSQ enode that it feeds!!!


    // AYA: 24/02/2023: the following are new fields I added towards the new methodology for handling loops
    bool is_shannons_mux = false;  // used inside the newLoops_management to identify the MUXes that have INIT as their SEL vs. MUXes inserted by Shannon's because the latter could exist at the loop header as well making it not enough to check the BB

    // AYA: 24/03/2023: this flag should be "true" only for phis (will become loopmux later) that are added for regeneration when the consumer is inside a loop that the producer is outside
    bool is_regen_mux = false; 

    //Aya: 27/03/2023: arrays for maintaining all of the suucs and regens fed by 1 tmfo.. and just like branches, addFork, puts them in CntrlSuccs
    std::vector<ENode*>* tmfo_supp_succs;
    std::vector<ENode*>* tmfo_regen_succs; 

    // Aya: 29/04/2023: added to mark LoopMUXes that were already fed with a Synchronizer
    bool is_loopMux_synchronized = false;

    // AYA: 27/09/2023: this field marks constants that happen to be triggered by SOurce
    bool is_const_source_triggered = false;

    // AYA: 30/09/2023: this field should be set to true for every node fed from an OOO super node
    bool is_dirty_node = false;

     // AYA: 27/09/2023
    bool is_if_else_cmerge = false;
    ENode* old_cond_of_if_else_merge = nullptr;

    // AYA: 30/09/2023: added this to keep the store condition of the Branch inside the Branch enode
        // because we lose it as soon as we pass it to the Bracnh through the TAGGER
    ENode* old_cond_of_if_else_branch = nullptr;

    // AYA: 30/09/2023: this flag should be true for any component constituiting an ALIGNER including the Branch and the Mux that manage every ALIGNER input (except the reference input)!!
    bool is_aligner_part = false;

    bool isBooleanOper = false;

private:
    void commonInit(const node_type nd, const char* name, Instruction* inst, Argument* a,
                    BasicBlock* bb);
};

class BBNode {

public:
    const char* name; 
    BasicBlock* BB;
    int Idx;
    std::vector<BBNode*>* CntrlPreds;
    std::vector<BBNode*>* CntrlSuccs;
    std::vector<ENode*>* Live_in;
    std::vector<ENode*>* Live_out;
    std::vector<ENode*>* Cntrl_nodes;

    // 23/06/2022: added the following flag to distinguish a virtualBB that is added temporarily for the analysis of a Phi consumer 
    bool is_virtBB = false;

    /*
    * Aya: 23/05/2022: the following vector holds the indices of BBs that the BBNode object is 
    * control dependent on          
    */
    std::vector<int>* BB_deps; 

	/////////////// New added fields for keeping loop information
	bool is_loop_latch, is_loop_exiting, is_loop_header;
	BasicBlock* loop_header_bb;
	Loop* loop;

    bool is_inner_most_loop = false;

	/////////////////////////////////////

    std::map<std::string, double> succ_freqs;
    int counter;

    BBNode(BasicBlock* bb, int idx,
           const char* name = NULL); 

    bool isImportedFromDotFile();
    void set_succ_freq(std::string succ_name, double freq);
    double get_succ_freq(std::string succ_name);
    bool valid_successor(std::string succ_name);

    // Aya: 12/09/2022: added the following field to mark the BBnodes that are either themselves the entire group or are the earliest BBnode in 1 group!
    bool is_group_head = false;

};
