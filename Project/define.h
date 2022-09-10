#pragma once
#ifndef DEFINE_H
#define DEFINE_H

#include <map>
#include <string>
#include <vector>
#include <algorithm>

#define EPSILON     "none"
#define GRAMMARFILE "grammar.txt"

using namespace std;

////////////////////////////////////////////////
/// 词法分析结构
// Lexer状态集
enum State {
    ERROR,      //错误
    START,      //起始
    NUMBER,     //数字
    OPSIGN,     //操作符
    STRING,     //字符串
    COMLINE,    //单行注释
    COMBLOCK    //多行注释
};

////////////////////////////////////////////////
/// 语法分析结构
// Parser动作集
enum Act{
    ACT_ERROR,
    ACT_NEXT,
    ACT_REDUCE,
    ACT_DONE
};

// Parser状态结构体
struct Action {
    Act s = ACT_ERROR;  // 动作类型
    int nextState = -1; // 下一状态
    pair<string, vector<string>> p;
};

// 项集族
struct Canonical {
    pair<string, vector<string>> p;
    int dot;
    vector<string> forecast;

    bool operator == (Canonical& c){
        if (p == c.p && dot == c.dot && forecast == c.forecast)
            return true;
        else
            return false;
    }
};

// 项目集闭包
struct Closure {
    vector<Canonical> cSet;
    map<string, int> cNext;
};

////////////////////////////////////////////////
/// 语义分析结构
// 语义变量
enum Type {
    INT,    // 整型
    VOID    // 空型
};

// 数据类型
enum Kind {
    VAR,    // 变量
    FUNC,   // 函数
    ARRAY   // 数组
};

// 四元式
struct Quaternion {
    string op;
    string arg1;
    string arg2;
    string result;
};

// 符号表预声明
struct SymbolTable;

// 符号项
struct Symbol {
    int id; // 符号表入口
    Type t; // 语义变量
    Kind k; // 数据类型
    int offset;     // 相对地址
    vector<int> dm; // 维度(数组)
    SymbolTable* pTable = NULL;  // 子符号表指针
};

// 符号表
struct SymbolTable {
    int width = 0;              // 地址宽度
    vector<Symbol> table;       // 符号表
    SymbolTable* parent = NULL; // 父符号表

    // 清空符号表
    void clear(){
        width = 0;
        table.clear();
    }
    // 向符号表插入符号
    void insert(int id, Type t, Kind k, int offset){
        Symbol temp;
        temp.id = id;
        temp.t=t;
        temp.k = k;
        temp.offset = offset;

        for (int i = 0; i < table.size(); i++){
            if (table[i].id == id)
                throw string("ERROR: ID-") + to_string(id) + string(" already exists.\n");
        }

        table.push_back(temp);
    }
    // 插入维度信息
    void insertDM(int id, vector<int>dm){
        for (int i = 0; i < table.size(); i++){
            if (table[i].id == id && ((table[i].k == ARRAY) || (table[i].k == FUNC))){
                table[i].dm = dm;
                break;
            }
        }
    }
    // 插入函数的子符号表
    void insertFunc(int id, SymbolTable* funcTable){
        for (int i = 0; i < table.size(); i++){
            if (table[i].id == id && table[i].k == FUNC)
                table[i].pTable = funcTable;
        }
    }
};

// 分析树
struct TreeNode {
    pair<string, int> data;
    TreeNode* parent = NULL;
    vector<TreeNode*> children;

    int pos;
    int TList;
    int FList;

    int n = 0;      // 值的大小
    Type t = INT;   // 语义变量
    Kind k = VAR;   // 数据类型
    string place;   // 地址入口
    int width = 0;  // 地址宽度
    vector<int> dm; // 维度信息
    vector<string> params;  // 参数表

    TreeNode() {
        parent = NULL;
        children.clear();
        data = pair<string, int>("", -1);
    }
};

////////////////////////////////////////////////
/// 代码优化结构
// 代码块
struct Block {
    int begin;
    int end;
    vector<string> waitVar;
    vector<string> activeVar;
    vector<string> uselessVar;
};

// DAG节点
struct DAGItem {
    bool useful = false;
    bool isLeaf;
    string value;
    string op;
    vector<string> label;
    Quaternion code;

    int parent = -1;
    int lChild = -1;
    int rChild = -1;
    int tChild = -1;
    bool isRemain = false;

    bool operator== (DAGItem b)    {
        bool f1 = this->isLeaf == b.isLeaf;
        bool f2 = this->value == b.value;
        bool f3 = this->op == b.op;
        bool f4 = this->label.size() == b.label.size();
        bool f5 = this->parent == b.parent;
        bool f6 = this->lChild == b.lChild;
        bool f7 = this->rChild == b.rChild;
        bool f8 = true;

        for (auto i = 0; i < this->label.size() && i < b.label.size(); i++){
            if (this->label[i] != b.label[i]){
                f8 = false;
                break;
            }
        }

        return f1 & f2 & f3 & f4 & f5 & f6 & f7 & f8;
    }
};

////////////////////////////////////////////////
/// 代码生成结构
// 待用活跃信息
struct MsgTableItem {
    int no;

    Quaternion TAS;
    pair<int, bool> arg1;
    pair<int, bool> arg2;
    pair<int, bool> result;
};

// 寄存器分配
struct AysTableItem {
    Quaternion TAS;
    vector<string> objCode;
    map<string, vector<pair<string, int>>> RVALUE;
    map<string, vector<string>> AVALUE;
};

#endif // DEFINE_H
