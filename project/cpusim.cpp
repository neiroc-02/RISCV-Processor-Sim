#include "CPU.h"
#include <iostream>
#include <bitset>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip> //for hex representation
using namespace std;

/* Add all the required standard and developed libraries here */

/*FETCH STAGE: Fetch an ins from instMem given the current PC*/
int fetch(int* instMem, int PC){
	return instMem[PC];
}

/*
Put/Define any helper function/definitions you need here
*/
int main(int argc, char *argv[])
{
	/* This is the front end of your project.
	You need to first read the instructions that are stored in a file and load them into an instruction memory.
	*/

	/* Each cell should store 1 byte. You can define the memory either dynamically, or define it as a fixed size with size 4KB (i.e., 4096 lines). Each instruction is 32 bits (i.e., 4 lines, saved in little-endian mode).
	Each line in the input file is stored as an hex and is 1 byte (each four lines are one instruction). You need to read the file line by line and store it into the memory. You may need a mechanism to convert these values to bits so that you can read opcodes, operands, etc.
	*/

	int instMem[4096] = {0}; // Changed to int array of bytes

	if (argc < 2)
	{
		// cout << "No file name entered. Exiting...";
		return -1;
	}

	ifstream infile(argv[1]); // open the file
	if (!(infile.is_open() && infile.good()))
	{
		cout << "error opening file\n";
		return 0;
	}
	string line;
	int i = 0;
	while (infile)
	{
		//Read 4 lines at a time since we want to catch the whole instruction
		for (int j = 0; j < 4; j++)
		{
			infile >> line;
			if (!infile) break;
			stringstream line2(line);
			if (!line2) break;
			int x = 0;
			line2 >> hex >> x; // Pull out a hex integer
			int temp = instMem[i];
			instMem[i] = (x << (8 * j)) + temp; //Shift incoming val by 8 * i to correctly place it in instruction 
		}
		if (!infile) break;
		i++;
	}
	int maxPC = i;

	/* Instantiate your CPU object here.  CPU class is the main class in this project that defines different components of the processor.
	CPU class also has different functions for each stage (e.g., fetching an instruction, decoding, etc.). */
	
	int register_file[32] = {0};
	CPU myCPU; // call the approriate constructor here to initialize the processor...
	// make sure to create a variable for PC and resets it to zero (e.g., unsigned int PC = 0);

	/* OPTIONAL: Instantiate your Instruction object here. */
	// Instruction myInst;
	// Instructions to Implement:
	/*
	ADD, LUI, ORI, XOR, SRAI, LB, LW, SB, SW, BEQ, JAL
	*/

	bool done = true;
	while (done == true) // processor's main loop. Each iteration is equal to one clock cycle.
	{
		// FETCH STAGE: CONVERT INS MEM TO HEX
		int instruction = fetch(instMem, myCPU.readPC()); 
		cout << "Inst: " <<  setw(8) << setfill('0') << hex << instruction << endl;
		if (instruction == 0) break;

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
			- PC + offset for Branch/Jump ins
		*/

		// ...
		myCPU.incPC();
		if (myCPU.readPC() > maxPC)
			break;
	}
	int a0 = 0;
	int a1 = 0;
	// print the results (you should replace a0 and a1 with your own variables that point to a0 and a1)
	cout << "(" << a0 << "," << a1 << ")" << endl;

	return 0;
}