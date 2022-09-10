#pragma once
#ifndef GENERATOR_H
#define GENERATOR_H

#include "define.h"
#include "optimizer.h"

#define STACK string("stack")
#define DATA string("data")
#define TEMP string("text")

using namespace std;

class Generator{
public:
    Generator();
    ~Generator();

    int stack_buf_size = 4 * 1024 * 32;
    int data_buf_size = 4 * 1024 * 32;
    int temp_buf_size = 4 * 1024 * 32;

    vector<string> objCode;
    vector<MsgTableItem> msgTabHis;
    vector<AysTableItem> aysHis;

    void init(vector<Quaternion> ic, vector<Block> bg, int stack_size);
    void genObjCode();

private:
    vector<Quaternion> interCode;
    vector<Block> blkGroup;
    map<string, vector<string>> AVALUE;
    vector<string> reCode;

    map<string, vector<pair<string, int>>> RVALUE = {
        {"$t0",vector<pair<string,int>>{}},
        {"$t1",vector<pair<string,int>>{}},
        {"$t2",vector<pair<string,int>>{}},
        {"$t3",vector<pair<string,int>>{}},
        {"$t4",vector<pair<string,int>>{}},
        {"$t5",vector<pair<string,int>>{}},
        {"$t6",vector<pair<string,int>>{}},
        {"$t7",vector<pair<string,int>>{}}
    };

    void endBlock();
    bool isNum(string str);
    void EMIT(string code);
    string getREG(string result);
    void freshRA(pair<int, bool> tag, string R, string V, bool value_changed);
    vector<MsgTableItem> genMsgTable(int block_no);
};

#endif // GENERATOR_H
