/*
 * apex_cpu.c
 * Contains APEX cpu pipeline implementation
 *
 * Author:
 * Copyright (c) 2020, Gaurav Kothari (gkothar1@binghamton.edu)
 * State University of New York at Binghamton
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apex_cpu.h"
#include "apex_macros.h"

int oq_ind,entryIndex;
/* Converts the PC(4000 series) into array index for code memory
 *
 * Note: You are not supposed to edit this function
 */
static int
get_code_memory_index_from_pc(const int pc)
{
    return (pc - 4000) / 4;
}

static void
print_instruction(const CPU_Stage *stage)
{
    switch (stage->opcode)
    {
        case OPCODE_ADD:
        case OPCODE_SUB:
        case OPCODE_MUL:
        case OPCODE_DIV:
        case OPCODE_AND:
        case OPCODE_OR:
        case OPCODE_XOR:
        {
            printf("%s,R%d,R%d,R%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->rs2);
            break;
        }

        case OPCODE_MOVC:
        {
            printf("%s,R%d,#%d ", stage->opcode_str, stage->rd, stage->imm);
            break;
        }

        case OPCODE_LOAD:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->imm);
            break;
        }

        case OPCODE_LOADP:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                stage->imm);
            break;
        }


        case OPCODE_STORE:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rs1, stage->rs2,
                   stage->imm);
            break;
        }

        case OPCODE_STOREP:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rs1, stage->rs2,
                   stage->imm);
            break;
        }


        case OPCODE_BZ:
        case OPCODE_BNZ:
        case OPCODE_BP:
        case OPCODE_BNP:
        case OPCODE_BN:
        case OPCODE_BNN:
        {
            printf("%s,#%d ", stage->opcode_str, stage->imm);
            break;
        }

        case OPCODE_HALT:
        {
            printf("%s", stage->opcode_str);
            break;
        }

        case OPCODE_NOP:
        {
            printf("%s", stage->opcode_str);
            break;
        }

        case OPCODE_ADDL:
        case OPCODE_SUBL:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->imm);
            break;
        }

        case OPCODE_CMP:
        {
            printf("%s,R%d,R%d ", stage->opcode_str, stage->rs1, stage->rs2);
            break;
        }

        case OPCODE_CML:
        {
            printf("%s,R%d,#%d ", stage->opcode_str, stage->rs1, stage->imm);
            break;
        }

        case OPCODE_JUMP:
        {
            printf("%s,R%d,#%d ", stage->opcode_str, stage->rs1, stage->imm);
            break;
        }

        case OPCODE_JALR:
        {
            printf("%s,R%d,R%d,#%d ", stage->opcode_str, stage->rd, stage->rs1,
                   stage->imm);
            break;
        }

    }
}

/* Debug function which prints the CPU stage content
 *
 * Note: You can edit this function to print in more detail
 */
static void
print_stage_content(const char *name, const CPU_Stage *stage)
{
    printf("%-15s: pc(%d) ", name, stage->pc);
    print_instruction(stage);
    printf("\n");
}

/* Debug function which prints the register file
 *
 * Note: You are not supposed to edit this function
 */
static void
print_reg_file(const APEX_CPU *cpu)
{
    int i;

    printf("----------\n%s\n----------\n", "Registers:");

    for (int i = 0; i < REG_FILE_SIZE / 2; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }

    printf("\n");

    for (i = (REG_FILE_SIZE / 2); i < REG_FILE_SIZE; ++i)
    {
        printf("R%-3d[%-3d] ", i, cpu->regs[i]);
    }

    printf("\n");
}

/* Definition of check_oq_entry function */
int check_op_queue_entry(APEX_CPU* cpu) {
    for (int oq_ind = 0; index < Op_QUEUE_SIZE; oq_ind++) {
        if (!cpu->op_queue.entries[oq_ind].is_valid) {
            cpu->op_queue.next_free_index = index;
            return 1;
        }
    }
    return 0;
}

int add_op_queue_entry(APEX_CPU* cpu, OpQueueEntry* newOpEntry) {
    // Check if there is a free entry in the OpQueue
    if (cpu->op_queue.next_free_index < Op_QUEUE_SIZE) {
        int entryIndex = cpu->op_queue.next_free_index;

        cpu->op_queue.entries[entryIndex].program_counter = newOpEntry->program_counter;
        cpu->op_queue.entries[entryIndex].is_valid = newOpEntry->is_valid;
        cpu->op_queue.entries[entryIndex].functional_unit_type = newOpEntry->functional_unit_type;
        cpu->op_queue.entries[entryIndex].immediate_value = newOpEntry->immediate_value;
        cpu->op_queue.entries[entryIndex].source1_ready = newOpEntry->source1_ready;
        cpu->op_queue.entries[entryIndex].source1_register = newOpEntry->source1_register;
        cpu->op_queue.entries[entryIndex].source1_value = newOpEntry->source1_value;
        cpu->op_queue.entries[entryIndex].source2_ready = newOpEntry->source2_ready;
        cpu->op_queue.entries[entryIndex].source2_register = newOpEntry->source2_register;
        cpu->op_queue.entries[entryIndex].source2_value = newOpEntry->source2_value;
        cpu->op_queue.entries[entryIndex].destination_register = newOpEntry->destination_register;
        cpu->op_queue.entries[entryIndex].load_store_queue_index = newOpEntry->load_store_queue_index;

        // Update the index for the next free entry in the OpQueue
        cpu->op_queue.next_free_index++;
        return 0;
    }
    return -1; // OpQueue is full
}

int is_op_queue_entry_valid(APEX_CPU* cpu, int numEntries)
{
    for (int i = 0; i < numEntries; i++)
    {
        if (cpu->op_queue.entries[i].is_valid && 
            cpu->op_queue.entries[i].source1_ready && 
            cpu->op_queue.entries[i].source2_ready)
        {
            update_instruction(cpu, i);
            return 1;
        }
    }
    return 0;
}

// void update_instruction(APEX_CPU* cpu, int index)
// {
//     // Assuming 'execute' is the next stage after issue/operation queue
//     cpu->execute.rs1_value = cpu->op_queue.entries[index].source1_value;
//     cpu->execute.rs2_value = cpu->op_queue.entries[index].source2_value;

//     // Assuming destination register and functional unit type are also needed
//     cpu->execute.rd = cpu->op_queue.entries[index].destination_register;

//     // You can also copy other necessary fields from the OpQueue entry to the execute stage
// }
int update_instruction(APEX_CPU* cpu, int index) {
    // Check if the index is within the bounds of the issue queue
    if (index < 0 || index >= Op_QUEUE_SIZE) {
        // Handle the error case, such as logging an error or returning an error code
        return -1;
    }

    // Access the issue queue entry
    OpQueueEntry* iq_entry = &cpu->op_queue.entries[index];

    // Check if the issue queue entry is valid
    if (!iq_entry->is_valid) {
        // The entry is not valid; handle this case appropriately
        return -1;
    }

    // Update the execute stage with the source registers and their values from the issue queue entry
    cpu->execute.rs1 = iq_entry->source1_register;
    cpu->execute.rs2 = iq_entry->source2_register;
    cpu->execute.rs1_value = iq_entry->source1_value;
    cpu->execute.rs2_value = iq_entry->source2_value;

    // Assuming destination register and functional unit type are also needed
    cpu->execute.rd = cpu->op_queue.entries[index].destination_register;


    return 0;
}


/*
 * Fetch Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_fetch(APEX_CPU *cpu)
{
    APEX_Instruction *current_ins;

    if (cpu->fetch.has_insn)
    {
        /* This fetches new branch target instruction from next cycle */
        if (cpu->fetch_from_next_cycle == TRUE)
        {
            cpu->fetch_from_next_cycle = FALSE;

            /* Skip this cycle*/
            return;
        }


        /* Store current PC in fetch latch */
        cpu->fetch.pc = cpu->pc;

        /* Index into code memory using this pc and copy all instruction fields
         * into fetch latch  */
        current_ins = &cpu->code_memory[get_code_memory_index_from_pc(cpu->pc)];
        strcpy(cpu->fetch.opcode_str, current_ins->opcode_str);
        cpu->fetch.opcode = current_ins->opcode;
        cpu->fetch.rd = current_ins->rd;
        cpu->fetch.rs1 = current_ins->rs1;
        cpu->fetch.rs2 = current_ins->rs2;
        cpu->fetch.imm = current_ins->imm;

        /* Update PC for next instruction */
        cpu->pc += 4;

        /* Copy data from fetch latch to decode latch*/
        cpu->decode = cpu->fetch;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Fetch", &cpu->fetch);
        }

        /* Stop fetching new instructions if HALT is fetched */
        if (cpu->fetch.opcode == OPCODE_HALT)
        {
            cpu->fetch.has_insn = FALSE;
        }

        if (cpu->fetch.opcode == OPCODE_NOP)
        {
            cpu->decode.has_insn = FALSE;
        }
    }
}

/*
 * Decode Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_decode(APEX_CPU *cpu)
{
    if (cpu->decode.has_insn)
    {
        /* Read operands from register file based on the instruction type */
        switch (cpu->decode.opcode)
        {
            case OPCODE_ADD:
            case OPCODE_ADDL:
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                break;
            }

            case OPCODE_SUB:
            case OPCODE_SUBL:
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                break;
            }

            case OPCODE_OR:
            case OPCODE_XOR:
            case OPCODE_AND:
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                break;
            }

            case OPCODE_MUL:
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                break;
            }

            case OPCODE_LOAD:
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                printf("hi");
                // cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                break;
            }

            case OPCODE_STORE:
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                printf("%d",cpu->decode.rs1_value);
                printf("%d",cpu->decode.rs2_value);
                break;
            }

            case OPCODE_STOREP:
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                // printf("%d",cpu->decode.rs1_value);
                // printf("%d",cpu->decode.rs2_value);
                break;
            }

            case OPCODE_LOADP:
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rd_value = cpu->regs[cpu->decode.rd];
                break;
            }

            case OPCODE_MOVC:
            {
                /* MOVC doesn't have register operands */
                break;
            }

            case OPCODE_CMP:
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rs2_value = cpu->regs[cpu->decode.rs2];
                break;
            }

            case OPCODE_CML:
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                break;
            }

            case OPCODE_JUMP:
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                break;
            }

            case OPCODE_JALR:
            {
                cpu->decode.rs1_value = cpu->regs[cpu->decode.rs1];
                cpu->decode.rd_value = cpu->regs[cpu->decode.rd];
                break;
            }
        }

        /* Copy data from decode latch to execute latch*/
        cpu->execute = cpu->decode;
        cpu->decode.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Decode/RF", &cpu->decode);
        }
    }
}

/*
 * Execute Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_execute(APEX_CPU *cpu)
{
    if (cpu->execute.has_insn)
    {
        /* Execute logic based on instruction type */
        switch (cpu->execute.opcode)
        {
            case OPCODE_ADD:
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value + cpu->execute.rs2_value;

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_ADDL:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value + cpu->execute.imm;
                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }
            
            case OPCODE_SUB:
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value - cpu->execute.rs2_value;

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_SUBL:
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value - cpu->execute.imm;
                    
                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_MUL:
            {
                cpu->execute.result_buffer
                    = cpu->execute.rs1_value - cpu->execute.rs2_value;
                break;
            }

            case OPCODE_LOAD:
            {
                cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.imm;
                printf("%d",cpu->execute.memory_address);
                break;
            }

            case OPCODE_LOADP:
            {
                /* Calculate the memory address by adding rs1_value and rs2_value */
                cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.imm;
                cpu->execute.rd = cpu->execute.rd_value + 4 ;
                break;
            }
            
            case OPCODE_STORE:
            {
                cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.imm;
                cpu->execute.result_buffer = cpu->execute.rs2_value;
                printf("%d",cpu->execute.result_buffer);
                printf("%d",cpu->execute.memory_address);
                break;
            }

            case OPCODE_STOREP:
            {
                cpu->execute.memory_address = cpu->execute.rs1_value + cpu->execute.imm;
                cpu->execute.result_buffer = cpu->execute.rs2_value;
                // printf("%d",cpu->execute.result_buffer);
                // printf("%d",cpu->execute.memory_address);
                break;
            }

            case OPCODE_JUMP:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value + cpu->execute.imm;

                cpu->pc = cpu->execute.result_buffer;

                cpu->fetch_from_next_cycle = TRUE;

                cpu->decode.has_insn = FALSE;

                cpu->fetch.has_insn = TRUE;

                break;
            }

            case OPCODE_JALR: 
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value + cpu->execute.imm;

                cpu->pc = cpu->execute.result_buffer;

                cpu->regs[cpu->execute.rd_value] = cpu->execute.pc + 4;

                cpu->fetch_from_next_cycle = TRUE;

                cpu->decode.has_insn = FALSE;

                cpu->fetch.has_insn = TRUE;

                break;
            }


            case OPCODE_BZ:
            {
                if (cpu->zero_flag == TRUE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;
                    
                    /* Since we are using reverse callbacks for pipeline stages, 
                     * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                break;
            }

            case OPCODE_BNZ:
            {
                if (cpu->zero_flag == FALSE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;
                    
                    /* Since we are using reverse callbacks for pipeline stages, 
                     * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                break;
            }

            case OPCODE_BP:
            {
                if (cpu->pos_flag == TRUE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;
                    
                    /* Since we are using reverse callbacks for pipeline stages, 
                     * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                break;
            }

            case OPCODE_BNP:
            {
                if (cpu->pos_flag == FALSE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;
                    
                    /* Since we are using reverse callbacks for pipeline stages, 
                     * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                break;
            }

            case OPCODE_BN:
            {
                if (cpu->neg_flag == TRUE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;
                    
                    /* Since we are using reverse callbacks for pipeline stages, 
                     * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                break;
            }

            case OPCODE_BNN:
            {
                if (cpu->neg_flag == FALSE)
                {
                    /* Calculate new PC, and send it to fetch unit */
                    cpu->pc = cpu->execute.pc + cpu->execute.imm;
                    
                    /* Since we are using reverse callbacks for pipeline stages, 
                     * this will prevent the new instruction from being fetched in the current cycle*/
                    cpu->fetch_from_next_cycle = TRUE;

                    /* Flush previous stages */
                    cpu->decode.has_insn = FALSE;

                    /* Make sure fetch stage is enabled to start fetching from new PC */
                    cpu->fetch.has_insn = TRUE;
                }
                break;
            }

            case OPCODE_CMP:
            {
                if (cpu->execute.rs1_value == cpu->execute.rs2_value)
                {
                    cpu->zero_flag = TRUE;
                    cpu->neg_flag = FALSE;
                    cpu->pos_flag = FALSE;
                }

                else if (cpu->execute.rs1_value < cpu->execute.rs2_value)
                {
                    cpu->zero_flag = FALSE;
                    cpu->neg_flag = TRUE;
                    cpu->pos_flag = FALSE;
                }

                else
                {
                    cpu->zero_flag = FALSE;
                    cpu->neg_flag = FALSE;
                    cpu->pos_flag = TRUE;
                }
            }

            case OPCODE_CML:
            {
                if (cpu->execute.rs1_value == cpu->execute.imm)
                {
                    cpu->zero_flag = TRUE;
                    cpu->neg_flag = FALSE;
                    cpu->pos_flag = FALSE;
                }

                else if (cpu->execute.rs1_value < cpu->execute.imm)
                {
                    cpu->zero_flag = FALSE;
                    cpu->neg_flag = TRUE;
                    cpu->pos_flag = FALSE;
                }

                else
                {
                    cpu->zero_flag = FALSE;
                    cpu->neg_flag = FALSE;
                    cpu->pos_flag = TRUE;
                }
            }

            case OPCODE_MOVC: 
            {
                cpu->execute.result_buffer = cpu->execute.imm;

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }

            case OPCODE_OR:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value | cpu->execute.rs2_value;
                break;
            }

            case OPCODE_XOR:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value ^ cpu->execute.rs2_value;
                break;
            }

            case OPCODE_AND:
            {
                cpu->execute.result_buffer = cpu->execute.rs1_value & cpu->execute.rs2_value;

                /* Set the zero flag based on the result buffer */
                if (cpu->execute.result_buffer == 0)
                {
                    cpu->zero_flag = TRUE;
                } 
                else 
                {
                    cpu->zero_flag = FALSE;
                }
                break;
            }
        }

        /* Copy data from execute latch to memory latch*/
        cpu->memory = cpu->execute;
        cpu->execute.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Execute", &cpu->execute);
        }
    }
}

/*
 * Memory Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static void
APEX_memory(APEX_CPU *cpu)
{
    if (cpu->memory.has_insn)
    {
        switch (cpu->memory.opcode)
        {
            case OPCODE_ADD:
            {
                /* No work for ADD */
                break;
            }

            case OPCODE_LOAD:
            {
                /* Read from data memory */
                cpu->memory.result_buffer
                    = cpu->data_memory[cpu->memory.memory_address];
                printf("%d",cpu->memory.result_buffer);
                break;
            }
            case OPCODE_LOADP:
            {
                /* Read from data memory */
                cpu->memory.result_buffer
                    = cpu->data_memory[cpu->memory.memory_address];
                // printf("%d",cpu->memory.result_buffer);
                break;
            }

            case OPCODE_STORE:
            {
                /* Write to data memory */
                cpu->data_memory[cpu->memory.memory_address] = cpu->memory.result_buffer;
                break;
            }

            case OPCODE_STOREP:
            {
                int data_to_store = cpu->memory.result_buffer;

                cpu->data_memory[cpu->memory.memory_address] = data_to_store;
                break;
            }


        }

        /* Copy data from memory latch to writeback latch*/
        cpu->writeback = cpu->memory;
        cpu->memory.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Memory", &cpu->memory);
        }
    }
}

/*
 * Writeback Stage of APEX Pipeline
 *
 * Note: You are free to edit this function according to your implementation
 */
static int
APEX_writeback(APEX_CPU *cpu)
{
    if (cpu->writeback.has_insn)
    {
        /* Write result to register file based on instruction type */
        switch (cpu->writeback.opcode)
        {
            case OPCODE_ADD:
            case OPCODE_ADDL:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_SUB:
            case OPCODE_SUBL:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_MUL:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_LOAD:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_LOADP:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_MOVC: 
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_OR:
            case OPCODE_XOR:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_JALR:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }

            case OPCODE_AND:
            {
                cpu->regs[cpu->writeback.rd] = cpu->writeback.result_buffer;
                break;
            }
        }

        cpu->insn_completed++;
        cpu->writeback.has_insn = FALSE;

        if (ENABLE_DEBUG_MESSAGES)
        {
            print_stage_content("Writeback", &cpu->writeback);
        }

        if (cpu->writeback.opcode == OPCODE_HALT)
        {
            /* Stop the APEX simulator */
            return TRUE;
        }
    }

    /* Default */
    return 0;
}

/*
 * This function creates and initializes APEX cpu.
 *
 * Note: You are free to edit this function according to your implementation
 */
APEX_CPU *
APEX_cpu_init(const char *filename)
{
    int i;
    APEX_CPU *cpu;

    if (!filename)
    {
        return NULL;
    }

    cpu = calloc(1, sizeof(APEX_CPU));

    if (!cpu)
    {
        return NULL;
    }

    /* Initialize PC, Registers and all pipeline stages */
    cpu->pc = 4000;
    memset(cpu->regs, 0, sizeof(int) * REG_FILE_SIZE);
    memset(cpu->data_memory, 0, sizeof(int) * DATA_MEMORY_SIZE);
    cpu->single_step = ENABLE_SINGLE_STEP;

    /* Parse input file and create code memory */
    cpu->code_memory = create_code_memory(filename, &cpu->code_memory_size);
    if (!cpu->code_memory)
    {
        free(cpu);
        return NULL;
    }

    if (ENABLE_DEBUG_MESSAGES)
    {
        fprintf(stderr,
                "APEX_CPU: Initialized APEX CPU, loaded %d instructions\n",
                cpu->code_memory_size);
        fprintf(stderr, "APEX_CPU: PC initialized to %d\n", cpu->pc);
        fprintf(stderr, "APEX_CPU: Printing Code Memory\n");
        printf("%-9s %-9s %-9s %-9s %-9s\n", "opcode_str", "rd", "rs1", "rs2",
               "imm");

        for (i = 0; i < cpu->code_memory_size; ++i)
        {
            printf("%-9s %-9d %-9d %-9d %-9d\n", cpu->code_memory[i].opcode_str,
                   cpu->code_memory[i].rd, cpu->code_memory[i].rs1,
                   cpu->code_memory[i].rs2, cpu->code_memory[i].imm);
        }
    }

    /* To start fetch stage */
    cpu->fetch.has_insn = TRUE;
    return cpu;
}

/*
 * APEX CPU simulation loop
 *
 * Note: You are free to edit this function according to your implementation
 */
void
APEX_cpu_run(APEX_CPU *cpu)
{
    char user_prompt_val;

    while (TRUE)
    {
        if (ENABLE_DEBUG_MESSAGES)
        {
            printf("--------------------------------------------\n");
            printf("Clock Cycle #: %d\n", cpu->clock);
            printf("--------------------------------------------\n");
        }

        if (APEX_writeback(cpu))
        {
            /* Halt in writeback stage */
            printf("APEX_CPU: Simulation Complete, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
            break;
        }

        APEX_memory(cpu);
        APEX_execute(cpu);
        APEX_decode(cpu);
        APEX_fetch(cpu);

        print_reg_file(cpu);

        if (cpu->single_step)
        {
            printf("Press any key to advance CPU Clock or <q> to quit:\n");
            scanf("%c", &user_prompt_val);

            if ((user_prompt_val == 'Q') || (user_prompt_val == 'q'))
            {
                printf("APEX_CPU: Simulation Stopped, cycles = %d instructions = %d\n", cpu->clock, cpu->insn_completed);
                break;
            }
        }

        cpu->clock++;
    }
}

/*
 * This function deallocates APEX CPU.
 *
 * Note: You are free to edit this function according to your implementation
 */
void
APEX_cpu_stop(APEX_CPU *cpu)
{
    free(cpu->code_memory);
    free(cpu);
}