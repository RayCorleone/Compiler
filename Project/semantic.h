#pragma once
#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "define.h"

class Semantic{
public:
    Semantic();
    ~Semantic();

    vector<Quaternion> midList;     // 四元式表(中间代码表)
    SymbolTable* glbTable;          // 全局符号表
    SymbolTable* currentSymTable;   // 当前符号表

    void semanticAnalyse(string token, TreeNode* root, map<int, string> nameTable);

private:
    int tempVarCnt;                     // 临时变量计数
    vector<int> offsetStack;            // 地址表栈
    vector<SymbolTable*> symTableStack; // 符号表栈

    string newtemp();       // 创建新临时变量
    Symbol* find(int id);   // 查找过程调用
    string entry(int id);   // 查找符号表入口
    void emit(string op, string arg1, string arg2, string result);
};

#endif // !SEMANTIC_H
