#pragma once
#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "define.h"
#include "parser.h"
#include <sstream>

using namespace std;

class Optimizer {
public:
    Optimizer();
    ~Optimizer();

    vector<Quaternion> interCode;
    vector<Block> blockGroup;

    void init(map<int, string> nt, SymbolTable* gt, vector<Quaternion> ic);
    double optimizerAnalyse();

private:
    int tempCnt = 0;
    vector<Quaternion> oriCode;
    vector<Block> oriBlock;

    map<int, string> nameTable;
    map<int, string> labelTable;
    SymbolTable* glbTable;
    vector<vector<DAGItem>> DAGGroup;

    string newtemp();
    void partition();
    bool preOptimize();
    vector<DAGItem> genDAG(int block_no);
    void _utilizeChildren(vector<DAGItem>& DAG, int now);
    bool isNum(string str);
    void optimize();
};

#endif // OPTIMIZER_H
