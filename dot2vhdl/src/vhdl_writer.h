/*
*  C++ Implementation: dot2Vhdl
*
* Description:
*
*
* Author: Andrea Guerrieri <andrea.guerrieri@epfl.ch (C) 2019
*
* Copyright: See COPYING file that comes with this distribution
*
*/

#ifndef _VHDL_WRITER_
#define _VHDL_WRITER_

#define COMPONENT_GENERIC   0
#define COMPONENT_CONSTANT  1

#define CLK "clk"
#define RST "rst"
#define DATAIN_ARRAY "dataInArray"
#define DATAOUT_ARRAY "dataOutArray"
#define PVALID_ARRAY "pValidArray"
#define NREADY_ARRAY "nReadyArray"
#define READY_ARRAY "readyArray"
#define VALID_ARRAY "validArray"



#define MAX_COMPONENTS  6




#define ENTITY_MERGE            "merge"
#define ENTITY_READ_MEMORY      "read_memory_single"
#define ENTITY_WRITE_MEMORY     "write_memory_single"
#define ENTITY_SINGLE_OP        "SingleOp"
#define ENTITY_GET_PTR          "getptr"
#define ENTITY_INT_MUL          "intMul"
#define ENTITY_INT_ADD          "intAdd"
#define ENTITY_INT_SUB          "intSub"
#define ENTITY_BUF              "elasticBuffer"
#define ENTITY_TEHB             "TEHB"
#define ENTITY_OEHB             "OEHB"
#define ENTITY_FIFO             "elasticFifo"
#define ENTITY_NFIFO            "nontranspFifo"
#define ENTITY_TFIFO            "transpFifo"
#define ENTITY_FORK             "fork"
#define ENTITY_ICMP             "intCmp"
#define ENTITY_CONSTANT         "Const"
#define ENTITY_BRANCH           "Branch"
#define ENTITY_END              "end_node"
#define ENTITY_START            "start_node"
#define ENTITY_SOURCE           "source"
#define ENTITY_SINK             "sink"
#define ENTITY_MUX              "Mux"
#define ENTITY_LOOPMUX          "LoopMux" // "LoopMux"  // Aya: 28/02/2023  // 04/04/2023: changed it gian_mux to print the name of Gianluca's MUX
#define ENTITY_CTRLMERGE        "CntrlMerge"
#define ENTITY_LSQ              "LSQ"
#define ENTITY_MC               "MemCont"
#define ENTITY_SEL				"SEL"
#define ENTITY_DISTRIBUTOR      "Distributor"
#define ENTITY_SELECTOR			"Selector"
#define ENTITY_INJECTOR			"Inj"
#define ENTITY_TMFO          "TMFO"  // Aya: 27/03/2023
#define ENTITY_SYNCH "Synch"  // Aya: 29/04/2023
#define ENTITY_TAGGER "Tagger"  // AYA: 17/09/2023
#define ENTITY_UNTAGGER "Un_Tagger"  // AYA: 17/09/2023
#define ENTITY_FREE_TAGS_FIFO "Free_Tags_Fifo"

#define ENTITY_ALIGNER_BRANCH "Aligner_Branch"   // AYA: 01/10/2023
#define ENTITY_ALIGNER_MUX "Aligner_Mux"

#define ENTITY_SPLIT "Split"
#define ENTITY_CONCAT "Concat"

#define COMPONENT_MERGE         "Merge"
#define COMPONENT_READ_MEMORY   "load"
#define COMPONENT_WRITE_MEMORY  "store"
#define COMPONENT_SINGLE_OP     "Merge"
#define COMPONENT_GET_PTR       "Merge"
#define COMPONENT_INT_MUL       "mul"
#define COMPONENT_INT_ADD       "add"
#define COMPONENT_INT_SUB       "sub"
#define COMPONENT_BUF           "Buffer"
#define COMPONENT_TEHB          "TEHB"
#define COMPONENT_OEHB          "OEHB"
#define COMPONENT_FIFO          "Fifo"
#define COMPONENT_NFIFO         "nFifo"
#define COMPONENT_TFIFO         "tFifo"
#define COMPONENT_FORK          "Fork"
#define COMPONENT_ICMP          "icmp"
#define COMPONENT_CONSTANT_     "Constant"
#define COMPONENT_BRANCH        "Branch"
#define COMPONENT_END           "Exit"
#define COMPONENT_START         "Entry"
#define COMPONENT_SOURCE        "Source"
#define COMPONENT_SINK          "Sink"
#define COMPONENT_MUX           "Mux"
#define COMPONENT_LOOPMUX        "LoopMux"   // Aya: 28/02/2023
#define COMPONENT_CTRLMERGE     "CntrlMerge"
#define COMPONENT_LSQ           "LSQ"
#define COMPONENT_MC            "MC"
#define COMPONENT_SEL			"SEL"
#define COMPONENT_DISTRIBUTOR   "Distributor"
#define COMPONENT_SELECTOR		"Selector"
#define COMPONENT_INJECTOR		"Inj"
#define COMPONENT_TMFO      "TMFO" // Aya: 27/03/2023
#define COMPONENT_SYNCH "Synch"  // Aya: 29/04/2023
#define COMPONENT_TAGGER "Tagger"  // AYA: 17/09/2023
#define COMPONENT_UNTAGGER "Un_Tagger"  // AYA: 17/09/2023
#define COMPONENT_FREE_TAGS_FIFO "Free_Tags_Fifo"

#define COMPONENT_ALIGNER_BRANCH "Aligner_Branch"   // AYA: 01/10/2023
#define COMPONENT_ALIGNER_MUX "Aligner_Mux"


#define COMPONENT_SPLIT "Split"   // AYA: 01/10/2023
#define COMPONENT_CONCAT "Concat"


#define UNDERSCORE  "_"
#define COLOUMN " : "
#define SEMICOLOUMN ";"
#define COMMA   ","

#define SIGNAL_STRING  "signal "
#define STD_LOGIC      "std_logic;"

#define DEFAULT_BITWIDTH    32

class vhdl_writer {
    
public:
    void write_vhdl ( string filename, int indx );
    void write_tb_wrapper ( string filename  );
    
private:



};


enum
{
    ENTITY_MERGE_INDX,            
    ENTITY_READ_MEMORY_INDX,      
    ENTITY_SINGLE_OP_INDX,        
    ENTITY_GET_PTR_INDX,          
    ENTITY_INT_MUL_INDX,          
    ENTITY_INT_ADD_INDX, 
    ENTITY_INT_SUB_INDX, 
    ENTITY_BUF_INDX,
    ENTITY_TEHB_INDX,
    ENTITY_OEHB_INDX,
    ENTITY_FIFO_INDX,
    ENTITY_NFIFO_INDX,
    ENTITY_TFIFO_INDX,
    ENTITY_FORK_INDX,
    ENTITY_ICMP_INDX,
    ENTITY_CONSTANT_INDX,
    ENTITY_BRANCH_INDX,
    ENTITY_END_INDX,
    ENTITY_START_INDX,   
    ENTITY_WRITE_MEMORY_INDX,
    ENTITY_SOURCE_INDX,
    ENTITY_SINK_INDX,
    ENTITY_MUX_INDX,
    ENTITY_LOOPMUX_INDX,
    ENTITY_CTRLMERGE_INDX,
    ENTITY_LSQ_INDX,
    ENTITY_MC_INDX,
    ENTITY_SEL_INDX,
    ENTITY_DISTRIBUTOR_INDX,
    ENTITY_SELECTOR_INDX,
    ENTITY_MAX
};



#endif
