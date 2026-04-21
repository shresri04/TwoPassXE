/*
 * CS530 Assignment 2 - Limited XE Assembler
 *
 *	Team:
 *		Lucas Fredricks (cssc2518)
 *		Shreya Sridharan (cssc2550)
 *
 *	assembler.cpp
 *
 */

#include "assembler.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

using namespace std;

//Build optab
Assembler::Assembler() {
    initOptab();
    initDirectives();
}

void Assembler::initOptab() {
    optab["ADD"]   = {0x18, 3};
    optab["ADDF"]  = {0x58, 3};
    optab["ADDR"]  = {0x90, 2};
    optab["AND"]   = {0x40, 3};
    optab["CLEAR"] = {0xB4, 2};
    optab["COMP"]  = {0x28, 3};
    optab["COMPF"] = {0x88, 3};
    optab["COMPR"] = {0xA0, 2};
    optab["DIV"]   = {0x24, 3};
    optab["DIVF"]  = {0x64, 3};
    optab["DIVR"]  = {0x9C, 2};
    optab["FIX"]   = {0xC4, 1};
    optab["FLOAT"] = {0xC0, 1};
    optab["HIO"]   = {0xF4, 1};
    optab["J"]     = {0x3C, 3};
    optab["JEQ"]   = {0x30, 3};
    optab["JGT"]   = {0x34, 3};
    optab["JLT"]   = {0x38, 3};
    optab["JSUB"]  = {0x48, 3};
    optab["LDA"]   = {0x00, 3};
    optab["LDB"]   = {0x68, 3};
    optab["LDCH"]  = {0x50, 3};
    optab["LDF"]   = {0x70, 3};
    optab["LDL"]   = {0x08, 3};
    optab["LDS"]   = {0x6C, 3};
    optab["LDT"]   = {0x74, 3};
    optab["LDX"]   = {0x04, 3};
    optab["LPS"]   = {0xD0, 3};
    optab["MUL"]   = {0x20, 3};
    optab["MULF"]  = {0x60, 3};
    optab["MULR"]  = {0x98, 2};
    optab["NORM"]  = {0xC8, 1};
    optab["OR"]    = {0x44, 3};
    optab["RD"]    = {0xD8, 3};
    optab["RMO"]   = {0xAC, 2};
    optab["RSUB"]  = {0x4C, 3};
    optab["SHIFTL"]= {0xA4, 2};
    optab["SHIFTR"]= {0xA8, 2};
    optab["SIO"]   = {0xF0, 1};
    optab["SSK"]   = {0xEC, 3};
    optab["STA"]   = {0x0C, 3};
    optab["STB"]   = {0x78, 3};
    optab["STCH"]  = {0x54, 3};
    optab["STF"]   = {0x80, 3};
    optab["STI"]   = {0xD4, 3};
    optab["STL"]   = {0x14, 3};
    optab["STS"]   = {0x7C, 3};
    optab["STSW"]  = {0xE8, 3};
    optab["STT"]   = {0x84, 3};
    optab["STX"]   = {0x10, 3};
    optab["SUB"]   = {0x1C, 3};
    optab["SUBF"]  = {0x5C, 3};
    optab["SUBR"]  = {0x94, 2};
    optab["SVC"]   = {0xB0, 2};
    optab["TD"]    = {0xE0, 3};
    optab["TIO"]   = {0xF8, 1};
    optab["TIX"]   = {0x2C, 3};
    optab["TIXR"]  = {0xB8, 2};
    optab["WD"]    = {0xDC, 3};
}





//Directives
void Assembler::initDirectives() {
    directives = {
        "START", "END", "BYTE", "WORD", "RESB", "RESW",
        "BASE", "NOBASE", "LTORG"
    };
}

//Formatting:
//trim spaces
string Assembler::trim(const string& s) { 
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

//convert to uppercase
string Assembler::toUpper(const string& s) {
    string out = s;
    transform(out.begin(), out.end(), out.begin(),
              [](unsigned char c){ return static_cast<char>(toupper(c)); });
    return out;
}

//true if decimal
bool Assembler::isNumber(const string& s) {
    if (s.empty()) return false;
    size_t i = 0;
    if (s[0] == '+' || s[0] == '-') i = 1;
    if (i >= s.size()) return false;
    for (; i < s.size(); ++i) {
        if (!isdigit(static_cast<unsigned char>(s[i]))) return false;
    }
    return true;
}

//convert hex to int
int Assembler::hexToInt(const string& s) {
    int value = 0;
    stringstream ss;
    ss << hex << s;
    ss >> value;
    return value;
}

string Assembler::intToHex(int value, int width) {
    stringstream ss;
    ss << uppercase << hex << setw(width) << setfill('0') << value;
    return ss.str();
}


// Get filename without path and extension
string Assembler::getBaseName(const string& filename) {
    size_t slash = filename.find_last_of("/\\");
    string base = (slash == string::npos) ? filename : filename.substr(slash + 1);
    size_t dot = base.find_last_of('.');
    if (dot != string::npos) base = base.substr(0, dot);
    return base;
}


//Parse line by line into label / opcode / operand.
SourceLine Assembler::parseLine(const string& text, int lineNumber) const {
    SourceLine line;
    line.originalLine = text;
    line.lineNumber = lineNumber;


    if (text.empty()) {
        line.isComment = true;
        return line;
    }
    //If line begins with '.' => comment line
    if (text[0] == '.') {
        line.isComment = true;
        line.comment = text;
        return line;
    }
    //If line begins with whitespace => no label
    bool startsWithSpace = isspace(static_cast<unsigned char>(text[0]));

    vector<string> tokens;
    string token;
    stringstream ss(text);
    while (ss >> token) {
        tokens.push_back(token);
    }

    if (tokens.empty()) {
        line.isComment = true;
        return line;
    }

    if (startsWithSpace) {
        line.label = "";
        line.opcode = tokens.size() >= 1 ? tokens[0] : "";
        line.operand = tokens.size() >= 2 ? tokens[1] : "";
    } else {
        line.label = tokens.size() >= 1 ? tokens[0] : "";
        line.opcode = tokens.size() >= 2 ? tokens[1] : "";
        line.operand = tokens.size() >= 3 ? tokens[2] : "";
    }

    // lit pool entry: "* =C'EOF'" — label is *, 2nd token is the lit
    // shift to operand then clear opcode so pass 1 & 2 can detect by label 
    if (line.label == "*") {
        line.operand = line.opcode; 
        line.opcode = "";
        if (line.operand.empty() || line.operand[0] != '=') {
            line.hasError = true;
            line.errorMessage = "Literal-pool entry operand must begin with =";
        }
        return line;
    }

    line.opcode = toUpper(line.opcode);
    return line;
}


bool Assembler::readSourceFile(const string& filename) {
    lines.clear();

    ifstream in(filename);
	if (!in) {
        cerr << "Error: Can't open file '" << filename << "'\n";
        return false;
    }

    string text;
    int lineNumber = 1;
    while (getline(in, text)) {
        lines.push_back(parseLine(text, lineNumber));
        lineNumber++;
    }

    return true;
}

//Record literal operands

void Assembler::addLiteralIfNeeded(const string& operand) {
    if (operand.empty() || operand[0] != '=') return;

    for (const auto& lit : littab) {
        if (lit.literal == operand) return;
    }

    LiteralEntry entry;
    entry.literal = operand;
    bool valid = false;

    string body = operand.substr(1);

    if (body.size() >= 3 && body[1] == '\'' && body.back() == '\'') {
        char type = body[0];
        string inside = body.substr(2, body.size() - 3);

        if (type == 'C' || type == 'c') {
            entry.length = static_cast<int>(inside.size());
            string hexValue;
            for (char c : inside) {
                hexValue += intToHex(static_cast<unsigned char>(c), 2);
            }
            entry.valueHex = hexValue;
            valid = true; //made fix to ensure validity
        } else if (type == 'X' || type == 'x') {
            entry.length = static_cast<int>(inside.size()) / 2;
            entry.valueHex = toUpper(inside);
        }
    } else if (isNumber(body)) {
        entry.length = 3;
        entry.valueHex = intToHex(stoi(body), 6);
        valid = true;
    }

    if(valid) littab.push_back(entry);
}

//assign addresses to all unassigned literals
void Assembler::assignLiterals(int& locctr) {
    for (auto& lit : littab) {
        if (!lit.assigned) {
            lit.address = locctr;
            lit.assigned = true;
            locctr += lit.length;
        }
    }
}

//determine byte length
bool Assembler::evaluateByteDirective(const string& operand, int& outLen) const {
    outLen = 0;

    if (operand.size() < 3) return false;
    if (operand[1] != '\'' || operand.back() != '\'') return false;

    char type = operand[0];
    string inside = operand.substr(2, operand.size() - 3);

    if (type == 'C' || type == 'c') {
        outLen = static_cast<int>(inside.size());
        return true;
    }

    if (type == 'X' || type == 'x') {
        if (inside.size() % 2 != 0) return false;
        for (char c : inside) {
            if (!isxdigit(static_cast<unsigned char>(c))) return false;
        }
        outLen = static_cast<int>(inside.size()) / 2;
        return true;
    }

    return false;
}

/* ============================================================
 * ======================== Pass 1 START =======================
 * ============================================================
 */
bool Assembler::pass1() {
    symtab.clear();
    littab.clear();
    startAddress = 0;
    programLength = 0;
    programName = "";

    int locctr = 0;
    bool seenFirstLine = false;

    for (size_t i = 0; i < lines.size(); ++i) {
        SourceLine& line = lines[i];

        if (line.isComment) {
            continue;
        }

        // Lit pool entry:
		// label: "*"
		// opcode=""
        if (line.label == "*") {
            if (!line.hasError) {
                addLiteralIfNeeded(line.operand);
                bool found = false;
                for (auto& lit : littab) {
                    if (lit.literal == line.operand) {
                        if (lit.assigned && lit.address != locctr) {
                            line.hasError = true;
                            line.errorMessage = "Literal already placed at " + intToHex(lit.address, 4);
                        } else {
                            lit.address = locctr;
                            lit.assigned = true;
                            line.address = locctr;
                            locctr += lit.length;
                        }
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    line.hasError = true;
                    line.errorMessage = "Unable to resolve literal length for: " + line.operand;
                }
            }
            continue;
        }

        //First line
        if (!seenFirstLine) {
            seenFirstLine = true;

            if (line.opcode == "START") {
                if (!line.label.empty()) {
                    programName = line.label;
                }

                if (!line.operand.empty()) {
                    startAddress = hexToInt(line.operand);
                    locctr = startAddress;
                }

                line.address = locctr;
                continue;
            }
        }

        //LOCCTR is @ this address
        line.address = locctr;

        //If the line has a label insert to symtab. If there are duplicates, error.
        if (!line.label.empty()) {
            string symbol = toUpper(line.label);

            if (symtab.find(symbol) != symtab.end()) {
                line.hasError = true;
                line.errorMessage = "Error: Duplicate symbol: " + symbol;
            } else {
                symtab[symbol] = locctr;
            }
        }

        //Record literal
        addLiteralIfNeeded(line.operand);

        //Calculate memory
        string opcode = line.opcode;
        bool extended = false;

        if (!opcode.empty() && opcode[0] == '+') {
            extended = true;
            opcode = opcode.substr(1);
        }

        //Machine instruction from OPTAB
        if (optab.find(opcode) != optab.end()) {
            int fmt = optab[opcode].format;

            if (fmt == 1) {
                locctr += 1;
            } else if (fmt == 2) {
                locctr += 2;
            } else if (fmt == 3) {
                /*
                 * format 3 instruction = 3 bytes
                 * format 4 instruction = 4 bytes when '+' is used
                 */
                locctr += extended ? 4 : 3;
            }
        }

        //Assembler directives + locctr assignment
        else if (opcode == "WORD") {
            locctr += 3;
        }
        else if (opcode == "RESW") {
            if (!isNumber(line.operand)) {
                line.hasError = true;
                line.errorMessage = "Invalid operand for RESW";
            } else {
                locctr += 3 * stoi(line.operand);
            }
        }
        else if (opcode == "RESB") {
            if (!isNumber(line.operand)) {
                line.hasError = true;
                line.errorMessage = "Invalid operand for RESB";
            } else {
                locctr += stoi(line.operand);
            }
        }
        else if (opcode == "BYTE") {
            int len = 0;
            if (!evaluateByteDirective(line.operand, len)) {
                line.hasError = true;
                line.errorMessage = "Invalid BYTE operand";
            } else {
                locctr += len;
            }
        }
        else if (opcode == "BASE" || opcode == "NOBASE") {
            //Base and Nobase no effect on memory
        }
        else if (opcode == "LTORG") {
            assignLiterals(locctr);
        }
        else if (opcode == "END") {
            assignLiterals(locctr);
            break;
        }
        else {
            //opcode invalid
            line.hasError = true;
            if (line.errorMessage.empty()) {
                line.errorMessage = "Invalid opcode/directive: " + opcode;
            }
        }
    }

    programLength = locctr - startAddress;
    return true;
}
/* ============================================================
 * ========================= END OF Pass 1========================
 * ============================================================
 */
<<<<<<< HEAD
=======


/* ============================================================
 * ========================= Pass 2 ==========================
 * ============================================================
 */

>>>>>>> 095df4baa40f652f5afcc31f91e6637b52fde365
//Driver for pass 1
bool Assembler::writeIntermediateFile(const string& sourceFilename) const {
    string outName = getBaseName(sourceFilename) + ".int";
    ofstream out(outName);
    if (!out) {
        cerr << "Error: cannot create file '" << outName << "'\n";
        return false;
    }

    out << "Line  Address  Source\n";
    out << "----  -------  ---------------------------------------------\n";

    for (const auto& line : lines) {
        out << setw(4) << line.lineNumber << "  ";

        if (line.isComment || line.address < 0) {
            out << "       ";
        } else {
            out << uppercase << hex << setw(6) << setfill('0')
                << line.address << setfill(' ') << dec;
        }

        out << "   " << line.originalLine;

        if (line.hasError) {
            out << "    *** ERROR: " << line.errorMessage;
        }

        out << "\n";
    }

    out << "\nProgram Name : " << programName << "\n";
    out << "Start Addr   : " << uppercase << hex << startAddress << dec << "\n";
    out << "Program Size : " << uppercase << hex << programLength << dec << "\n";

    return true;


}

<<<<<<< HEAD




/* - read file
 * - perform Pass 1
 * - write intermediate file
 * - write symbol/literal table file
 */
=======
>>>>>>> 095df4baa40f652f5afcc31f91e6637b52fde365
bool Assembler::runPass1(const string& filename) {
    if (!readSourceFile(filename)) return false;
    if (!pass1()) return false;
    if (!writeSymtabFile(filename)) return false;
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "lxe: no source files were provided.\n";
        cout << "Please run the program again with one or more SIC/XE source files.\n";
        return 1;
    }
	
	//setting i=0 results in listing files also being made for lxe
	for (int i = 1; i < argc; ++i) {
		Assembler assembler; // new obj for each file

		if (!assembler.runPass1(argv[i])) {
			cerr << "Pass 1 failed for: " << argv[i] << "\n";
			continue;
		}
		if (!assembler.runPass2(argv[i])) {
			cerr << "Pass 2 failed for: " << argv[i] << "\n";
		}
	}

    return 0;
}


//Create symbol/literal table file here//

//Create symbol/literal table file here//



 /* ============================================================
 * ========================= Pass 2 ==========================
 * ============================================================
 */

