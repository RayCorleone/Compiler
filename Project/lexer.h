#pragma once
#ifndef LEXER_H
#define LEXER_H

#include "define.h"

using namespace std;

class Lexer {
public:
    Lexer();
    ~Lexer();

    int pos;        //当前位置
    int line;       //当前行数
    int idCnt;      //符号计数
    State state;    //分析状态
    string strIn;	//分析字符串
    map<int, string> nameTable; //符号表
    vector<pair<string,pair<string,int>>> wordTable;    //分析表

    // 获取函数
    void lexerOpen(string fstring);
    // 词法分析
    pair<string, int> lexerAnalyse();

private:
    // 4个分析串表
    vector<string> spaceList = { " ","\t","\r" };
    vector<string> op1List = { "+","-","&","|","^","*","/","<",">","=",";",",","(",")","[","]","{","}", "!" };
    vector<string> op2List = { "<=", "!=", "==", ">=" ,"&&","||" };
    vector<string> keyList = { "int", "void", "while", "if", "else", "return" };

    // 各个特征词表
    map<string, pair<string, int>> tokenList = {
        pair<string,pair<string,int>>("int",    pair<string,int>("INT",-1)),
        pair<string,pair<string,int>>("void",   pair<string,int>("VOID",-1)),
        pair<string,pair<string,int>>("id",     pair<string,int>("ID",-1)),
        pair<string,pair<string,int>>("while",  pair<string,int>("WHILE",-1)),
        pair<string,pair<string,int>>("if",     pair<string,int>("IF",-1)),
        pair<string,pair<string,int>>("else",   pair<string,int>("ELSE",-1)),
        pair<string,pair<string,int>>("return", pair<string,int>("RET",-1)),

        pair<string,pair<string,int>>("=",      pair<string,int>("ASSIGN",-1)),
        pair<string,pair<string,int>>("+",      pair<string,int>("OP1",0)),
        pair<string,pair<string,int>>("-",      pair<string,int>("OP1",1)),
        pair<string,pair<string,int>>("&",      pair<string,int>("OP1",2)),
        pair<string,pair<string,int>>("|",      pair<string,int>("OP1",3)),
        pair<string,pair<string,int>>("^",      pair<string,int>("OP1",4)),
        pair<string,pair<string,int>>("*",      pair<string,int>("OP2",0)),
        pair<string,pair<string,int>>("/",      pair<string,int>("OP2",1)),

        pair<string,pair<string,int>>("<",      pair<string,int>("CMP",0)),
        pair<string,pair<string,int>>("<=",     pair<string,int>("CMP",1)),
        pair<string,pair<string,int>>(">",      pair<string,int>("CMP",2)),
        pair<string,pair<string,int>>(">=",     pair<string,int>("CMP",3)),
        pair<string,pair<string,int>>("==",     pair<string,int>("CMP",4)),
        pair<string,pair<string,int>>("!=",     pair<string,int>("CMP",5)),
        pair<string,pair<string,int>>("||",     pair<string,int>("OR",-1)),
        pair<string,pair<string,int>>("&&",     pair<string,int>("AND",-1)),
        pair<string,pair<string,int>>("!",      pair<string,int>("NOT",-1)),
        pair<string,pair<string,int>>("\n",     pair<string,int>("NL",-1)),

        pair<string,pair<string,int>>(";",      pair<string,int>(";",-1)),
        pair<string,pair<string,int>>(",",      pair<string,int>(",",-1)),
        pair<string,pair<string,int>>("(",      pair<string,int>("(",-1)),
        pair<string,pair<string,int>>(")",      pair<string,int>(")",-1)),
        pair<string,pair<string,int>>("[",      pair<string,int>("[",-1)),
        pair<string,pair<string,int>>("]",      pair<string,int>("]",-1)),
        pair<string,pair<string,int>>("{",      pair<string,int>("{",-1)),
        pair<string,pair<string,int>>("}",      pair<string,int>("}",-1))
    };
};
#endif // !LEXER_H
