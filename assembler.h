#ifndef ASSEMBLER_H
#define ASSEMBLER_H
#include <string>
#include <vector>
#include <unordered_map>
#include <set>

/*
 * Input: a source line from the SIC/XE program
 *
 * We will be looking at the:
 * - line number
 * - assigned address
 * - Label, opcode, operand
 * - original source line
 * - any errors found during Pass 1
 */
struct SourceLine {
    int lineNumber = 0;
    int address = -1;
    std::string label;
    std::string opcode;
    std::string operand;
    std::string comment;
    std::string originalLine;
    bool isComment = false;
    bool hasError = false;
    std::string errorMessage;
};


struct OpInfo {
    int opcode = 0;
    int format = 0;
};

//In pass 1 we record literals and assign addresses to them
struct LiteralEntry {
    std::string literal;
    std::string valueHex;
    int length = 0;
    int address = -1;
    bool assigned = false;
};

class Assembler {
public:
    Assembler();
    bool runPass1(const std::string& filename);

private:
    std::vector<SourceLine> lines;
    std::unordered_map<std::string, int> symtab;
    std::vector<LiteralEntry> littab;
    std::unordered_map<std::string, OpInfo> optab;
    std::set<std::string> directives;

    int startAddress = 0;
    int programLength = 0;
    std::string programName;

    void initOptab();
    void initDirectives();

    bool readSourceFile(const std::string& filename);
    SourceLine parseLine(const std::string& text, int lineNumber) const;

    // Pass 1
    bool pass1();

    //Output (makes easier for debugging)
    bool writeIntermediateFile(const std::string& sourceFilename) const;
    bool writeSymtabFile(const std::string& sourceFilename) const;

    //
    void addLiteralIfNeeded(const std::string& operand);
    void assignLiterals(int& locctr);

    // ===== Utility =====
    static std::string trim(const std::string& s);
    static std::string toUpper(const std::string& s);
    static bool isNumber(const std::string& s);
    static int hexToInt(const std::string& s);
    static std::string intToHex(int value, int width);
    static std::string getBaseName(const std::string& filename);

    // ===== Directive helpers =====
    bool evaluateByteDirective(const std::string& operand, int& outLen) const;
};

#endif