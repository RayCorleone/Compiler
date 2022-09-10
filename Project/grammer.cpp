#include "grammar.h"

Grammar::Grammar(){
    C.clear();
    P.clear();
    VN.clear();
    VT.clear();
    str.clear();
    GOTO.clear();
    FIRST.clear();
    ACTION.clear();
}

Grammar::~Grammar(){
    C.clear();
    P.clear();
    VN.clear();
    VT.clear();
    str.clear();
    GOTO.clear();
    FIRST.clear();
    ACTION.clear();
}

void Grammar::initGrammar(){
    /// 打开文件
    ifstream f(GRAMMARFILE, ios::in);
    if (!f.is_open())
        throw string("ERROR: Can Not Open Grammar File.\n");

    /// 变量初始化
    string strLine;             //当前行
    vector<string> branch;      //一个推进分支
    bool flag = false;          //表达式左右区分
    string tempV;               //临时记录符号
    pair<string, vector<vector<string>>> tempP; //临时记录表达式

    /// 循环读取Grammer
    while (!f.eof()){
        //一次读取一行信息
        getline(f, strLine);
        flag = false;
        tempP.first = string("");
        tempP.second.clear();
        branch.clear();

        //对一行的表达式分析
        for (int i = 0; i < strLine.size(); i++){
            switch (strLine[i]){
            // 读取非终结符
            case '<':
                tempV.clear();
                i++;
                while (strLine[i] != '>') {
                    tempV += strLine[i];
                    i++;
                }
                VN.push_back(tempV);
                if (!flag)
                    tempP.first = tempV;
                else
                    branch.push_back(tempV);
                break;

            // 读取终结符
            case '\'':
                tempV.clear();
                i++;
                while (strLine[i] != '\'') {
                    tempV += strLine[i];
                    i++;
                }
                VT.push_back(tempV);
                branch.push_back(tempV);
                break;

            // 区分表达式左右
            case ':':
                while (strLine[i] != '=') {
                    tempV += strLine[i];
                    i++;
                }
                flag = true;
                break;

            // 读取右侧分支并保存
            case '|':
                tempP.second.push_back(branch);
                branch.clear();
                break;

            default:break;
            }
        }

        //保存一行的读取结果
        tempP.second.push_back(branch);
        branch.clear();
        P.insert(tempP);

        //清空变量
        tempP.first = string("");
        tempP.second.clear();
    }

    /// VT与VN去重
    str = VN[0];
    sort(VN.begin(), VN.end());
    vector<string>::iterator pos = unique(VN.begin(), VN.end());
    VN.erase(pos, VN.end());
    sort(VT.begin(), VT.end());
    pos = unique(VT.begin(), VT.end());
    VT.erase(pos, VT.end());
    VT.push_back(string("#"));
    f.close();

    /// 判断文法是否正确
    if (VN.size() != P.size())
        throw string("GRAMMAR ERROR: the number of productions is not equal with the number of VN.\n");
}

void Grammar::initFirst(){
    ///FIRST集初始化
    for (int i = 0; i < VT.size(); i++)
        FIRST[VT[i]] = { VT[i] };

    bool flag = true;
    while (flag){
        flag = false;
        ///遍历所有分支
        map<string, vector<vector<string>>>::iterator p;
        for (p = P.begin(); p != P.end(); p++){
            string X = p->first;
            if (FIRST.find(X) == FIRST.end())
                FIRST[X] = {};

            for (int j = 0; j < p->second.size(); j++){
                if (p->second[j].size() > 0){
                    string Y = p->second[j][0];
                    for (int r = 0; r < FIRST[Y].size(); r++){
                        if (count(FIRST[X].begin(), FIRST[X].end(), FIRST[Y][r]) == 0){
                            FIRST[X].push_back(FIRST[Y][r]);
                            flag = true;
                        }
                    }
                }
                else
                    throw string("GRAMMAR ERROR: null production on the right side.\n");
            }
        }
    }
}

void Grammar::initClosure(Closure& c){
    for (int i = 0; i < c.cSet.size(); i++){
        if (c.cSet[i].dot >= c.cSet[i].p.second.size())
            continue;

        string dotV = c.cSet[i].p.second[c.cSet[i].dot];
        ///如果dotV是VN
        if (count(VN.begin(), VN.end(), dotV) != 0){
            //检查dotV的每一条产生式
            for (int j = 0; j < P[dotV].size(); j++){
                Canonical tempC;
                tempC.p = pair<string, vector<string>>(dotV, P[dotV][j]);
                tempC.dot = 0;

                //遍历dotV后的第一个符号
                int k = c.cSet[i].dot + 1;
                //如果遍历完成，将set[i]的expect加入到新的expect
                if (k == c.cSet[i].p.second.size())
                    tempC.forecast.insert(tempC.forecast.end(), c.cSet[i].forecast.begin(), c.cSet[i].forecast.end());
                //否则加入FIRST[beta]
                else{
                    string v = c.cSet[i].p.second[k];
                    for (int r = 0; r < FIRST[v].size(); r++)
                        tempC.forecast.push_back(FIRST[v][r]);
                }

                //去重
                bool repeat = false;
                for (int t = 0; t < c.cSet.size(); t++){
                    if (c.cSet[t].p == tempC.p && c.cSet[t].dot == tempC.dot){
                        repeat = true;
                        c.cSet[t].forecast.insert(c.cSet[t].forecast.end(), tempC.forecast.begin(), tempC.forecast.end());
                        vector<string>::iterator pos = unique(c.cSet[t].forecast.begin(), c.cSet[t].forecast.end());
                        c.cSet[t].forecast.erase(pos, c.cSet[t].forecast.end());
                        sort(c.cSet[t].forecast.begin(), c.cSet[t].forecast.end());
                        break;
                    }
                }

                if (!repeat){
                    sort(tempC.forecast.begin(), tempC.forecast.end());
                    c.cSet.push_back(tempC);
                }
            }
        }
    }
}

Closure Grammar::getNext(Closure& c, string A){
    Closure ret = {{},{}};
    for (int i = 0; i < c.cSet.size(); i++){
        if (c.cSet[i].dot < c.cSet[i].p.second.size() && c.cSet[i].p.second[c.cSet[i].dot] == A){
            Canonical newcan = c.cSet[i];
            newcan.dot++;
            ret.cSet.push_back(newcan);
        }
    }
    return ret;
}

vector<string> Grammar::getX(Closure& c){
    vector<string> ret;
    for (int i = 0; i < c.cSet.size(); i++){
        if (c.cSet[i].dot < c.cSet[i].p.second.size())
            ret.push_back(c.cSet[i].p.second[c.cSet[i].dot]);
    }
    vector<string>::iterator pos = unique(ret.begin(), ret.end());
    ret.erase(pos, ret.end());
    return ret;
}

bool Grammar::isEqual(vector<Canonical>& a, vector<Canonical>& b){
    if (a.size() != b.size())
        return false;
    else{
        for (int i = 0; i < a.size(); i++){
            bool flag = false;
            for (int j = 0; j < b.size(); j++){
                if (a[i] == b[j]){
                    flag = true;
                    break;
                }
            }
            if (!flag)
                return false;
        }
        return true;
    }
}

void Grammar::initCollection(){
    int count = 0;
    stack<int> wait;
    Closure I = {{{pair<string, vector<string>>(string("S'"),{str}),0,{string("#")}}},{}};
    initClosure(I);

    C[count] = I;
    wait.push(count);
    count++;

    while (!wait.empty()){
        int now = wait.top();
        wait.pop();

        vector<string>XList = getX(C[now]);
        for (int i = 0; i < XList.size(); i++){
            Closure newclo = getNext(C[now], XList[i]);
            initClosure(newclo);

            int exist = -1;
            map< int, Closure >::iterator p;
            for (p = C.begin(); p != C.end(); p++){
                if (isEqual(p->second.cSet, newclo.cSet)){
                    exist = p->first;
                    break;
                }
            }

            if (exist == -1){
                C[now].cNext[XList[i]] = count;
                C[count] = newclo;
                wait.push(count);
                count++;
            }

            else
                C[now].cNext[XList[i]] = exist;
        }
    }
}

void Grammar::initLR1Table(){
    map<int, Closure>::iterator p;
    for (p = C.begin(); p != C.end(); p++){
        vector<Action> newAct(VT.size());
        vector<int>newGo(VN.size(), -1);

        vector<Canonical> set = p->second.cSet;
        map<string, int> next = p->second.cNext;

        map<string, int>::iterator q;
        for (q = next.begin(); q != next.end(); q++){
            vector<string>::iterator pos;
            if ((pos = find(VN.begin(), VN.end(), q->first)) != VN.end())
                newGo[pos - VN.begin()] = q->second;
            else if ((pos = find(VT.begin(), VT.end(), q->first)) != VT.end()){
                newAct[pos - VT.begin()].s = ACT_NEXT;
                newAct[pos - VT.begin()].nextState = q->second;
            }
            else
                throw string("GRAMMER ERROR: unable to get ACTION/GOTO table.\n");
        }

        for (int i = 0; i < set.size(); i++){
            if (set[i].dot == set[i].p.second.size()){
                if (set[i].p.first == string("S'"))
                    (newAct.end() - 1)->s = ACT_DONE;
                else{
                    Canonical tmpcan = set[i];
                    for (int j = 0; j < tmpcan.forecast.size(); j++){
                        vector<string>::iterator pos = find(VT.begin(), VT.end(), tmpcan.forecast[j]);
                        newAct[pos - VT.begin()].s = ACT_REDUCE;
                        newAct[pos - VT.begin()].p = tmpcan.p;
                    }
                }
            }
        }

        ACTION[p->first] = newAct;
        GOTO[p->first] = newGo;
    }
}
