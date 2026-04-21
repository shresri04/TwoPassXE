/* CS530 Assignment 2 - Limited XE Assembler
 *
 *	Team:
 *		Lucas Fredricks (cssc2518)
 *		Shreya Sridharan (cssc2550)
 *
 *	symtab_output.cpp
 *	- Writes SYMTAB and LITTAB to .st
 *
 * Column layout — symbol table:
 *   0-7   CSect (program name on first row, blank otherwise)
 *   8-15  Symbol name
 *   16-23 Value  (6-hex uppercase, padded to 8 wide)
 *   24-31 LENGTH (6-hex uppercase, CSect row only)
 *   32+   Flags  ('R' for relocatable symbols, blank for CSect row)
 *
 * Column layout — literal table:
 *   0-5   Name    (content between quotes in =C'...' / =X'...')
 *   6-15  Operand (valueHex, 10 wide)
 *   16-24 Address (4-hex uppercase, 9 wide)
 *   25+   Length  (decimal)
 */

#include "assembler.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace std;

namespace {

constexpr int CSECT_W = 8;
constexpr int SYMBOL_W = 8;
constexpr int VALUE_W = 8;
constexpr int LENGTH_W = 8;

constexpr int LIT_NAME_W = 6;
constexpr int LIT_OPERAND_W = 10;
constexpr int LIT_ADDR_W = 9;


}

bool Assembler::writeSymtabFile(const string& sourceFilename) const {
    const string outName = getBaseName(sourceFilename) + ".st";
    ofstream out(outName);
    if (!out) {
        cerr << "Error: Unable to create SYMTAB file: " << outName << "\n";
        return false;
    }

    // SYMTAB
	out << "CSect   Symbol  Value   LENGTH  Flags:\n";
    out << string(38, '-') << "\n";

    // CSect row: program name, start address, program length, no flag
    if (!programName.empty()) {
        out << left  << setw(CSECT_W)  << programName
            << left  << setw(SYMBOL_W) << ""
            << left  << setw(VALUE_W)  << intToHex(startAddress, 6)
            << left  << setw(LENGTH_W) << intToHex(programLength, 6)
            << string(16, ' ')
            << "\n";
    }

    // Sort syms by address (ascending)
    vector<pair<string, int>> sorted(symtab.begin(), symtab.end());
    sort(sorted.begin(), sorted.end(),
         [](const pair<string,int>& a, const pair<string,int>& b) {
             return a.second < b.second;
         });

    for (const auto& entry : sorted) {
        out << left  << setw(CSECT_W)  << ""                      // 0-7  (blank)
            << left  << setw(SYMBOL_W) << entry.first             // 8-15
            << left  << setw(VALUE_W)  << intToHex(entry.second, 6) // 16-23
            << left  << setw(LENGTH_W) << ""                      // 24-31 (blank)
            << "R\n";
    }

    // LITTAB
    out << "\n";
    out << "Literal Table \n";
    out << "Name  Operand   Address  Length:\n";
    out << string(32, '-') << "\n";

    // Sort by address ascending
	// unassigned lits put last
    vector<LiteralEntry> sortedLits = littab;
    sort(sortedLits.begin(), sortedLits.end(),
         [](const LiteralEntry& a, const LiteralEntry& b) {
             if (a.assigned != b.assigned) return a.assigned > b.assigned;
             return a.address < b.address;
         });

    for (const auto& lit : sortedLits) {
        string name = lit.literal; // Change to const string if bellow is removed
		// May need to remove if we want full token instead of just inner part
		// also need to change LIT_NAME_W to 10 to accound for this
		// Start:
		size_t first = name.find('\'');
		size_t last = name.rfind('\'');
		if(first != string::npos && last != string::npos && last > first)
			name = name.substr(first + 1, last - first - 1);
		//end
        const string operand = toUpper(lit.valueHex);
        const string addr    = lit.assigned ? intToHex(lit.address, 4) : "----";

        out << left << setw(LIT_NAME_W)    << name
            << left << setw(LIT_OPERAND_W) << operand
            << left << setw(LIT_ADDR_W)    << addr
            << lit.length
            << "\n";
    }

    return true;
}
