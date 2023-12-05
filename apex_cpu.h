/*
 * apex_cpu.h
 * Contains APEX cpu pipeline declarations
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#ifndef _APEX_CPU_H_
#define _APEX_CPU_H_

#include "apex_macros.h"

/* Format of an APEX instruction  */
typedef struct APEX_Instruction
{
    char opcode_str[128];
    int opcode;
    int rd;
    int rs1;
    int rs2;
    int imm;
    int is_lit;    //flag to identify if the operand is literal or not
    int is_store;  // flag to show store command
    int stall;
} APEX_Instruction;

/* Model of CPU stage latch */
typedef struct CPU_Stage
{
    int pc;
    char opcode_str[128];
    int opcode;
    int rs1;
    int rs2;
    int rd;
    int imm;
    int is_lit;
    int rs1_value;
    int rs2_value;
    int rd_value;
    int result_buffer;
    int memory_address;
    int has_insn;
    int data_of_store;
    int stall;
} CPU_Stage;

typedef struct OpQueueEntry
{
    int program_counter;
    int is_valid;             // Indicates if the entry is valid
    int functional_unit_type; // Type of functional unit needed
    int immediate_value;      // Immediate value for the instruction
    int source1_ready;        // Flag indicating if source1 is ready
    int source1_register;     // Source1 register ID
    int source1_value;        // Source1 value
    int source2_ready;        // Flag indicating if source2 is ready
    int source2_register;     // Source2 register ID
    int source2_value;        // Source2 value
    int destination_register; // Destination register ID
    int load_store_queue_index; // Index in the Load Store Queue (if applicable)
    // Additional fields as needed
} OpQueueEntry;

typedef struct OpQueue
{
    OpQueueEntry entries[Op_QUEUE_SIZE];
    int next_free_index; // Index of the next free entry in the queue
} OpQueue;

typedef struct APEX_Reg_Status 
{
    int value;
    int status; // 0 means available, 1 means being used
    int valid;
} APEX_Reg_Status;

typedef struct {
    int value;         // Value stored in the physical register
    int valid;         // Flag to indicate if the register is valid
    int status;      // Associated architectural register
} PhysicalRegister;

/* Model of APEX CPU */
typedef struct APEX_CPU
{
    int pc;                        /* Current program counter */
    int clock;                     /* Clock cycles elapsed */
    int insn_completed;            /* Instructions retired */
    int regs[REG_FILE_SIZE];       /* Integer register file */
    int code_memory_size;          /* Number of instruction in the input file */
    APEX_Reg_Status register_status[REG_FILE_SIZE]; // Status of registers
    APEX_Instruction *code_memory; /* Code Memory */
    int data_memory[DATA_MEMORY_SIZE]; /* Data Memory */
    int single_step;               /* Wait for user input after every cycle */
    int zero_flag;                 /* {TRUE, FALSE} Used by BZ and BNZ to branch */
    int pos_flag;                  
    int neg_flag;
    int cc;                        
    int fetch_from_next_cycle;
    int stall;
    OpQueue op_queue;

    /* Pipeline stages */
    CPU_Stage fetch;
    CPU_Stage decode;
    CPU_Stage execute;
    CPU_Stage memory;
    CPU_Stage writeback;

    OpQueue opq;

    PhysicalRegister phys_reg[NUM_PHYSICAL_REGS];
} APEX_CPU;

APEX_Instruction *create_code_memory(const char *filename, int *size);
APEX_CPU *APEX_cpu_init(const char *filename);
void APEX_cpu_run(APEX_CPU *cpu);
void APEX_cpu_stop(APEX_CPU *cpu);
void stall_handling(APEX_CPU *cpu);
#endif
