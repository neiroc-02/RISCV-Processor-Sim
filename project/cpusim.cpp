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

// Take in a bitset of any size and sign extend
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

/*FETCH STAGE: Fetch an ins from instMem given the current PC*/
bitset<32> fetch(int *instMem, int PC) // jjust wanted to be cool and use a pointer
{
	return bitset<32>(instMem[PC]);
}

/* DECODE STAGE: Identify R, I, S, B, U, J type instructions and return the control signals*/
bitset<9> alu_signals(const bitset<7> &opcode)
{
	// The bitset returned defines these control signals:
	//	- bitset[0] = REGWRITE
	//	- bitset[1] = ALUSRC
	//	- bitset[2] = BRANCH
	//	- bitset[3] = MEMREAD
	//	- bitset[4] = MEMWRITE
	//	- bitset[5] = MEMTOREG
	//	- bitset[6] = ALUOP //00 -> add, 01 -> sub, 10 -> check funct3 11 -> NoOP
	//	- bitset[7] = ALUOP
	//  - bitset[8] = JUMP

	// TODO: Figure out what ALUOP and J/U type control signals should be
	unordered_map<bitset<7>, bitset<9>> ctrl_signals = {
		{bitset<7>("0110011"), bitset<9>("100000xx0")}, // R-type instruction
		{bitset<7>("0010011"), bitset<9>("110000xx0")}, // I-type instruction
		{bitset<7>("0000011"), bitset<9>("110101xx0")}, // I-type instruction (Load)
		{bitset<7>("0100011"), bitset<9>("010010xx0")}, // S-type instruction
		{bitset<7>("1100011"), bitset<9>("001000xx0")}, // B-type instruction
		{bitset<7>("1101111"), bitset<9>("000000xx1")}, // J-type instruction (JAL)
		{bitset<7>("0110111"), bitset<9>()},		    // U-type instruction
	};
	return ctrl_signals[opcode];
}
/* DECODE STAGE: Determine if/what immediate needs to be generated*/
//NOTE: Correctly idenifies instructions now
int imm_gen(const bitset<32> &instruction)
{
	bitset<7> opcode;
	for (int i = 0; i < 7; i++){
		opcode[i] = instruction[i];
	}
	// I-type instruction: Needs Ins[31:20]
	if (opcode.to_ulong() == bitset<7>(0b0010011).to_ulong() || opcode.to_ulong() == bitset<7>(0b0000011).to_ulong()){
		cout << "I-type instruction\n";
		bitset<12> temp;
		for (int i = 0; i < 12; i++)
		{
			temp[i] = instruction[20 + i];
		}
		bitset<3> funct3; //Added check for SRAI
		for (int i = 0; i < 3; i++){
			funct3[i] = instruction[12 + i];
		}
		if (funct3.to_ulong() == 5){
			for (int i = 5; i < 12; i++){
				temp[i] = 0;
			}
		}
		return sign_extension(temp);
	}
	// S-type instruction: Needs Ins[31:25] and Ins[11:7]
	else if (opcode.to_ulong() == bitset<7>("0100011").to_ulong())
	{
		cout << "S-type instruction\n";
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
		cout << "B-type instruction\n";
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
		cout << "J-type instruction\n";
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
		cout << "U-type instruction\n";
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
		cout << "R-type instruction\n";
		return 0;
	}	
	else
	{
		cout << "I should not be here!" << endl;
		return -1;
	}
}

/*DECODE STAGE: ALU Control*/
/*
int alu_ctrl()
{

}
*/

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
		cout << "Inst: 0x" << setw(8) << setfill('0') << hex << instruction.to_ulong() << endl;

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
		// NOTE: HOW DO I ADD CONTROL SIGNALS WITH NO EXTRA BITS, look at funct 3 maybe for sw v. sb distinction
		// Getting the opcode
		bitset<7> opcode; // Getting the opcode for the controller...
		for (int i = 0; i < 8; i++)
		{
			opcode[i] = instruction[i];
		}
		// Getting the register locations
		bitset<5> rs1_idx;
		bitset<5> rs2_idx;
		bitset<5> rd_idx;
		for (int i = 0; i < 5; i++)
		{
			rs1_idx[i] = instruction[15 + i];
			rs2_idx[i] = instruction[20 + i];
			rd_idx[i] = instruction[7 + i];
		}
		bitset<4> alu_ctrl_in;
		alu_ctrl_in[3] = instruction[30];
		for (int i = 0; i < 3; i++)
		{
			alu_ctrl_in[i] = instruction[12 + i];
		}

		// Above looks good!

		cout << "Opcode: " << opcode << endl;
		//cout << rs1_idx << endl;
		//cout << rs2_idx << endl;
		//cout << rd_idx << endl;
		// cout << "Produced Signals: " << --- << endl;
		cout << "Produced Imm: " << dec << imm_gen(instruction) << endl;

		// EXECUTE STAGE:
		/*
			Using operands specified by controllers, run the ALU to complete task
		*/

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