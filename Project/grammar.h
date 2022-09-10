#pragma once
#ifndef GRAMMAR_H
#define GRAMMAR_H

#include "define.h"
#include <stack>
#include <fstream>

class Grammar {
public:
    Grammar();
    ~Grammar();

    string str;         //非终结符串
    vector<string> VN;  //非终结符
    vector<string> VT;  //终结符
    map<int, Closure> C;                //项集族
    map<int, vector<int>> GOTO;         //GOTO函数
    map<int, vector<Action>> ACTION;    //ACTION函数
    map<string, vector<string>> FIRST;  //First集
    map<string, vector<vector<string>>> P;  //表达式

    vector<string> getX(Closure& c);
    Closure getNext(Closure& c, string A);
    bool isEqual(vector<Canonical>& a, vector<Canonical>& b);

    void initGrammar();
    void initFirst();
    void initClosure(Closure& c);
    void initCollection();
    void initLR1Table();
};

#endif // GRAMMAR_H
