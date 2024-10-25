#include "CPU.h"
#include <iostream>
#include <bitset>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map> // for necessary mappings
#include <iomanip>		 // for hex representation
using namespace std;

/* HELPER FUNCTION FOR SIGN EXTENSION */
template <size_t N>
int sign_extension(const bitset<N> &immediate)
{
	const int LENGTH = immediate.size();
	unsigned long value = immediate.to_ulong();
	if (immediate[LENGTH - 1] == 1)
	{
		value |= (~0UL << LENGTH); // shift all 1's by LENGTH to start the array with all 1's
	}
	return static_cast<int>(value);
}
/* FETCH STAGE: Fetch an ins from instMem given the current PC*/
bitset<32> fetch(int *instMem, int PC) // jjust wanted to be cool and use a pointer
{
	return bitset<32>(instMem[PC]);
}
/* DECODE STAGE: Identify R, I, S, B, U, J type instructions and return the control signals*/
bitset<9> controller(const bitset<7> &opcode)
{
	/*
	bitset[8] = REGWRITE
	bitset[7] = ALUSRC
	bitset[6] = BRANCH
	bitset[5] = MEMREAD
	bitset[4] = MEMWRITE
	bitset[3] = MEMTOREG
	bitset[2] = ALUOP[0] //00 -> add, 01 -> sub, 10 -> funct3, 11 -> NoOP??
	bitset[1] = ALUOP[1]
	bitset[0] = JUMP
	*/
	unordered_map<bitset<7>, bitset<9>> ctrl_signals = {
		{bitset<7>("0110011"), bitset<9>("100000100")}, // R-type instruction; varies, check funct3
		{bitset<7>("0010011"), bitset<9>("110000100")}, // I-type instruction; varies, check funct3
		{bitset<7>("0000011"), bitset<9>("110101000")}, // I-type instruction; (Load) ADD
		{bitset<7>("0100011"), bitset<9>("010010000")}, // S-type instruction (Store) ADD
		{bitset<7>("1100011"), bitset<9>("001000010")}, // B-type instruction (BEQ) SUB for equality
		{bitset<7>("1101111"), bitset<9>("000000111")}, // J-type instruction (JAL) NOOP
		{bitset<7>("0110111"), bitset<9>("110000110")}, // U-type instruction (LUI) NOOP
	};
	return ctrl_signals[opcode];
}
/* DECODE STAGE: Determine if/what immediate needs to be generated*/
int imm_gen(const bitset<32> &instruction)
{
	bitset<7> opcode;
	for (int i = 0; i < 7; i++)
	{
		opcode[i] = instruction[i];
	}
	// I-type instruction: Needs Ins[31:20]
	if (opcode.to_ulong() == bitset<7>(0b0010011).to_ulong() || opcode.to_ulong() == bitset<7>(0b0000011).to_ulong())
	{
		bitset<12> temp;
		for (int i = 0; i < 12; i++)
		{
			temp[i] = instruction[20 + i];
		}
		// FIXME: SRAI Check, not sure if I should return 3 or the one with the imm[11:5]=0x20
		/*
		bitset<3> funct3; //Added check for SRAI
		for (int i = 0; i < 3; i++){
			funct3[i] = instruction[12 + i];
		}
		if (funct3.to_ulong() == 5){
			for (int i = 5; i < 12; i++){
				temp[i] = 0;
			}
			//temp[10] = 1;
		}
		*/
		return sign_extension(temp);
	}
	// S-type instruction: Needs Ins[31:25] and Ins[11:7]
	else if (opcode.to_ulong() == bitset<7>("0100011").to_ulong())
	{
		bitset<12> temp;
		for (int i = 0; i < 4; i++)
		{
			temp[i] = instruction[7 + i];
		}
		for (int i = 4; i < 12; i++)
		{
			temp[i] = instruction[21 + i];
		}
		return sign_extension(temp);
	}
	// B-type instruction
	else if (opcode.to_ulong() == bitset<7>("1100011").to_ulong())
	{
		bitset<13> temp;
		temp[0] = 0;				// Bit 0 will always be 0
		temp[12] = instruction[31]; // Bit 12 will always come from ins[31]
		temp[11] = instruction[7];	// Bit 11 will always come from ins[7]
		// Bits 5:10 are located in 25:30 of the instruction
		for (int i = 5; i < 11; i++)
		{
			temp[i] = instruction[25 + i];
		}
		// Bits 1:4 are located in 8:11
		for (int i = 1; i < 5; i++)
		{
			temp[i] = instruction[8 + i];
		}
		return sign_extension(temp);
	}
	// J-type instruction
	else if (opcode.to_ulong() == bitset<7>("1101111").to_ulong())
	{
		bitset<21> temp;
		temp[20] = instruction[31]; // MSB will always be from ins[31]
		temp[0] = 0;				// LSB wil always be 0
		for (int i = 1; i < 11; i++)
		{
			temp[i] = instruction[20 + i];
		}
		temp[12] = instruction[19];
		for (int i = 12; i < 20; i++)
		{
			temp[i] = instruction[i];
		}
		return sign_extension(temp);
	}
	// U-type instruction: Needs ins[31:12]
	else if (opcode.to_ulong() == bitset<7>("0110111").to_ulong())
	{
		bitset<32> temp;
		for (int i = 12; i < 32; i++)
		{
			temp[i] = instruction[i];
		}
		return sign_extension(temp);
	}
	// R-type instruction: no immediate
	else if (opcode.to_ulong() == bitset<7>("0110011").to_ulong())
	{
		return 0;
	}
	else
	{
		cout << "I should not be here!" << endl;
		return -1;
	}
}
/* DECODE STAGE: ALU Control */
enum Operation { ADD, SUB, XOR, OR, RSR, NO_OP };
Operation alu_ctrl(const bitset<2> &alu_op, const bitset<4> &funct3_7)
{
	if (alu_op == bitset<2>("00")){
		return ADD;
	}
	else if (alu_op == bitset<2>("01")){
		return SUB;
	}
	else if (alu_op == bitset<2>("11")){
		return NO_OP;
	}
	else if (alu_op == bitset<2>("10")){
		cout << "Must check funct3" << endl;
		bitset<3> funct3;
		for (int i = 0; i < 3; i++){
			funct3[i] = funct3_7[i];
		}
		cout << funct3 << endl;
		if (funct3 == bitset<3>("000")){
			return ADD;
		}
		else if (funct3 == bitset<3>("100")){
			return XOR;
		}
		else if (funct3 == bitset<3>("110")){
			return OR;
		}
		else {
			cout << "Bad funct 3: " << funct3 << endl;
			return NO_OP;
		}
	}
	else {
		cout << "I should not be here!" << endl;
		return NO_OP;
	}
}
/* EXECUTE STAGE: ALU */
int alu(Operation op, int input_1, int input_2){

}

int main(int argc, char *argv[])
{
	/* This is the front end of your project. You need to first read the instructions that are stored in a file and load them into an instruction memory.*/
	int instMem[4096] = {0}; // Changed to int array of bytes

	if (argc < 2)
	{
		cout << "No file name entered. Exiting..." << endl;
		return -1;
	}

	ifstream infile(argv[1]); // open the file
	if (!(infile.is_open() && infile.good()))
	{
		cout << "Error opening file\n";
		return 0;
	}
	string line;
	int i = 0;
	while (infile)
	{
		// Read 4 lines at a time since we want to catch the whole instruction
		for (int j = 0; j < 4; j++)
		{
			infile >> line;
			if (!infile)
				break;
			stringstream line2(line);
			if (!line2)
				break;
			int x = 0;
			line2 >> hex >> x; // Pull out a hex integer
			int temp = instMem[i];
			instMem[i] = (x << (8 * j)) + temp; // Shift incoming val by 8 * i to correctly place it in instruction
		}
		if (!infile)
			break;
		i++;
	}
	int maxPC = i;

	/* Instantiate your CPU object here.  CPU class is the main class in this project that defines different components of the processor.
	CPU class also has different functions for each stage (e.g., fetching an instruction, decoding, etc.). */

	int register_file[32] = {0};
	CPU myCPU; // call the approriate constructor here to initialize the processor...

	/*
	Instructions to Implement: ADD, LUI, ORI, XOR, SRAI, LB, LW, SB, SW, BEQ, JAL
	*/

	bool done = false;
	while (!done) // processor's main loop. Each iteration is equal to one clock cycle.
	{
		// FETCH STAGE: CONVERT INSMEM TO HEX/BINARY BITSET
		bitset<32> instruction = fetch(instMem, myCPU.readPC());
		if (instruction.none())
			break;
		cout << "Inst: " << setw(8) << setfill('0') << hex << instruction.to_ulong() << endl;

		// DECODE STAGE:
		/*
			Break the instruction up into segments:
			- INS[6:0] -> Controller: convert to an array of control signals based on opcode
			- INS[19:15] -> Register Mapping: get reg val
			- INS[24:20] -> Register Mapping: get reg val
			- INS[11:7] -> Dest Register: save this as dest location
			- INS[31:0] -> ImmGen: ???, probably needs its own function to convert
			- INS[30, 14:12] -> ALU Control: convert this to an operation (add, sub, shift, etc.)
		*/
		/* Getting the opcode */
		bitset<7> opcode;
		for (int i = 0; i < 7; i++)
		{
			opcode[i] = instruction[i];
		}
		/* Getting the register locations */
		bitset<5> rs1_idx, rs2_idx, rd_idx;
		for (int i = 0; i < 5; i++)
		{
			rs1_idx[i] = instruction[15 + i];
			rs2_idx[i] = instruction[20 + i];
			rd_idx[i] = instruction[7 + i];
		}
		/* Getting input signal for ALU Controller */
		bitset<4> alu_ctrl_in;
		alu_ctrl_in[3] = instruction[30];
		for (int i = 0; i < 3; i++)
		{
			alu_ctrl_in[i] = instruction[12 + i];
		}
		/* Getting the immediate */
		int immediate = imm_gen(instruction);
		/* Getting the controller signals */
		bitset<9> control = controller(opcode);

		bitset<2> alu_op;
		alu_op[0] = control[1];
		alu_op[1] = control[2];
		
		Operation OP = alu_ctrl(alu_op, alu_ctrl_in);

		/* DEBUG PRINTS FOR DECODE PHASE */
		// Above looks good!
		//cout << "Opcode: " << opcode << endl;
		//cout << "Controller: " << control << endl;
		//cout << "ALU OP: " << alu_op << endl;
		//cout << "Operation: " << OP << endl;
		//cout << endl;
		// cout << rs1_idx << endl;
		// cout << rs2_idx << endl;
		// cout << rd_idx << endl;
		// cout << "Produced Imm: " << dec << imm_gen(instruction) << endl;


		// EXECUTE STAGE:
		/*
			Using operands specified by controllers, run the ALU to complete task
		*/

		/* Decide the input values for the ALU */
		int input_1 = register_file[rs1_idx.to_ulong()];
		int input_2 = 0;
		bool ALU_SRC = control[7];
		if (ALU_SRC){ 
			/* If ALU_SRC is 1, use the Immediate */
			input_2 = immediate;
		}
		else {
			/* Else, use the rs2 value */
			input_2 = register_file[rs2_idx.to_ulong()];
		}
		

		// STORE STAGE:
		/*
			Using control signals decide to store in memory or store in reg
		*/

		// UPDATE PC:
		/*
			Increment PC correctly: two cases...
			- PC + 4 for all ins except Branch/Jump (PC++ in this program)
			- PC + offset for Successful Branch/Jump ins (PC + offset/4 in this program)
		*/

		// ...
		myCPU.incPC();
		if (myCPU.readPC() > maxPC)
			break;
	}
	int a0 = register_file[10]; // a0 is x10
	int a1 = register_file[11]; // a1 is x11
	// print the results (you should replace a0 and a1 with your own variables that point to a0 and a1)
	cout << "(" << a0 << "," << a1 << ")" << endl;
	return 0;
}