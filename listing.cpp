/*
 *	CS530 Assignment 2 - Limited XE Assembler
 *
 *	Team:
 *		Lucas Fredricks (cssc2518)
 *		Shreya Sridharan (cssc2550)
 *
 * listing.cpp
 *	- Generates listing file after pass 2
 *
 * Column layout (0-indexed):
 *   0-3   : Address (4 hex uppercase) or 4 spaces
 *   4-7   : 4 spaces
 *   8-15  : Label (8 wide, left-justified)
 *   16    : '+' for format-4, ' ' otherwise; '=' for literal-pool entries
 *   17-24 : Opcode (8 wide, left-justified)
 *   25    : Operand prefix ('#'/'@'/'=' if present, else ' ')
 *   26-50 : Operand body (25 wide, left-justified)
 *   51+   : Object code
 */

#include "assembler.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

using namespace std;

namespace {

// Emit one formatted instruction/directive row.
// address < 0 → blank address field (used for END and comment-only lines).
void writeRow(ostream& out,
              int address,
              const string& label,
              bool extended,
              const string& opcode,
              const string& operandFull,
              const string& objectCode,
              const string& errorMsg) {

    // Format Collumns
	//
	//0-3: address
    if (address >= 0) {
        out << right << uppercase << hex << setw(4) << setfill('0') << address
            << setfill(' ') << dec;
    } else {
        out << "    ";
    }

    // 4-7: buffer
    out << "    ";

    // 8-15: label
    out << left << setw(8) << label;

    // 16: + or space
    out << (extended ? '+' : ' ');

    // 17-24: opcode
    out << left << setw(8) << opcode;

    // 25: operand char, and 26-50: operand body
    char prefix = ' ';
    string body = operandFull;
    if (!operandFull.empty() &&
        (operandFull[0] == '#' || operandFull[0] == '@' || operandFull[0] == '=')) {
        prefix = operandFull[0];
        body   = operandFull.substr(1);
    }
    out << prefix;
    out << left << setw(25) << body;

    // 51+: object code
    if (!objectCode.empty()) {
        out << right << objectCode;
    }

    if (!errorMsg.empty()) {
        out << " ** ERROR: " << errorMsg;
    }

    out << "\n";
}

// Emit a literal-pool entry row (* label, literal at col 16).
// operand is the raw literal
void writeLiteralRow(ostream& out,
                     int address,
                     const string& operand,
                     const string& objectCode) {
    // 0-3
    out << right << uppercase << hex << setw(4) << setfill('0') << address
        << setfill(' ') << dec;

    // 4-7
    out << "    ";

    // 8: *, 9-15: padding
    out << left << setw(8) << "*";

    // 16: = , 17+: rest of literal (strips the lead operand)
    string body = operand;
    if (!body.empty() && body[0] == '=') body = body.substr(1);

    out << '=';
    // Pad body so obj code will be at 51 
    out << left << setw(34) << body;

    if (!objectCode.empty()) {
        out << right << objectCode;
    }

    out << "\n";
}

}


 // Write listing file after P2
 // Returns true if successfull

bool Assembler::writeListingFile(const string& sourceFilename) const {
    const string outName = getBaseName(sourceFilename) + ".l";
    ofstream out(outName);
    if (!out) {
        cerr << "ERROR: Unable to create listing file: " << outName << "\n";
        return false;
    }

    for (size_t i = 0; i < lines.size(); ++i) {
        const SourceLine& L = lines[i];

        // Comments and blank lines printed as orignal
        if (L.isComment) {
            out << L.originalLine << "\n";
            continue;
        }

        string obj;
        if (i < objectCodes.size()) obj = objectCodes[i];

        // Literal-pool entry: label= * , operand=literal token
        if (L.label == "*") {
            writeLiteralRow(out, L.address, L.operand, obj);
            continue;
        }

        // set extended flag and clean
        bool extended = (!L.opcode.empty() && L.opcode[0] == '+');
        string mnemonic = extended ? L.opcode.substr(1) : L.opcode;

        // suppress address for END
        string opcUpper = mnemonic;
        for (char& c : opcUpper) c = (char)toupper((unsigned char)c);
        int addr = (opcUpper == "END") ? -1 : L.address;

        writeRow(out, addr, L.label, extended, mnemonic,
                 L.operand, obj, L.hasError ? L.errorMessage : string());
    }

    return true;
}
