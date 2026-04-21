/* 
 * CS530 Assignment 2 - Limited XE Assembler
 *
 *	Team:
 *		Lucas Fredricks (cssc2518)
 *		Shreya Sridharan (cssc2550)
 *
 *
 * pass2.cpp
 *
 * Instructions: 
 *		- Format 1: 1-byte opcode 
 *		- Format 2: opcode + two 4 bit register nibbles 
 *		- Format 3: 24 bit PC or base relative 
 *		- Format 4: 32 bit absolute 20 bit address (needs leading +) 
 *
 *		Addressing modes: - simple (n = 1, i = 1)
 *		- Immed. # (n = 0, i = 1)
 *		- Indirect @ (n = 1, i = 0)
 *		- Index (x = 1)
 *		- Literal (littab)
 *
 */


#include "assembler.h"

#include <cctype>
#include <cstdint>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

namespace {
	
	// look up register num (form 2)
	// A=0, X=1, L=2, B=3, S=4, T=5, F=6, PC=8, SW=9, (unknown register rtrns -1)
	
int lookupRegister(const string& name) {
	static const unordered_map<string, int> regs = {
		{"A", 0}, {"X", 1}, {"L", 2}, {"B", 3}, 
		{"S", 4}, {"T", 5}, {"F", 6},
		{"PC", 8}, {"SW", 9}
	};
	auto it = regs.find(name);
	return (it == regs.end()) ? -1 : it->second;
}

	
// Parsed representation of oprand string
// Strip addressing mode and index suffix

struct ParsedOperand {
	string symbol;
	bool immediate = false;
	bool indirect = false;
	bool indexed = false;
	bool isLiteral = false;
	bool isNumeric = false;
};


ParsedOperand parseOperand(const string& raw) {
	ParsedOperand po;
	if (raw.empty()) return po;

	// match literals against LITTAb, include leading = sign
	if (raw[0] == '='){
		po.isLiteral = true;
		po.symbol = raw;
		return po;
	}

	string s = raw;

	// Get addressing mode prefix
	if (s[0] == '#') {
		po.immediate = true;
		s = s.substr(1);
	} else if (s[0] == '@') {
		po.indirect = true;
		s = s.substr(1);
	}
		
	// check for edge case, lower case X flag
	if (s.size() >= 2) {
		const size_t n = s.size();
		if (s[n-2] == ',' && (s[n-1] == 'X' || s[n-1] == 'x')) {
			po.indexed = true;
			s = s.substr(0, n - 2);
		}
	}
	po.symbol = s;

	// Check if remainder is decimal for immediate 3 (displacement)
	if (!s.empty()) {
		size_t i = (s[0] == '+' || s[0] == '-') ? 1 : 0;
		bool allDigits = (i < s.size());
		
		for (; i < s.size(); ++i) {
			if (!isdigit(static_cast<unsigned char>(s[i]))) {
				allDigits = false;
				break;
			}
		}
		po.isNumeric = allDigits;
	}

	return po;

	}

// Get LITTAB using linear scan via full token. None found = nullptr

const LiteralEntry* findLiteral(const vector<LiteralEntry>& littab,
								const string& literal) {
	for (const auto& lit : littab) {
		if (lit.literal == literal) return &lit;
	}
	return nullptr;
}

// Int set to fixed width uppercase hex string
string toHexFixed(unsigned value, int width){
	ostringstream oss;
	oss << uppercase << hex;
	oss.width(width);
	oss.fill('0');
	oss << value;
	return oss.str();
}


}

// Inject lit pool rows for lits placed by LTORG/END
// skip lits its that already appear as a * label line
void Assembler::injectLiteralLines() {
	if (littab.empty()) return;

	// Build address -> literal map for assigned literals
	unordered_map<int, const LiteralEntry*> litByAddr;
	for (const auto& lit : littab) {
		if (lit.assigned) litByAddr[lit.address] = &lit;
	}
	if (litByAddr.empty()) return;

	// Build set of literal tokens already in lines[] as explicit * source lines
	set<string> nativeLiterals;
	for (const auto& L : lines) {
		if (L.label == "*" && !L.operand.empty()) {
			nativeLiterals.insert(L.operand);
		}
	}

	vector<SourceLine> rebuilt;
	rebuilt.reserve(lines.size() + litByAddr.size());

	for (size_t i = 0; i < lines.size(); ++i) {
		const SourceLine& L = lines[i];
		rebuilt.push_back(L);

		const string op = toUpper(L.opcode);
		if (op != "LTORG" && op != "END") continue;

		int endAddr = startAddress + programLength;
		for (size_t j = i + 1; j < lines.size(); ++j) {
			if (!lines[j].isComment && lines[j].address >= 0) {
				endAddr = lines[j].address;
				break;
			}
		}

		int start = (L.address >= 0) ? L.address : 0;
		for (int addr = start; addr < endAddr; ++addr) {
			auto it = litByAddr.find(addr);
			if (it == litByAddr.end()) continue;
			const LiteralEntry* lit = it->second;

			// skip if already present as explicit * source line
			if (nativeLiterals.count(lit->literal)) continue;

			SourceLine syn;
			syn.lineNumber = L.lineNumber;
			syn.address    = lit->address;
			syn.label      = "*";
			syn.opcode     = "*LTORG*";
			syn.operand    = lit->literal;
			rebuilt.push_back(syn);
		}
	}

	lines = std::move(rebuilt);
}

/* Object code helper - directives
 *
 * BYTE directive Object code for C and X forms
 * legth was calced and validated in P1
 * This produces the hex bytes
 */

string Assembler::assembleByte(const string& operand) const {
	if (operand.size() < 3 || operand[1] != '\'' || operand.back() != '\'') {
		return ""; // Error already handled in p1
	}
	const char type = operand[0];
	const string inside = operand.substr(2, operand.size() - 3);

	if (type =='C' || type == 'c') {
		string out;
		for (char c : inside) {
			out += intToHex(static_cast<unsigned char>(c), 2);
		}
		return out;
	}
	if (type == 'X' || type == 'x') {
		return toUpper(inside);
	}
	return "";
}

	
/*
 * WORD:
 *	- produces 3 bytes
 *	- use 2's comp for negative values. Mask to 24 bitsbefore formating
 */

string Assembler::assembleWord(const string& operand) const {
	if (!isNumber(operand)) return "";
	long long v = stoll(operand);
	unsigned masked = static_cast<unsigned>(v &0xFFFFFF);
	return toHexFixed(masked, 6);
}


// Object code helpers - Instructions

// Format 1: single opcode byte w/ no operand. Opcode byte in OPTAB is the 8-bit val
string Assembler::assembleFormat1(const OpInfo& op) {
	return toHexFixed(static_cast<unsigned>(op.opcode)& 0xFF, 2);
}

// Format 2: opcode + two 4 bit registers

string Assembler::assembleFormat2(const OpInfo& op, const string& operand,
									SourceLine& line) {
	string r1Str, r2Str;
	size_t comma = operand.find(',');
	if (comma == string::npos) {
		r1Str = operand;
		r2Str = "";
	} else {
		r1Str = operand.substr(0, comma);
		r2Str = operand.substr(comma + 1);
	}
	
	// Trim whitepace around comma
	r1Str = trim(r1Str);
	r2Str = trim(r2Str);

	// Handle SCV and SHIFT using the second field to take a num instead of reg.
	auto resolve= [&](const string& s) -> int {
		if (s.empty()) return 0;
		if (isNumber(s)) {
			int v = stoi(s);
			return (v >= 0 && v <= 15) ? v : -1;
		}
		return lookupRegister(toUpper(s));
	};
	
	int r1 = r1Str.empty() ? 0 : resolve(r1Str);
	int r2 = r2Str.empty() ? 0 : resolve(r2Str);

	if (r1 < 0 || r2 < 0) {
		line.hasError = true;
		line.errorMessage = "Format 2: invalid register '" + (r1 < 0 ? r1Str : r2Str) + "'";
		return "";
	}

	unsigned byte1 =static_cast<unsigned>(op.opcode) & 0xFF;
	unsigned byte2 = ((r1 & 0xF) << 4) | (r2 & 0xF);
	return toHexFixed(byte1, 2) + toHexFixed(byte2, 2);
}

/*
 * Format 3 & 4
 *
 * Byte 0 = opcode[7 : 2] | n | i
 * byte 1 = x | b | p | e| disp (f3)
 *		    x | b | p | e| address (f4)
 * byte 2 = disp or addr
 * byte 3 = addr (f4 only)
 */

string Assembler::assembleFormat3or4(SourceLine& line, const OpInfo& op, int pc, bool format4) {
	
	const ParsedOperand po = parseOperand(line.operand);

	// base ni bits (simple = 11, immed = 01, indirect = 10)
	unsigned n = 1, i = 1;
	if (po.immediate) {n = 0; i = 1; }
	else if (po.indirect) {n = 1; i = 0;}

	unsigned x = po.indexed ? 1u : 0u;
	unsigned b = 0, p = 0, e = format4 ? 1u : 0u;

	// Only allow index addressing with simple addr mode
	// reject early so error shows on the line where it happened
	if (po.indexed && (po.immediate || po.indirect)) {
		line.hasError = true;
		line.errorMessage = "Cannot use indexed addressing withj # or @";
		return "";
	}

	// Edge case: RSUB has no operand. ie. P1 parsed line.operand as empty sop po.symbol is empty too
	int disp = 0;
	bool dispResolved = false;

	if (po.symbol.empty()) {
		disp = 0;
		dispResolved = true;
	}
	//Immed. num: the operand is the disp, no PC/base
	//rel comp: the direct addressing of a cosnt
	else if (po.immediate && po.isNumeric) {
		disp = stoi(po.symbol);
		dispResolved = true;
		// protect from overflow
		int maxVal = format4 ? 0xFFFFF : 0xfff;
		int minVal = format4 ? 0 : -2048; //f3 pc relative is signed
		if (disp < minVal || disp > maxVal) {
			line.hasError = true;
			line.errorMessage = "Immediate value is out of range";
			return "";
		}
	}

	//Literal: resolve with LITTAB
	else if(po.isLiteral) {
		const LiteralEntry* lit = findLiteral(littab, po.symbol);
		if (!lit | !lit->assigned) {
			line.hasError = true;
			line.errorMessage = "Undefined or unsigned literal: " + po.symbol;
			return "";
		}
		int ta = lit->address;
		if (format4) {
			disp = ta;
			dispResolved = true;
		} else {
			int pcDisp = ta - pc;
			if (pcDisp >= -2048 && pcDisp < 2047) {
				disp = pcDisp & 0xFFF;
				p = 1;
				dispResolved = true;
			}
		}
		if (!dispResolved) {
			line.hasError = true;
			line.errorMessage = "Literal displacement is out of range";
			return "";
		}
	}
	// SYM ref needs to be looked up in SYMTAB then pc rel / base rel / f4
	else {
		auto it = symtab.find(toUpper(po.symbol));
		if (it == symtab.end()) {
			line.hasError = true;
			line.errorMessage = "Undefined Symbol: " + po.symbol;
			return "";
		}
		int ta = it->second;

		if (format4) {
			disp = ta;
			dispResolved = true;
		} else {
			int pcDisp = ta - pc;
			if (pcDisp >= -2048 && pcDisp <= 2047) {
				disp = pcDisp & 0xFFF;
				p = 1;
				dispResolved = true;
		} else if (baseSet) {
			int baseDisp = ta - baseValue;
			if (baseDisp >= 0 && baseDisp <= 4095) {
				disp = baseDisp;
				b = 1;
				dispResolved = true;
			}
		}
		if (!dispResolved){
			line.hasError = true;
			line.errorMessage = "Address is out of PC/ base relatiove range";
			return "";
			}
		}
	}

	// Assemble the bytes
	
	// Need to mask b/c the low two bits of opcode byte always 0
	unsigned opByte = (static_cast<unsigned>(op.opcode) & 0xFC) | (n << 1) | i;

	if (format4) {
		unsigned addr20 = static_cast<unsigned>(disp) & 0xFFFFF;
		unsigned byte1 = (x << 7) | (b << 6) |(p << 5) | (e << 4) | ((addr20 >> 16) & 0xF);
		unsigned byte2 = (addr20 >> 8) & 0xFF;
		unsigned byte3 = addr20 & 0xFF;
		return toHexFixed(opByte, 2)
			+ toHexFixed(byte1, 2)
			+ toHexFixed(byte2, 2)
			+ toHexFixed(byte3, 2);
	} else {
		unsigned disp12 = static_cast<unsigned>(disp) & 0xFFF;
		unsigned byte1 = (x << 7) | (b << 6) | (p << 5) | (e << 4) | ((disp12 >> 8) & 0xF);
		unsigned byte2 = disp12 & 0xFF;
		return toHexFixed(opByte, 2)
			+ toHexFixed(byte1, 2)
			+ toHexFixed(byte2, 2);
	}
}

string Assembler::assembleInstruction(SourceLine& line, int pc) {
	string op = line.opcode;
	bool format4 = !op.empty() && op[0] == '+';
	if (format4) op = op.substr(1);
	op = toUpper(op);

	auto it = optab.find(op);
	if (it == optab.end()) {
		line.hasError = true;
		line.errorMessage = "Unknown opcode: " + line.opcode;
		return "";
	}
	const OpInfo& info = it->second;

	//F4 needed for instructions marked as f3 by OPTAB
	if (format4 && info.format != 3) {
		line.hasError = true;
		line.errorMessage = "Format 4 does not allow '+' on this instuction";
		return "";
	}

	switch (info.format) {
		case 1: return assembleFormat1(info);
		case 2: return assembleFormat2(info, line.operand, line);
		case 3: return assembleFormat3or4(line, info, pc, format4);
		default:
			line.hasError = true;
			line.errorMessage = "Unsuported instuction format";
			return "";
	}
}

/* ==============================================================
 *			PASS 2 Main function
*  ==============================================================
*/

bool Assembler::pass2() {
	injectLiteralLines();
	objectCodes.assign(lines.size(), "");
	baseSet = false;
	baseValue = 0;

	for (size_t i = 0; i < lines.size(); ++i) {
		SourceLine& L = lines[i];

		if (L.isComment) continue;
		if (L.hasError) continue; // preserve p1 erros

		const string op = toUpper(L.opcode);

		// prog header directive: no obj code, name/ start already set in p2. 
		// Start wont apear past the first line
		if (op == "BASE"){
			auto it = symtab.find(toUpper(L.operand));
			if (it == symtab.end()) {
				L.hasError = true;
				L.errorMessage = "BASE: Undefined Symbol: " + L.operand;
			} else {
				baseValue = it->second;
				baseSet = true;
			}
			continue;
		}
		if (op == "NOBASE") {
			baseSet = false;
			continue;
		}
		// Storage res
		if (op == "RESB" || op == "RESW") continue;

		// Program header / end
		if (op == "START") continue;
		if (op == "END") continue;

		// LTORG
		if (op == "LTORG") continue;

		// Data
		if (op == "BYTE") {
			objectCodes[i] = assembleByte(L.operand);
			continue;
		}
		if (op == "WORD") {
			objectCodes[i] = assembleWord(L.operand);
			continue;
		}

		// Lit pool entry: label= "*", opcode empty, operand= literal token
		if (L.label == "*") {
			const LiteralEntry* lit = findLiteral(littab, L.operand);
			if (lit) {
				objectCodes[i] = toUpper(lit->valueHex);
			}
			continue;
		}

		// PC will be the next source line
		int pc = L.address;
		for (size_t j = i + 1; j < lines.size(); ++j) {
			if (lines[j].address >= 0 && !lines[j].isComment) {
				pc = lines[j].address;
				break;
			}
		}
		// If no next addr, fall back to end of prog
		if (pc == L.address) {
			pc = startAddress + programLength;
		}

		objectCodes[i] = assembleInstruction(L, pc);
	}

	return true;
}

// driver for pass 2 (assumes pass 1 has already been done on the same file)

bool Assembler::runPass2(const string& filename) {
	if (!pass2()) return false;
	if (!writeListingFile(filename)) return false;
	return true;
}




