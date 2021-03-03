#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "mu-mips.h"

/***************************************************************/
/* Print out a list of commands available                                                                  */
/***************************************************************/
void help() {        
	printf("------------------------------------------------------------------\n\n");
	printf("\t**********MU-MIPS Help MENU**********\n\n");
	printf("sim\t-- simulate program to completion \n");
	printf("run <n>\t-- simulate program for <n> instructions\n");
	printf("rdump\t-- dump register values\n");
	printf("reset\t-- clears all registers/memory and re-loads the program\n");
	printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
	printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
	printf("high <val>\t-- set the HI register to <val>\n");
	printf("low <val>\t-- set the LO register to <val>\n");
	printf("print\t-- print the program loaded into memory\n");
	printf("show\t-- print the current content of the pipeline registers\n");
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory                                                                            */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) &&  ( address <= MEM_REGIONS[i].end) ) {
			uint32_t offset = address - MEM_REGIONS[i].begin;
			return (MEM_REGIONS[i].mem[offset+3] << 24) |
					(MEM_REGIONS[i].mem[offset+2] << 16) |
					(MEM_REGIONS[i].mem[offset+1] <<  8) |
					(MEM_REGIONS[i].mem[offset+0] <<  0);
		}
	}
	return 0;
}

/***************************************************************/
/* Write a 32-bit word to memory                                                                                */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
	int i;
	uint32_t offset;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) && (address <= MEM_REGIONS[i].end) ) {
			offset = address - MEM_REGIONS[i].begin;

			MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
			MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
			MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
			MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
		}
	}
}

/***************************************************************/
/* Execute one cycle                                                                                                              */
/***************************************************************/
void cycle() {                                                
	handle_pipeline();
	CURRENT_STATE = NEXT_STATE;
	CYCLE_COUNT++;
}

/***************************************************************/
/* Simulate MIPS for n cycles                                                                                       */
/***************************************************************/
void run(int num_cycles) {                                      
	
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped\n\n");
		return;
	}

	printf("Running simulator for %d cycles...\n\n", num_cycles);
	int i;
	for (i = 0; i < num_cycles; i++) {
		if (RUN_FLAG == FALSE) {
			printf("Simulation Stopped.\n\n");
			break;
		}
		cycle();
	}
}

/***************************************************************/
/* simulate to completion                                                                                               */
/***************************************************************/
void runAll() {                                                     
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped.\n\n");
		return;
	}

	printf("Simulation Started...\n\n");
	while (RUN_FLAG){
		cycle();
	}
	printf("Simulation Finished.\n\n");
}

/***************************************************************/ 
/* Dump a word-aligned region of memory to the terminal                              */
/***************************************************************/
void mdump(uint32_t start, uint32_t stop) {          
	uint32_t address;

	printf("-------------------------------------------------------------\n");
	printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
	printf("-------------------------------------------------------------\n");
	printf("\t[Address in Hex (Dec) ]\t[Value]\n");
	for (address = start; address <= stop; address += 4){
		printf("\t0x%08x (%d) :\t0x%08x\n", address, address, mem_read_32(address));
	}
	printf("\n");
}

/***************************************************************/
/* Dump current values of registers to the teminal                                              */   
/***************************************************************/
void rdump() {                               
	int i; 
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
	printf("# Cycles Executed\t: %u\n", CYCLE_COUNT);
	printf("PC\t: 0x%08x\n", CURRENT_STATE.PC);
	printf("-------------------------------------\n");
	printf("[Register]\t[Value]\n");
	printf("-------------------------------------\n");
	for (i = 0; i < MIPS_REGS; i++){
		printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
	}
	printf("-------------------------------------\n");
	printf("[HI]\t: 0x%08x\n", CURRENT_STATE.HI);
	printf("[LO]\t: 0x%08x\n", CURRENT_STATE.LO);
	printf("-------------------------------------\n");
}

/***************************************************************/
/* Read a command from standard input.                                                               */  
/***************************************************************/
void handle_command() {                         
	char buffer[20];
	uint32_t start, stop, cycles;
	uint32_t register_no;
	int register_value;
	int hi_reg_value, lo_reg_value;

	printf("MU-MIPS SIM:> ");

	if (scanf("%s", buffer) == EOF){
		exit(0);
	}

	switch(buffer[0]) {
		case 'S':
		case 's':
			if (buffer[1] == 'h' || buffer[1] == 'H'){
				show_pipeline();
			}else {
				runAll(); 
			}
			break;
		case 'M':
		case 'm':
			if (scanf("%x %x", &start, &stop) != 2){
				break;
			}
			mdump(start, stop);
			break;
		case '?':
			help();
			break;
		case 'Q':
		case 'q':
			printf("**************************\n");
			printf("Exiting MU-MIPS! Good Bye...\n");
			printf("**************************\n");
			exit(0);
		case 'R':
		case 'r':
			if (buffer[1] == 'd' || buffer[1] == 'D'){
				rdump();
			}else if(buffer[1] == 'e' || buffer[1] == 'E'){
				reset();
			}
			else {
				if (scanf("%d", &cycles) != 1) {
					break;
				}
				run(cycles);
			}
			break;
		case 'I':
		case 'i':
			if (scanf("%u %i", &register_no, &register_value) != 2){
				break;
			}
			CURRENT_STATE.REGS[register_no] = register_value;
			NEXT_STATE.REGS[register_no] = register_value;
			break;
		case 'H':
		case 'h':
			if (scanf("%i", &hi_reg_value) != 1){
				break;
			}
			CURRENT_STATE.HI = hi_reg_value; 
			NEXT_STATE.HI = hi_reg_value; 
			break;
		case 'L':
		case 'l':
			if (scanf("%i", &lo_reg_value) != 1){
				break;
			}
			CURRENT_STATE.LO = lo_reg_value;
			NEXT_STATE.LO = lo_reg_value;
			break;
		case 'P':
		case 'p':
			print_program(); 
			break;
		default:
			printf("Invalid Command.\n");
			break;
	}
}

/***************************************************************/
/* reset registers/memory and reload program                                                    */
/***************************************************************/
void reset() {   
	int i;
	/*reset registers*/
	for (i = 0; i < MIPS_REGS; i++){
		CURRENT_STATE.REGS[i] = 0;
	}
	CURRENT_STATE.HI = 0;
	CURRENT_STATE.LO = 0;
	
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
	
	/*load program*/
	load_program();
	
	/*reset PC*/
	INSTRUCTION_COUNT = 0;
	CURRENT_STATE.PC =  MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/***************************************************************/
/* Allocate and set memory to zero                                                                            */
/***************************************************************/
void init_memory() {                                           
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		MEM_REGIONS[i].mem = malloc(region_size);
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
}

/**************************************************************/
/* load program into memory                                                                                      */
/**************************************************************/
void load_program() {                   
	FILE * fp;
	int i, word;
	uint32_t address;

	/* Open program file. */
	fp = fopen(prog_file, "r");
	if (fp == NULL) {
		printf("Error: Can't open program file %s\n", prog_file);
		exit(-1);
	}

	/* Read in the program. */

	i = 0;
	while( fscanf(fp, "%x\n", &word) != EOF ) {
		address = MEM_TEXT_BEGIN + i;
		mem_write_32(address, word);
		printf("writing 0x%08x into address 0x%08x (%d)\n", word, address, address);
		i += 4;
	}
	PROGRAM_SIZE = i/4;
	printf("Program loaded into memory.\n%d words written into memory.\n\n", PROGRAM_SIZE);
	fclose(fp);
}

/************************************************************/
/* maintain the pipeline                                                                                           */ 
/************************************************************/
void handle_pipeline()
{
	/*INSTRUCTION_COUNT should be incremented when instruction is done*/
	/*Since we do not have branch/jump instructions, INSTRUCTION_COUNT should be incremented in WB stage */
	
	WB();
	MEM();
	EX();
	ID();
	IF();
}

//writing back result into memory, increment instruction count at this stage
/************************************************************/
/* writeback (WB) pipeline stage:                                                                          */ 
/************************************************************/
void WB()
{
	/*IMPLEMENT THIS*/
}

//memory accessed
/************************************************************/
/* memory access (MEM) pipeline stage:                                                          */ 
/************************************************************/
void MEM()
{
	/*IMPLEMENT THIS*/
}

//instruction executed
/************************************************************/
/* execution (EX) pipeline stage:                                                                          */ 
/************************************************************/
void EX()
{
	/*IMPLEMENT THIS*/
}

//This is where the instruction is actually determined by the bit fields
/************************************************************/
/* instruction decode (ID) pipeline stage:                                                         */ 
/************************************************************/
void ID()
{
	/*IMPLEMENT THIS*/
}

/************************************************************/
/* instruction fetch (IF) pipeline stage:                                                              */ 
/************************************************************/
void IF()
{
	ID_IF.IR = mem_read_32(CURRENT_STATE.PC)
	ID_IF.PC = CURRENT_STATE.PC; //putting program counter into pipeline regs DON'T KNOW IF NEEDED YET
	CURRENT_STATE.PC = CURRENT_STATE.PC + 4; //incrementing program counter by four
	/*IMPLEMENT THIS*/
}


/************************************************************/
/* Initialize Memory                                                                                                    */ 
/************************************************************/
void initialize() { 
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in MIPS assembly format)    */ 
/************************************************************/
void print_program(){
	/*IMPLEMENT THIS*/
	
	//From lab 1, need to fix leading zeroes problem
	uint32_t instruction = (mem_read_32(addr)); //reading in address from mem
	
	//creating bit massk
	unsigned opcode_mask = createMask(26,31); //last six bits mask, opcode
	unsigned rs_mask = createMask(21,25);
	unsigned rt_mask = createMask(16,20);
	unsigned imm_mask = createMask(0,15);	
	unsigned base_mask = createMask(21,25);
	unsigned offset_mask = createMask(0,15);
	unsigned target_mask = createMask(0,26);	
	unsigned sa_mask = createMask(6,10);
	unsigned branch_mask = createMask(16,20);	
	unsigned func_mask = createMask(0,5);
	unsigned rd_mask = createMask(11,15);
	

	//applying masks to get parts of command	
	unsigned opcode = applyMask(opcode_mask, instruction);
	unsigned rs = applyMask(rs_mask, instruction);
	unsigned rt = applyMask(rt_mask, instruction);
	unsigned immediate = applyMask(imm_mask, instruction);
	unsigned base = applyMask(base_mask, instruction);
	unsigned offset = applyMask(offset_mask, instruction);
	unsigned target = applyMask(target_mask, instruction);
	unsigned sa = applyMask(sa_mask, instruction);
	unsigned branch = applyMask(branch_mask, instruction);
	unsigned func = applyMask(func_mask, instruction);
	unsigned rd = applyMask(rd_mask, instruction);
	
	//printf("opcode = %x      ", opcode);
	
	switch(opcode)
	{
		case 0x20000000: //add ADDI
		{
			printf("ADDI ");
			printf("$%x $%x 0x%x\n", rs, rt, immediate); 
			break;
		}	
		case 0x24000000: //ADDIU
		{
			printf("ADDIU ");
			printf("$%x $%x 0x%04x\n", rs, rt, immediate); 
			break;
		}	
		case 0x30000000: //ANDI
		{
			printf("ANDI ");
			printf("$%x $%x 0x%x\n", rs, rt, immediate); 
			break;
		}
		case 0x34000000: //ORI
		{
			printf("ORI ");
			printf("$%x $%x, 0x%04x\n", rs, rt, immediate); 
			break;
		}
		case 0x38000000: //XORI
		{
			printf("XORI ");
			printf("$%x $%x 0x%x\n", rs, rt, immediate);
			break; 
		}
		case 0x28000000: //STLI set on less than immediate
		{
			printf("STLI ");
			printf("$%x $%x 0x%x\n", rs, rt, immediate); 
			break;
		}
		case 0x8C000000: //Load Word - for now on is load/store instructions mostly
		{
			printf("LW ");
			printf("$%x 0x%x $%x\n", rt, offset, base); 
			break;
		}
		case 0x80000000: //Load Byte LB
		{
			printf("LB ");
			printf("$%x, 0x%x $%x\n", rt, offset, base); 
			break;
		}
		case 0x84000000: //Load halfword
		{
			printf("LH ");
			printf("$%x 0x%x $%x)\n", rt, offset, base); 
			break;
		}
		case 0x3C000000: //LUI Load Upper Immediate, was 0F
		{
			printf("LUI ");
			printf("$%x 0x%x\n", rt, immediate); 
			break;
		}
		case 0xAC000000: //SW Store Word
		{
			printf("SW ");
			printf("$%x 0x%x $%x\n", rt, offset, base); 
			break;
		}
		case 0xA0000000: //SB Store Byte CHECK FORMAT
		{
			printf("SB ");
			printf("$%x 0x%x $%x\n", rt, offset, base); 
			break;
		}
		case 0xA4000000: //SH Store Halfword CHECK FORMAT
		{
			printf("SH ");
			printf("$%x 0x%x $%x)\n", rt, offset, base); 
			break;
		}
		case 0x10000000: //BEQ Branch if equal - start of branching instructions
		{
			printf("BEQ ");
			printf("$%x $%x 0x%04x\n", rs, rt, offset); 
			break;
		}
		case 0x14000000: //BNE Branch on Not Equal
		{
			printf("BNE ");
			printf("$%x $%x 0x%04x\n", rs, rt, offset); 
			break;
		}
		case 0x18000000: //BLEZ Brnach on Less than or equal to zero 
		{
			printf("BLEZ ");
			printf("$%x 0x%x\n", rs, offset); 
			break;
		}
		case 0x4000000: //special branch cases
		{	
			switch(branch)
			{
				case 0x00: //BLTZ Brnach on Less than zero
				{
					printf("BLTZ ");
					printf("#%x 0x%x\n", rs, offset); 
					break;
				}
				case 0x10000: // BGEZ Branch on greater than or equal zero
				{
					printf("BGEZ ");
					printf("$%x 0x%x\n", rs, offset); 
					break;
				}
			}
			break;	
			
		}
		case 0x1C000000: //BGTZ Branch on Greater than Zero
		{
			printf("BLEZ ");
			printf("$%x 0x%x\n", rs, offset); 
			break;
		}
		case 0x08000000: //Jump J (bum bum bummmm bum, RIP Eddie VanHalen)
		{
			printf("J ");
			printf("0x%x\n", (addr & 0xF0000000) | (target));
			break;
		}
		case 0x0C000000: //JAL Jump and Link
		{
			printf("JAL ");
			printf("0x%x\n", (addr & 0xF0000000) | (target));
			break;
		}
		case 0x00000000: //special case when first six bits are 000000, function operations
		{
			switch(func)
			{
				case 0x20: //ADD
				{
					printf("ADD ");
					printf("$%x $%x $%x\n", rd, rs, rt);
					break;
				}
				case 0x21: //ADDU
				{
					printf("ADDU ");
					printf("$%x $%x $%x\n", rd, rs, rt);
					break;
				}
				case 0x22: //SUB
				{
					printf("SUB ");
					printf("$%x $%x $%x\n", rd, rs, rt);
					break;
				}
				case 0x23: //SUBU
				{
					printf("SUBU ");
					printf("$%x $%x $%x\n", rd, rs, rt);
					break;
				}
				case 0x18: //MULT
				{
					printf("MULT ");
					printf("$%x $%x\n", rs, rt); 
					break;
				}
				case 0x19: //MULTU
				{
					printf("MULTU ");
					printf("$%x $%x\n", rs, rt); 
					break;
				}
				case 0x1A: //DIV
				{
					printf("DIV ");
					printf("$%x $%x\n", rs, rt); 
					break;
				}
				case 0x1B: //DIVU
				{
					printf("DIVU ");
					printf("$%x $%x\n", rs, rt); 
					break;	
				}
				case 0x24: //AND
				{
					printf("AND ");
					printf("$%x $%x $%x\n", rd, rs, rt);
					break;
				}
				case 0x25: //OR
				{
					printf("OR ");
					printf("$%x $%x $%x\n", rd, rs, rt);
					break;
				}
				case 0x26: //XOR
				{
					printf("XOR ");
					printf("$%x $%x $%x\n", rd, rs, rt);
					break;
				}
				case 0x27: //NOR
				{
					printf("NOR ");
					printf("$%x $%x $%x\n", rd, rs, rt);
					break;
				}
				case 0x2A: //SLT Set on less than
				{
					printf("SLT ");
					printf("$%x $%x $%x\n", rd, rs, rt);
					break;
				}
				case 0x00: //SLL Shift Left Logical NEED TO CHECK FORMAT
				{
					printf("SLL ");
					printf("$%x $%x %x\n", rd, rt, sa); //different from the rest with the sa thingy
					break;
				}
				case 0x02: //SRL Shift Right Logical CHECK FORMAT
				{
					printf("SRL ");
					printf("$%x $%x %x\n", rd, rt, sa); //different from the rest with the sa thingy
					break;
				}
				case 0x03: //SRA Shift Right Arithmetic
				{
					printf("SRA ");
					printf("$%x $%x %x\n", rd, rt, sa); //different from the rest with the sa thingy
					break;
				} 
				case 0x10: //MFHI Move from HI
				{
					printf("MFHI ");
					printf("$%x\n", rd); //only the rd register
					break;
				}
				case 0x12: //MFLO Move from LO
				{
					printf("MFHI ");
					printf("$%x\n", rd); //only the rd register
					break;
				}
				case 0x11: //MTHI Move to HI
				{
					printf("MFHI ");
					printf("$%x\n", rs); //only the rs register
					break;
				}
				case 0x13: //MTLO Move to LO
				{
					printf("MFHI ");
					printf("$%x\n", rs); //only the rs register
					break;
				}
				case 0x08: //JR Jump Register
				{
					printf("JR ");
					printf("%x\n", rs); //only the rs register
					break;
				}
				case 0x09: //JALR Jump and Link Register CHECK FORMAT
				{
					printf("JR ");
					if(rd == 0x1F) //if rd is all one's (or 31) then not given
					{
						printf("$%x\n", rs);
					}
					else //rd is given
						printf("$%x $%x\n", rd, rs);
					break;
				}
				case 0x0C: //SYSCALL
				{
					printf("SYSCALL\n");
					break;
				}
			}

			
			break;
		}			
		default:
		{
			printf("Command not found...\n");
			break;
		}
			
	}	
}

/************************************************************/
/* Print the current pipeline                                                                                    */ 
/************************************************************/
void show_pipeline(){
	/*IMPLEMENT THIS*/
	//ID_RF part of the lab
	printf("Current PC: %x\n", CURRENT_STATE.PC); //may need to be one the one from the pipeline regs
	printf("IF/ID %x\n", ID_ID.IR); //double check data type and what not for this one
	printf("IF/ID.PC %x\n", ID_IF.PC);
	
	//EX part
	printf("ID/EX.IR %x\n", ID_EX.IR);
	printf("ID/EX.A %x\n", ID_EX.A);
	printf("ID/EX.B %x\n", ID_EX.B);
	printf("ID/EX.imm %x\n", ID_EX.imm);
	
	//MEM
	printf("EX/MEM.IR %x\n", EX_MEM.IR);
	printf("EX/MEM.A %x\n", EX_MEM.A);
	printf("EX?MEM.B %x\n", EX_MEM.B);
	printf("ALUOutput %x\n", EX_MEM.ALUOutput);
	
	//WB
	printf("MEM/WB.IR %x\n", MEM_WB.IR);
	printf("MEM/WB.ALUOutput %x\n", MEM_WB.ALUOutput);
	printf("MEM/WB.LMD %x\n", MEM_WB.LMD);
	
}

/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {                              
	printf("\n**************************\n");
	printf("Welcome to MU-MIPS SIM...\n");
	printf("**************************\n\n");
	
	if (argc < 2) {
		printf("Error: You should provide input file.\nUsage: %s <input program> \n\n",  argv[0]);
		exit(1);
	}

	strcpy(prog_file, argv[1]);
	initialize();
	load_program();
	help();
	while (1){
		handle_command();
	}
	return 0;
}
