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
	unsigned long value = temp.to_ulong();
	if (immediate[LENGTH - 1] == 1)
	{
		value |= (~0UL << LENGTH); //shift all 1's by LENGTH to start the array with all 1's
	}
	return static_cast<int>(value)
}

/*FETCH STAGE: Fetch an ins from instMem given the current PC*/
bitset<32> fetch(int *instMem, int PC) // jjust wanted to be cool and use a pointer
{
	return bitset<32>(instMem[PC]);
}

/* DECODE STAGE: Identify R, I, S, B, U, J type instructions and return the control signals*/
bitset<8> alu_signals(const bitset<7> &opcode)
{
	// The bitset returned defines these control signals:
	//	- bitset[0] = REGWRITE
	//	- bitset[1] = ALUSRC
	//	- bitset[2] = BRANCH
	//	- bitset[3] = MEMREAD
	//	- bitset[4] = MEMWRITE
	//	- bitset[5] = MEMTOREG
	//    Maybe need 3 bits?
	//	- bitset[6] = ALUOP //00 -> add, 01 -> sub, 10 -> check funct3 11 -> NoOP
	//	- bitset[7] = ALUOP
	//  - bitset[8] = JUMP

	// TODO: Figure out what ALUOP and J/U type control signals should be
	unordered_map<bitset<7>, bitset<8>> ctrl_signals = {
		{bitset<7>("0110011"), bitset<8>("100000xx0")}, // R-type instruction
		{bitset<7>("0010011"), bitset<8>("110000xx0")}, // I-type instruction
		{bitset<7>("0000011"), bitset<8>("110101xx0")}, // I-type instruction (Load)
		{bitset<7>("0100011"), bitset<8>("010010xx0")}, // S-type instruction
		{bitset<7>("1100011"), bitset<8>("001000xx0")}, // B-type instruction
		{bitset<7>("1101111"), bitset<8>("000000xx1")}, // J-type instruction (JAL)
		{bitset<7>("0110111"), bitset<8>()},		    // U-type instruction
	};
	return ctrl_signals[opcode];
}

/* DECODE STAGE: Determine if/what immediate needs to be generated*/
int imm_gen(const bitset<7> &opcode, const bitset<32> &instruction)
{
	// I-type instruction: Needs Ins[31:20]
	if (opcode == bitset<7>("0010011") || opcode == bitset<7>("0000011"))
	{
		bitset<12> temp;
		for (int i = 0; i < 12; i++)
		{
			temp[i] = instruction[20 + i];
		}
		return sign_extension(temp);
	}
	// S-type instruction: Needs Ins[31:25] and Ins[11:7]
	else if (opcode == bitset<7>("0100011"))
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
	//TODO: B-type instruction: Needs Ins[3]
	else if (opcode == bitset<7>("1100011"))
	{
	}
	//TODO: J-type instruction
	else if (opcode == bitset<7>("1101111"))
	{
	}
	// U-type instruction: Needs ins[31:12]
	else if (opcode == bitset<7>("0110111"))
	{
		bitset<20> temp;
		for (int i = 0; i < 20; i++)
		{
			temp[i] = instruction[11 + i];
		}
		return sign_extension(temp);
	}
	// R-type instruction: no immediate
	else
	{
		return 0;
	}
}

int main(int argc, char *argv[])
{
	/* This is the front end of your project.
	You need to first read the instructions that are stored in a file and load them into an instruction memory.
	*/

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

	// make sure to create a variable for PC and resets it to zero (e.g., unsigned int PC = 0);
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
		for (int i = 0; i < 6; i++)
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
		bitset<4> alu_ctrl;
		alu_ctrl[3] = instruction[30];
		for (int i = 0; i < 3; i++)
		{
			alu_ctrl[i] = instruction[12 + i];
		}

		// Above looks good!
		/*
		cout << "Bitsets: " << endl;
		cout << opcode << endl;
		cout << rs1 << endl;
		cout << rs2 << endl;
		cout << rd << endl;
		*/

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
			- PC + 4 for all ins except Branch/Jump
			- PC + offset for Successful Branch/Jump ins
		*/

		// ...
		myCPU.incPC();
		if (myCPU.readPC() > maxPC)
			break;
	}
	int a0 = register_file[10]; //a0 is x10
	int a1 = register_file[11]; //a1 is x11
	// print the results (you should replace a0 and a1 with your own variables that point to a0 and a1)
	cout << "(" << a0 << "," << a1 << ")" << endl;
	return 0;
}