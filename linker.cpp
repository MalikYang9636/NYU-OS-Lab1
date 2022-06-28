/* File: linker.cpp
 * Program: two-pass linker
 * Author: Zeyu Yang
 * Last Modified: 02/28/2021
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
using namespace std;

struct DefList{
    int defCount;
    vector<string> definition;
    vector<int> relAddress;
};

struct UList{
    int useCount;
    vector<string> usedSym;
};

struct ProList{
    int codeCount;
    vector<string> type;
    vector<string> instr;
};

void tokenizer(char* input[]);
bool parseToken();
bool parseDefinition(DefList& dl);
bool parseUse(UList& ul);
bool parseProgram(ProList& pl);
bool isNum(string tokenItem);
bool isSym(string tokenItem);
bool isIEAR(string tokenItem);
int turnToInt(string num);
void getSymbolTable(vector<DefList>& fDefList, vector<ProList>& fProList);
string trim(string& str);
bool symDuplicate(string symbol);
void symTooBig(vector<DefList>& fDefList, vector<ProList>& fProList);
void getMemoryMap(vector<UList>& fUList, vector<ProList>& fProList);
void printSymNotInUse(vector<DefList>& fDefList, vector<UList>& fUList);
string turnToString(int num);
string turnAllZero(string instr);
string addZero(string instr);

vector<int> rowNum;
vector<int> offset;
vector<string> token;
int finalPositionX;
int finalPositionY;
int tokenPointer;
vector<DefList> fileDefList;
vector<UList> fileUList;
vector<ProList> fileProList;
int num_instr = 0;
vector<string> tableSym;
vector<int> tableValue;
int finalLineLength = 0;

int main(int argc, char* argv[]) {
    /*     Pass One    */
    tokenizer(argv);
    if (parseToken()){/* If input is parsed successfully */
        /*     Pass Two    */
        symTooBig(fileDefList, fileProList);
        getSymbolTable(fileDefList, fileProList);
        getMemoryMap(fileUList, fileProList);
        printf("\n");
        printSymNotInUse(fileDefList, fileUList);
    }
}

/* Read input file */
void tokenizer(char* input[]){
    ifstream infile (input[1], ios::binary);
    if (infile.is_open()) {
        int row = 0;
        string line;
        int rowLength;
        while (getline(infile,line)){
            string lineTrim = line;
            if (trim(lineTrim) != ""){
                line = trim(line);
                finalLineLength = line.length();
            }else{
                finalLineLength = line.length();
                line = trim(line);
            }
            row++;
            rowLength = line.length();
            int pointer = 0;
            for (int k = 0; k < rowLength; k++) {
                if (line.substr(k, 1) != " " && line.substr(k, 1) != "\t"){
                    pointer = k;
                    break;
                }
            }
            for (int i = pointer; i < rowLength; i++){
                //cout << "i: " << i << endl;
                string tokenChar = line.substr(i, 1);
                string tokenValue;
                if (i < rowLength-1){
                    string tokenCharNext = line.substr(i+1, 1);
                    if ((tokenChar == " " || tokenChar == "\t") && (tokenCharNext != " " && tokenCharNext != "\t")){
                        tokenValue = line.substr(pointer, i-pointer);
                        rowNum.push_back(row);
                        offset.push_back(i-tokenValue.length()+1);
                        tokenValue = trim(tokenValue);
                        token.push_back(tokenValue);
                        pointer = i+1;
                    }
                } else{
                    tokenValue = line.substr(pointer, i-pointer+1);
                    rowNum.push_back(row);
                    offset.push_back(i-tokenValue.length()+2);
                    token.push_back(tokenValue);
                }
            }
        }

        finalPositionX = row;
        finalPositionY = finalLineLength + 1;
        infile.close();
    } else {
        cout << "Fail to open file." << endl;
    }
}

/* Parse every token
 * Identify definition list, use list and program list of each module
 * Report parse error if exists
 */
bool parseToken(){
    int totalToken = token.size();
    while (tokenPointer < totalToken){
        DefList dl;
        UList ul;
        ProList pl;
        if (parseDefinition(dl)){
            fileDefList.push_back(dl);
        } else{
            return false;
        }
        if (parseUse(ul)){
            fileUList.push_back(ul);
        } else{
            return false;
        }
        if (tokenPointer >= totalToken){
            printf("Parse Error line %d offset %d: %s\n",
                   finalPositionX, finalPositionY, "NUM_EXPECTED");
            return false;
        }
        if (parseProgram(pl)){
            fileProList.push_back(pl);
        } else{
            return false;
        }
    }
    return true;
}

/* Parse definition list */
bool parseDefinition(DefList& dl){
    int defCount = 0;
    if (!isNum(token[tokenPointer])) {
        printf("Parse Error line %d offset %d: %s\n",
               rowNum[tokenPointer], offset[tokenPointer], "NUM_EXPECTED");
        return false;
    } else{
        defCount = turnToInt(token[tokenPointer]);
    }
    if (defCount < 0){
        printf("Parse Error line %d offset %d: %s\n",
               rowNum[tokenPointer], offset[tokenPointer], "NUM_EXPECTED");
        return false;
    } else if (defCount > 16){
        printf("Parse Error line %d offset %d: %s\n",
               rowNum[tokenPointer], offset[tokenPointer], "TOO_MANY_DEF_IN_MODULE");
        return false;
    } else if (defCount == 0) {
        dl.defCount = defCount;
        tokenPointer = tokenPointer + 1;
        return true;
    } else {
        int defLength = tokenPointer + defCount*2 + 1;
        if (defLength > token.size() && isNum(token.back())){
            printf("Parse Error line %d offset %d: %s\n",
                   finalPositionX, finalPositionY, "SYM_EXPECTED");
            return false;
        }
        if (defLength > token.size() && isSym(token.back())){
            printf("Parse Error line %d offset %d: %s\n",
                   finalPositionX, finalPositionY, "NUM_EXPECTED");
            return false;
        }
        dl.defCount = defCount;
        for (int i = 0; i < defCount*2; i++) {
            if (i % 2 == 0 && !isSym(token[tokenPointer+i+1])){
                tokenPointer = tokenPointer + i + 1;
                printf("Parse Error line %d offset %d: %s\n",
                       rowNum[tokenPointer], offset[tokenPointer], "SYM_EXPECTED");
                return false;
            } else if (i % 2 == 0 && isSym(token[tokenPointer+i+1]) && token[tokenPointer+i+1].length() <= 16) {
                dl.definition.push_back(token[tokenPointer + i + 1]);
            } else if (i % 2 == 0 && isSym(token[tokenPointer+i+1]) && token[tokenPointer+i+1].length() > 16){
                tokenPointer = tokenPointer + i + 1;
                printf("Parse Error line %d offset %d: %s\n",
                       rowNum[tokenPointer], offset[tokenPointer], "SYM_TOO_LONG");
                return false;
            } else if (i % 2 != 0 && !isNum(token[tokenPointer+i+1])){
                tokenPointer = tokenPointer + i + 1;
                printf("Parse Error line %d offset %d: %s\n",
                       rowNum[tokenPointer], offset[tokenPointer], "NUM_EXPECTED");
                return false;
            } else {
                dl.relAddress.push_back(turnToInt(token[tokenPointer+i+1]));
            }
        }
        tokenPointer = tokenPointer + defCount*2 + 1;
    }
    return true;
}

/* Parse use list */
bool parseUse(UList& ul){
    int useCount = 0;
    if (!isNum(token[tokenPointer])) {
        printf("Parse Error line %d offset %d: %s\n",
               rowNum[tokenPointer], offset[tokenPointer], "NUM_EXPECTED");
        return false;
    } else{
        useCount = turnToInt(token[tokenPointer]);
    }
    if (useCount < 0){
        printf("Parse Error line %d offset %d: %s\n",
               rowNum[tokenPointer], offset[tokenPointer], "NUM_EXPECTED");
        return false;
    }else if (useCount > 16){
        printf("Parse Error line %d offset %d: %s\n",
               rowNum[tokenPointer], offset[tokenPointer], "TOO_MANY_USE_IN_MODULE");
        return false;
    } else if (useCount == 0) {
        ul.useCount = useCount;
        tokenPointer = tokenPointer + 1;
        return true;
    } else{
        int useLength = tokenPointer + useCount + 1;
        if (useLength > token.size()){
            printf("Parse Error line %d offset %d: %s\n",
                   finalPositionX, finalPositionY, "SYM_EXPECTED");
            return false;
        }
        ul.useCount = useCount;
        for (int i = 0; i < useCount; i++) {
            if (!isSym(token[tokenPointer+i+1])){
                tokenPointer = tokenPointer + i + 1;
                printf("Parse Error line %d offset %d: %s\n",
                       rowNum[tokenPointer], offset[tokenPointer], "SYM_EXPECTED");
                return false;
            } else if (isSym(token[tokenPointer+i+1]) && token[tokenPointer+i+1].length() > 16){
                tokenPointer = tokenPointer + i + 1;
                printf("Parse Error line %d offset %d: %s\n",
                       rowNum[tokenPointer], offset[tokenPointer], "SYM_TOO_LONG");
                return false;
            } else{
                ul.usedSym.push_back(token[tokenPointer + i + 1]);
            }
        }
        tokenPointer = tokenPointer + useCount + 1;
    }
    return true;
}

/* Parse program list */
bool parseProgram(ProList& pl){
    int codeCount = 0;
    if (!isNum(token[tokenPointer])) {
        printf("Parse Error line %d offset %d: %s\n",
               rowNum[tokenPointer], offset[tokenPointer], "NUM_EXPECTED");
        return false;
    } else if ((num_instr + turnToInt(token[tokenPointer])) > 512){
        printf("Parse Error line %d offset %d: %s\n",
               rowNum[tokenPointer], offset[tokenPointer], "TOO_MANY_INSTR");
        return false;
    } else{
        num_instr = num_instr + turnToInt(token[tokenPointer]);
        codeCount = turnToInt(token[tokenPointer]);
    }
    if (codeCount < 0){
        printf("Parse Error line %d offset %d: %s\n",
               rowNum[tokenPointer], offset[tokenPointer], "NUM_EXPECTED");
        return false;
    } else if (codeCount == 0) {
        tokenPointer = tokenPointer + 1;
        return true;
    } else {
        int codeLength = tokenPointer + codeCount*2 + 1;
        if (codeLength > token.size() && isSym(token.back())){
            printf("Parse Error line %d offset %d: %s\n",
                   finalPositionX, finalPositionY, "ADDR_EXPECTED");
            return false;
        }
        if (codeLength > token.size() && isIEAR(token.back())){
            printf("Parse Error line %d offset %d: %s\n",
                   finalPositionX, finalPositionY, "NUM_EXPECTED");
            return false;
        }
        if (codeLength > token.size() && isNum(token.back())){
            printf("Parse Error line %d offset %d: %s\n",
                   finalPositionX, finalPositionY, "ADDR_EXPECTED");
            return false;
        }
        pl.codeCount = codeCount;
        for (int i = 0; i < codeCount*2; i++) {
            if (i % 2 == 0 && !isIEAR(token[tokenPointer+i+1])){
                tokenPointer = tokenPointer + i + 1;
                printf("Parse Error line %d offset %d: %s\n",
                       rowNum[tokenPointer], offset[tokenPointer], "ADDR_EXPECTED");
                return false;
            } else if (i % 2 == 0 && isIEAR(token[tokenPointer+i+1])) {
                pl.type.push_back(token[tokenPointer + i + 1]);
            } else if (i % 2 != 0 && !isNum(token[tokenPointer+i+1])){
                tokenPointer = tokenPointer + i + 1;
                printf("Parse Error line %d offset %d: %s\n",
                       rowNum[tokenPointer], offset[tokenPointer], "NUM_EXPECTED");
                return false;
            } else {
                string instruction = token[tokenPointer+i+1];
                pl.instr.push_back(instruction);
            }
        }
        tokenPointer = tokenPointer + codeCount*2 + 1;
    }
    return true;
}

/* Generate symbol Table after pass 1 */
void getSymbolTable(vector<DefList>& fDefList, vector<ProList>& fProList){
    int exist = 0;
    cout << "Symbol Table" <<endl;
    for (int i = 0; i < fDefList.size(); i++) {
        for (int k = 0; k < fDefList[i].definition.size(); k++) {
            string variable = fDefList[i].definition[k];
            int value = fDefList[i].relAddress[k];
            for (int j = 0; j < i; j++) {
                int baseAddress = fProList[j].codeCount;
                value = value + baseAddress;
            }
            if (symDuplicate(variable) && exist == 0){
                cout << variable << "=" << value << " " <<
                "Error: This variable is multiple times defined; first value used" << endl;
                tableSym.push_back(variable);
                tableValue.push_back(value);
                exist++;
            } else if (!symDuplicate(variable)){
                cout << variable << "=" << value << endl;
                tableSym.push_back(variable);
                tableValue.push_back(value);
            }
        }
    }
}

/* Generate memory map */
void getMemoryMap(vector<UList>& fUList, vector<ProList>& fProList){
    cout << "\nMemory Map" <<endl;
    int stepNum = 999;
    for (int i = 0; i < fProList.size(); i++) {
        vector<string> usedSymbols;
        for (int j = 0; j < fProList[i].type.size(); j++) {
            string type = fProList[i].type[j];
            stepNum = stepNum + 1;
            string sn = turnToString(stepNum);
            string snDisplay = sn.substr(1,3);
            string is1 = "0";
            string is = fProList[i].instr[j].substr(1, 3);
            for (int k = 0; k < is.length(); k++) {
                string num = is.substr(k, 1);
                if (num != "0"){
                    is1 = is.substr(k);
                    break;
                }
            }
            int useNum = turnToInt(is1);
            if (type == "I") {
                if (turnToInt(fProList[i].instr[j]) >= 10000){
                    cout << snDisplay << ": 9999 " << "Error: Illegal immediate value; treated as 9999" << endl;
                } else{
                    cout << snDisplay << ": " << addZero(turnAllZero(fProList[i].instr[j])) << endl;
                }
            }else if (type == "A"){
                if (fProList[i].instr[j].length() >= 5){
                    cout << snDisplay << ": 9999 " << "Error: Illegal opcode; treated as 9999" << endl;
                } else{
                    if (useNum > 512){
                        int addressChanged = turnToInt(fProList[i].instr[j].substr(0, 1))*1000;
                        string acToString = addZero(turnAllZero(turnToString(addressChanged)));
                        cout << snDisplay << ": " << acToString <<
                             " Error: Absolute address exceeds machine size; zero used" << endl;
                    } else{
                        cout << snDisplay << ": " << addZero(fProList[i].instr[j]) << endl;
                    }
                }
            } else if (type == "R"){
                if (fProList[i].instr[j].length() >= 5){
                    cout << snDisplay << ": 9999 " << "Error: Illegal opcode; treated as 9999" << endl;
                } else{
                    int base = 0;
                    for (int k = 0; k < i; k++) {
                        base = base + fProList[k].codeCount;
                    }
                    if (useNum > fProList[i].codeCount){
                        int addressChanged = turnToInt(fProList[i].instr[j].substr(0, 1))*1000;
                        int address = addressChanged + base;
                        string adToString = addZero(turnAllZero(turnToString(address)));
                        cout << snDisplay << ": " << adToString <<
                             " Error: Relative address exceeds module size; zero used" << endl;
                    } else{
                        int address = turnToInt(fProList[i].instr[j]) + base;
                        string adToString = addZero(turnAllZero(turnToString(address)));
                        cout << snDisplay << ": " << adToString << endl;
                    }
                }
            } else if (type == "E"){
                int address;
                if (useNum >= fUList[i].usedSym.size()){
                    cout << snDisplay << ": " << addZero(turnAllZero(fProList[i].instr[j])) <<
                    " Error: External address exceeds length of uselist; treated as immediate" << endl;
                } else{
                    string sym = fUList[i].usedSym[useNum];
                    int value = 0;
                    bool symExist = false;
                    for (int k = 0; k < tableSym.size(); k++) {
                        if (tableSym[k] == sym){
                            value = tableValue[k];
                            symExist = true;
                            break;
                        }
                    }
                    address = turnToInt(fProList[i].instr[j].substr(0, 1))*1000 + value;
                    string adToString = addZero(turnAllZero(turnToString(address)));
                    if (symExist){
                        cout << snDisplay << ": " << adToString << endl;
                        usedSymbols.push_back(sym);
                    } else{
                        cout << snDisplay << ": " << adToString << " Error: "
                        << sym <<" is not defined; zero used" << endl;
                        usedSymbols.push_back(sym);
                    }
                }
            }
        }
        for (int m = 0; m < fUList[i].usedSym.size(); m++) {
            bool exist = false;
            for (int n = 0; n < usedSymbols.size(); n++) {
                if (usedSymbols[n] == fUList[i].usedSym[m]){
                    exist = true;
                    break;
                }
            }
            if (!exist){
                cout << "Warning: Module " << i+1 << ": " << fUList[i].usedSym[m]
                << " appeared in the uselist but was not actually used" << endl;
            }
        }
    }
}

/* Check if there is a symbol defined more then once */
bool symDuplicate(string symbol){
    bool erase = false;
    int exist = 0;
    for (int i = 0; i < fileDefList.size(); i++) {
        int position = 0;
        if (fileDefList[i].defCount != 0){
            for (vector<string>::iterator it = fileDefList[i].definition.begin(); it != fileDefList[i].definition.end();) {
                if (symbol == *it && exist == 0){
                    exist++;
                } else if (symbol == *it && exist > 0){
                    erase = true;
                }
                ++it;
                position++;
            }
        }
    }
    return erase;
}

/* Print warning if there is a symbol address too big */
void symTooBig(vector<DefList>& fDefList, vector<ProList>& fProList){
    vector<string> symChecked;
    bool exist = false;
    for (int i = 0; i < fDefList.size(); i++) {
        for (int j = 0; j < fDefList[i].definition.size(); j++) {
            string sym = fDefList[i].definition[j];
            for (int k = 0; k < symChecked.size(); k++) {
                if (symChecked[k] == sym){
                    exist = true;
                    break;
                }
            }
            if (!exist){
                int address = fDefList[i].relAddress[j];
                int codeCount = fProList[i].codeCount;
                if (address >= codeCount){
                    cout << "Warning: Module "<< i+1 <<": " << sym << " too big " << address <<
                         " (max="<< codeCount-1 <<") assume zero relative" << endl;
                    fDefList[i].relAddress[j] = 0;
                }
                symChecked.push_back(sym);
                break;
            }
        }
    }
}

/* Print warning if there is symbol defined but not in use */
void printSymNotInUse(vector<DefList>& fDefList, vector<UList>& fUList){
    vector<string> symDefined;
    vector<int> module;
    vector<string> symInUse;
    for (int i = 0; i < fDefList.size(); i++) {
        for (int j = 0; j < fDefList[i].definition.size(); j++) {
            symDefined.push_back(fDefList[i].definition[j]);
            module.push_back(i);
        }
    }
    for (int k = 0; k < fUList.size(); k++) {
        for (int l = 0; l < fUList[k].usedSym.size(); l++) {
            symInUse.push_back(fUList[k].usedSym[l]);
        }
    }
    for (int i = 0; i < symDefined.size(); i++) {
        bool symUsed = false;
        for (int j = 0; j < symInUse.size(); j++) {
            if (symDefined[i] == symInUse[j]){
                symUsed = true;
                break;
            }
        }
        if (!symUsed){
            cout << "Warning: Module "<< module[i]+1 <<": "
            << symDefined[i] <<" was defined but never used" << endl;
        }
    }
    printf("\n");
}

bool isNum(string tokenItem){
    for (int i = 0; i < tokenItem.length(); i++) {
        char tI = tokenItem[i];
        if (!isdigit(tI)){
            return false;
        }
    }
    return true;
}

bool isSym(string tokenItem){
    char tI = tokenItem[0];
    if (isalpha(tI)){
        return true;
    }
    return false;
}

bool isIEAR(string tokenItem){
    if (tokenItem.length() != 1){
        return false;
    } else if (tokenItem != "I" && tokenItem != "E" && tokenItem != "A" && tokenItem != "R"){
        return false;
    } else {
        return true;
    }
}

int turnToInt(string num){
    int x;
    stringstream ss;
    ss << num;
    ss >> x;
    ss.clear();
    return x;
}

string turnToString(int num){
    stringstream st;
    st<<num;
    string str= st.str();
    return str;
}

string trim(string& str){
    string strTrim;
    strTrim = str.erase(str.find_last_not_of(" \t") + 1);
    return strTrim;
}

string turnAllZero(string instr){
    bool allZero = true;
    for (int j = 0; j < instr.length(); j++) {
        if (instr.substr(j, 1) != "0"){
            allZero = false;
            break;
        }
    }
    if (allZero){
        return "0000";
    } else{
        return instr;
    }
}

string addZero(string instr){
    string instrAddZero = instr;
    if (instr.length()!=4){
        for (int i = 0; i < 4-instr.length(); i++) {
            instrAddZero = "0" + instrAddZero;
        }
        return instrAddZero;
    } else{
        return instr;
    }
}




