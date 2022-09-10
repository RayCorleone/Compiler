#pragma once
#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "grammar.h"
#include "semantic.h"

class Parser {
public:
    Parser();
    ~Parser();

    Lexer L;    //词法分析器
    Grammar G;  //语法文件解读
    Semantic S; //语义分析器

    int line;   //行数
    vector<string> strStack;    //输入串
    vector<string> strState;    //状态栈
    vector<string> strSymbol;   //符号栈

    vector<vector<vector<string>>> stackTable;  //分析表

    // LR1初始化
    void initLR1();
    // 读取字符
    void parserOpen(string input);
    // 语法分析
    void parserAnalyse();
};

#endif // !PARSER_H
