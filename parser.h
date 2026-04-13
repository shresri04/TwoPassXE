#include <string>
using namespace std;

struct ParsedLine{
    bool isComment = false;
    bool isBlank = false; 
};

class Parser {
    public: 
        static ParsedLine parseline(const string & line); 
    private: 
        static string trim(const string & str);
};

#endif