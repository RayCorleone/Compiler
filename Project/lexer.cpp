#include "lexer.h"

Lexer::Lexer(){
    pos = 0;
    line = 1;
    idCnt = 0;
    state = START;
    nameTable.clear();
    wordTable.clear();
}

Lexer::~Lexer(){
    pos = 0;
    line = 1;
    idCnt = 0;
    strIn = "";
    state = START;
    nameTable.clear();
    wordTable.clear();
}

void Lexer::lexerOpen(string fstring){
    pos = 0;
    line = 1;
    idCnt = 0;
    state = START;
    nameTable.clear();
    wordTable.clear();
    strIn = fstring;
}

pair<string, int> Lexer::lexerAnalyse(){
    /// 创建临时变量
    string temp;
    pair<string,pair<string,int>> save;

    /// 循环读取
    while (pos < strIn.size()){
        // 循环临时变量
        string begin;   //所读字符串首字符
        string nextChar;//下一个字符
        string opTemp;  //双运算符

        begin = strIn[pos];
        if(pos + 1 < strIn.size())
            nextChar = strIn[pos + 1];
        else
            nextChar = '\0';
        opTemp = begin + nextChar;

        // 循环状态机
        switch (state){

        //->新字符串状态
        case START:
            //-空格
            if (find(spaceList.begin(), spaceList.end(), begin) != spaceList.end()){
				pos++;
                state = START;
			}

            //-换行符
            else if (begin == "\n"){
				pos++;
                line++;
                state = START;
                save.first = "<\\n>";
                save.second = tokenList.find(begin)->second;
                wordTable.push_back(save);
                return tokenList.find(begin)->second;
			}

            //-注释
            else if (begin == "/"){
                if (nextChar == "/"){
					pos += 2;
                    state = COMLINE;
				}
                else if (nextChar == "*"){
					pos += 2;
                    state = COMBLOCK;
				}
                else{
                    state = OPSIGN;
				}
			}

            //-数字
            else if (begin[0] >= '0' && begin[0] <= '9'){
                temp.erase();
                temp += begin;
				pos++;
                state = NUMBER;
			}

            //-操作符
            else if (find(op1List.begin(), op1List.end(), begin) != op1List.end())
                state = OPSIGN;

            //-ID或KeyWord
            else if ((begin[0] >= 'a' && begin[0] <= 'z')||(begin[0] >= 'A' && begin[0] <= 'Z')||(begin[0] == '_')){
                temp.erase();
                temp += begin;
				pos++;
                state = STRING;
			}

            //-错误
            else
                state = ERROR;
            break;

        //->单行注释状态
        case COMLINE:
            if (begin != "\n"){
                pos++;
                state = COMLINE;
			}
            else{   //遇到换行结束注释
				pos++;
                state = START;
                line++;
                save.first = "<\\n>";
                save.second = tokenList.find(begin)->second;
                wordTable.push_back(save);
                return tokenList.find(begin)->second;
			}
			break;


        //->多行注释状态
        case COMBLOCK:
            if (begin == "*" && nextChar == "/"){   //遇到*\结束注释
				pos += 2;
                state = START;
			}
            else if (begin == "\n"){
				pos++;
                state = COMBLOCK;
                line++;
                save.first = "<\\n>";
                save.second = tokenList.find(begin)->second;
                wordTable.push_back(save);
                return tokenList.find(begin)->second;
			}
            else{
				pos++;
                state = COMBLOCK;
			}
			break;

        //->数字状态
        case NUMBER:
            if (begin[0] >= '0' && begin[0] <= '9'){
                temp += begin;
				pos++;
                state = NUMBER;
			}
            else{
                state = START;
                save.first = temp;
                save.second = pair<string, int>("NUM", atoi(temp.c_str()));
                wordTable.push_back(save);
                return pair<string, int>("NUM", atoi(temp.c_str()));
			}
			break;

        //->操作符状态
        case OPSIGN:
            if (find(op2List.begin(), op2List.end(), opTemp) != op2List.end()){
				pos += 2;
                state = START;
                save.first = opTemp;
                save.second = tokenList.find(opTemp)->second;
                wordTable.push_back(save);
                return tokenList.find(opTemp)->second;
			}
            else{
				pos++;
                state = START;
                save.first = begin;
                save.second = tokenList.find(begin)->second;
                wordTable.push_back(save);
                return tokenList.find(begin)->second;
			}
			break;

        //->ID或Keyword
        case STRING:
            if ((begin[0] >= 'a' && begin[0] <= 'z')||(begin[0] >= 'A' && begin[0] <= 'Z')||(begin[0] == '_')||(begin[0] >= '0'&&begin[0] <= '9')){
                temp += begin;
				pos++;
                state = STRING;
			}
            else{
                //-Keyword
                if (find(keyList.begin(), keyList.end(), temp) != keyList.end()){
                    state = START;
                    save.first = temp;
                    save.second = tokenList.find(temp)->second;
                    wordTable.push_back(save);
                    return tokenList.find(temp)->second;
				}

                //-ID
                else{
                    state = START;
                    int id = 0;
                    bool exist = false;
                    for (map<int, string>::iterator iter = nameTable.begin(); iter != nameTable.end(); iter++){
                        if (iter->second == temp){
                            exist = true;
                            id = iter->first;
							break;
						}
					}
                    if (!exist){
                        nameTable.insert(pair<int, string>(idCnt, temp));
                        save.first = temp;
                        save.second = pair<string, int>("ID", idCnt);
                        wordTable.push_back(save);
                        return pair<string, int>("ID", idCnt++);
					}
                    else{
                        save.first = temp;
                        save.second = pair<string, int>("ID", id);
                        wordTable.push_back(save);
                        return pair<string, int>("ID", id);
					}
				}
			}
			break;

        //->错误状态
		case ERROR:
            state = START;
            string msg = string("ERROR: An error has occured in lexer. <Line: ") + to_string(line) + string("; String：") + begin;
            throw msg;
            save.first = "error";
            save.second = pair<string, int>("ERROR", -1);
            wordTable.push_back(save);
            return pair<string, int>("ERROR", -1);
			break;
		}
	}

    save.first = "<END>";
    save.second = pair<string, int>("#", -1);
    wordTable.push_back(save);

    return pair<string, int>("#", -1);
}
