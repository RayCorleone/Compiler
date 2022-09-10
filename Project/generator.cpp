#include "generator.h"

Generator::Generator(){
}

Generator::~Generator(){
}

void Generator::init(vector<Quaternion> ic, vector<Block> bg, int stack_size){
    interCode = ic;
    blkGroup = bg;
    stack_buf_size = stack_size * 1024;
    data_buf_size = stack_size * 1024;
    temp_buf_size = stack_size * 1024;
    return;
}

bool Generator::isNum(string str){
    stringstream sin(str);
    int d;
    char c;
    if (!(sin >> d))
        return false;
    if (sin >> c)
        return false;
    return true;
}

vector<MsgTableItem> Generator::genMsgTable(int block_no){
    vector<MsgTableItem> msg_table;
    map<string, pair<int, bool>> msg_link;

    for (auto pos = blkGroup[block_no].end; pos >= blkGroup[block_no].begin; pos--)
    {
        Quaternion TAS = interCode[pos];
        MsgTableItem new_table_item;
        new_table_item.no = pos;
        new_table_item.TAS = TAS;
        if (TAS.arg1[0] == 'G' || TAS.arg1[0] == 'V' || TAS.arg1[0] == 'T')
        {
            if (msg_link.find(TAS.arg1) == msg_link.end())
            {
                if (TAS.arg1[0] == 'G' || find(blkGroup[block_no].waitVar.begin(), blkGroup[block_no].waitVar.end(), TAS.arg1) != blkGroup[block_no].waitVar.end())
                    msg_link[TAS.arg1] = pair<int, bool>(INT_MAX, true);
                else
                    msg_link[TAS.arg1] = pair<int, bool>(0, false);
            }
            new_table_item.arg1 = msg_link[TAS.arg1];
            msg_link[TAS.arg1] = pair<int, bool>(pos, true);
        }
        if (TAS.arg2[0] == 'G' || TAS.arg2[0] == 'V' || TAS.arg2[0] == 'T')
        {
            if (msg_link.find(TAS.arg2) == msg_link.end())
            {
                if (TAS.arg2[0] == 'G' || find(blkGroup[block_no].waitVar.begin(), blkGroup[block_no].waitVar.end(), TAS.arg2) != blkGroup[block_no].waitVar.end())
                    msg_link[TAS.arg2] = pair<int, bool>(INT_MAX, true);
                else
                    msg_link[TAS.arg2] = pair<int, bool>(0, false);
            }
            new_table_item.arg2 = msg_link[TAS.arg2];
            msg_link[TAS.arg2] = pair<int, bool>(pos, true);
        }
        if (TAS.result[0] == 'G' || TAS.result[0] == 'V' || TAS.result[0] == 'T')
        {
            if (msg_link.find(TAS.result) == msg_link.end())
            {
                if (TAS.result[0] == 'G' || find(blkGroup[block_no].waitVar.begin(), blkGroup[block_no].waitVar.end(), TAS.result) != blkGroup[block_no].waitVar.end())
                    msg_link[TAS.result] = pair<int, bool>(INT_MAX, true);
                else
                    msg_link[TAS.result] = pair<int, bool>(0, false);
            }
            new_table_item.result = msg_link[TAS.result];
            msg_link[TAS.result] = pair<int, bool>(0, false);
        }
        msg_table.push_back(new_table_item);
        msgTabHis.push_back(new_table_item);
    }
    reverse(msg_table.begin(), msg_table.end());

    return msg_table;
}

void Generator::EMIT(string code){
    objCode.push_back(code);
    reCode.push_back(code);
}

string Generator::getREG(string result){
    string R;
    bool has_R = false;
    //result本身有寄存器
    if (AVALUE.find(result) != AVALUE.end() && AVALUE[result].size() > 0)
    {
        for (auto i = 0; i < AVALUE[result].size(); i++)
        {
            if (AVALUE[result][i] != "M"){
                R = AVALUE[result][i];
                has_R = true;
                break;
            }
        }
    }

    if (!has_R)
    {
        for (map<string, vector<pair<string, int>>>::iterator iter = RVALUE.begin(); iter != RVALUE.end(); iter++)
        {
            if (iter->second.size() == 0){
                R = iter->first;
                return R;
            }
        }

        //choose R which will be used in longest time
        int farthest_R = -1;
        for (map<string, vector<pair<string, int>>>::iterator iter = RVALUE.begin(); iter != RVALUE.end(); iter++)
        {
            int closest_V = INT_MAX;
            for (auto i = 0; i < iter->second.size(); i++)
            {
                if (iter->second[i].second < closest_V)
                    closest_V = iter->second[i].second;
            }
            if (closest_V > farthest_R)
            {
                farthest_R = closest_V;
                R = iter->first;
            }
        }
    }

    for (auto i = 0; i < RVALUE[R].size(); i++)
    {
        string V = RVALUE[R][i].first;
        if (AVALUE[V].size() == 1 && AVALUE[V][0] == R)
        {
            //save variable V
            if (V[0] == 'G')
                EMIT("sw " + R + "," + DATA + "+" + to_string(stoi(V.substr(1))));
            else if (V[0] == 'V')
                EMIT("sw " + R + "," + STACK + "+" + to_string(4 + stoi(V.substr(1))) + "($fp)");
            else if (V[0] == 'T')
                EMIT("sw " + R + "," + TEMP + "+" + to_string(4 * stoi(V.substr(1))));
            else{
                throw string("ERROR: AVALUE has unexpected value:") + V + string("\n");
            }
        }

        //delete R from AVALUE
        vector<string>::iterator Ritor = find(AVALUE[V].begin(), AVALUE[V].end(), R);
        AVALUE[V].erase(Ritor);
        //add memroy address to AVALUE
        if (find(AVALUE[V].begin(), AVALUE[V].end(), "M") == AVALUE[V].end())
            AVALUE[V].push_back("M");
    }

    //delete all V from RVALUE
    RVALUE[R].clear();
    return R;
}

void Generator::freshRA(pair<int, bool> tag, string R, string V, bool value_changed){
    if (value_changed || !tag.second)
    {
        for (auto i = 0; i < AVALUE[V].size(); i++)
        {
            if (RVALUE.find(AVALUE[V][i]) != RVALUE.end())
            {
                string opR = AVALUE[V][i];
                for (auto j = 0; j < RVALUE[opR].size(); j++)
                {
                    if (RVALUE[opR][j].first == V)
                    {
                        vector<pair<string, int>>::iterator iter = find(RVALUE[opR].begin(), RVALUE[opR].end(), RVALUE[opR][j]);
                        RVALUE[opR].erase(iter);
                        break;
                    }
                }
            }
        }
        if (tag.second)
        {
            AVALUE[V] = vector<string>{ R };
            RVALUE[R].push_back(pair<string, int>(V, tag.first));
        }
        else
        {
            AVALUE.erase(V);
        }
    }
    else
    {
        bool is_find = false;
        //在R的记录中寻找V
        for (auto i = 0; i < RVALUE[R].size(); i++)
        {
            if (RVALUE[R][i].first == V)
            {
                //找到V后更新V
                is_find = true;
                RVALUE[R][i].second = tag.first;
                break;
            }
        }
        //没找到V添加V
        if (!is_find)
            RVALUE[R].push_back(pair<string, int>(V, tag.first));

        //V是否存在于AVALUE
        if (AVALUE.find(V) == AVALUE.end())
        {
            //V不存在于AVALUE中则新建V
            AVALUE[V] = vector<string>{ R };
        }
        else
        {
            //V在AVALUE中
            if (find(AVALUE[V].begin(), AVALUE[V].end(), R) == AVALUE[V].end())
                //V的记录中没有R则添加R
                AVALUE[V].push_back(R);
        }
    }
}

void Generator::endBlock(){
    //基本块处理结束
    for (map<string, vector<string>>::iterator iter = AVALUE.begin(); iter != AVALUE.end(); iter++)
    {
        string V = iter->first;
        //尚未存入内存的变量
        if (find(AVALUE[V].begin(), AVALUE[V].end(), "M") == AVALUE[V].end())
        {
            string R;
            for (auto i = 0; i < AVALUE[V].size(); i++)
            {
                if (AVALUE[V][i] != "M")

                {
                    R = AVALUE[V][i];
                    break;
                }
            }
            if (V[0] == 'G')
                EMIT("sw " + R + "," + DATA + "+" + to_string(stoi(V.substr(1))));
            else if (V[0] == 'V')
                EMIT("sw " + R + "," + STACK + "+" + to_string(4 + stoi(V.substr(1))) + "($fp)");
            else if (V[0] == 'T')
                EMIT("sw " + R + "," + TEMP + "+" + to_string(4 * stoi(V.substr(1))));
            else
            {
                throw string("ERROR: AVALUE has unexpected value:") + V + string("\n");
            }
        }
    }
    for (map<string, vector<pair<string, int>>>::iterator iter = RVALUE.begin(); iter != RVALUE.end(); iter++)
    {
        iter->second.clear();
    }
    AVALUE.clear();
}

void Generator::genObjCode(){
    EMIT(".data");
    EMIT(DATA + ":.space " + to_string(data_buf_size));
    EMIT(STACK + ":.space " + to_string(stack_buf_size));
    EMIT(TEMP + ":.space " + to_string(temp_buf_size));
    EMIT(".text");
    EMIT("j B0");
    EMIT("B1:");
    EMIT("nop");
    EMIT("j B1");
    EMIT("nop");
    EMIT("B2:");
    EMIT("jal Fmain");
    EMIT("nop");
    EMIT("break");
    EMIT("B0:");
    EMIT("addi $gp,$zero,0");
    EMIT("addi $fp,$zero,0");
    EMIT("addi $sp,$zero,4");
    EMIT("j B2");
    EMIT("nop");

    for (auto block_no = 0; block_no < blkGroup.size(); block_no++){

        vector<MsgTableItem> MsgTable = genMsgTable(block_no);
        bool j_end = false;

        for (auto i = 0; i < MsgTable.size(); i++)
        {
            reCode.clear();
            Quaternion TAS = MsgTable[i].TAS;
            string Reg_arg1, Reg_arg2;
            if (TAS.arg1 == "" || TAS.op == "=[]")
                Reg_arg1 = "";
            else if (TAS.arg1[0] == '$')
                Reg_arg1 = TAS.arg1;
            else if (TAS.arg1 == "[$sp]")
                Reg_arg1 = STACK + "($sp)";
            else if (isNum(TAS.arg1))
            {
                if (TAS.op == "+")
                    Reg_arg1 = TAS.arg1;
                else
                {
                    EMIT("addi $t8,$zero," + TAS.arg1);
                    Reg_arg1 = "$t8";
                }
            }
            else if (TAS.arg1[0] == 'G')
            {
                if (AVALUE.find(TAS.arg1) == AVALUE.end())
                {
                    AVALUE[TAS.arg1] = vector<string>{ "M" };
                }
                else if (AVALUE[TAS.arg1].size() == 0)
                {
                    throw string("ERROR: Can't find the address of") + TAS.arg1 + string("\n");
                }

                if (AVALUE[TAS.arg1].size() == 1 && AVALUE[TAS.arg1][0] == "M")
                {
                    EMIT("lw $t8," + DATA + "+" + to_string(stoi(TAS.arg1.substr(1))));
                    Reg_arg1 = "$t8";
                }
                else
                {
                    for (auto i = 0; i < AVALUE[TAS.arg1].size(); i++)
                    {
                        if (AVALUE[TAS.arg1][i] != "M")
                            Reg_arg1 = AVALUE[TAS.arg1][i];
                    }
                }
            }
            else if (TAS.arg1[0] == 'V')
            {
                if (AVALUE.find(TAS.arg1) == AVALUE.end())
                {
                    AVALUE[TAS.arg1] = vector<string>{ "M" };
                }
                else if (AVALUE[TAS.arg1].size() == 0)
                {
                    throw string("ERROR: Can't find the address of") + TAS.arg1 + string("\n");
                }

                if (AVALUE[TAS.arg1].size() == 1 && AVALUE[TAS.arg1][0] == "M")
                {
                    EMIT("lw $t8," + STACK + "+" + to_string(4 + stoi(TAS.arg1.substr(1))) + "($fp)");
                    Reg_arg1 = "$t8";
                }
                else
                {
                    for (auto i = 0; i < AVALUE[TAS.arg1].size(); i++)
                    {
                        if (AVALUE[TAS.arg1][i] != "M")
                            Reg_arg1 = AVALUE[TAS.arg1][i];
                    }
                }
            }
            else if (TAS.arg1[0] == 'T')
            {
                if (AVALUE.find(TAS.arg1) == AVALUE.end())
                {
                    AVALUE[TAS.arg1] = vector<string>{ "M" };
                }
                else if (AVALUE[TAS.arg1].size() == 0)
                {
                    throw string("ERROR: Can't find the address of") + TAS.arg1 + string("\n");
                }

                if (AVALUE[TAS.arg1].size() == 1 && AVALUE[TAS.arg1][0] == "M")
                {
                    EMIT("lw $t8," + TEMP + "+" + to_string(4 * stoi(TAS.arg1.substr(1))));
                    Reg_arg1 = "$t8";
                }
                else
                {
                    for (auto i = 0; i < AVALUE[TAS.arg1].size(); i++)
                    {
                        if (AVALUE[TAS.arg1][i] != "M")
                            Reg_arg1 = AVALUE[TAS.arg1][i];
                    }
                }
            }

            if (TAS.arg2 == "")
                Reg_arg2 = "";
            else if (TAS.arg2[0] == '$')
                Reg_arg2 = TAS.arg2;
            else if (TAS.arg2 == "[$sp]")
                Reg_arg2 = STACK + "($sp)";
            else if (isNum(TAS.arg2))
            {
                if (TAS.op == "+" && !isNum(Reg_arg1))//不能有两个立即数
                    Reg_arg2 = TAS.arg2;
                else
                {
                    EMIT("addi $t9,$zero," + TAS.arg2);
                    Reg_arg2 = "$t9";
                }
            }
            else if (TAS.arg2[0] == 'G')
            {
                if (AVALUE.find(TAS.arg2) == AVALUE.end())
                {
                    AVALUE[TAS.arg2] = vector<string>{ "M" };
                }
                else if (AVALUE[TAS.arg2].size() == 0)
                {
                    throw string("ERROR: Can't find the address of") + TAS.arg2 + string("\n");
                }

                if (AVALUE[TAS.arg2].size() == 1 && AVALUE[TAS.arg2][0] == "M")
                {
                    EMIT("lw $t9," + DATA + "+" + to_string(stoi(TAS.arg2.substr(1))));
                    Reg_arg2 = "$t9";
                }
                else
                {
                    for (auto i = 0; i < AVALUE[TAS.arg2].size(); i++)
                    {
                        if (AVALUE[TAS.arg2][i] != "M")
                            Reg_arg2 = AVALUE[TAS.arg2][i];
                    }
                }
            }
            else if (TAS.arg2[0] == 'V')
            {
                if (AVALUE.find(TAS.arg2) == AVALUE.end())
                {
                    AVALUE[TAS.arg2] = vector<string>{ "M" };
                }
                else if (AVALUE[TAS.arg2].size() == 0)
                {
                    throw string("ERROR: Can't find the address of") + TAS.arg2 + string("\n");
                }

                if (AVALUE[TAS.arg2].size() == 1 && AVALUE[TAS.arg2][0] == "M")
                {
                    EMIT("lw $t9," + STACK + "+" + to_string(4 + stoi(TAS.arg2.substr(1))) + "($fp)");
                    Reg_arg2 = "$t9";
                }
                else
                {
                    for (auto i = 0; i < AVALUE[TAS.arg2].size(); i++)
                    {
                        if (AVALUE[TAS.arg2][i] != "M")
                            Reg_arg2 = AVALUE[TAS.arg2][i];
                    }
                }
            }
            else if (TAS.arg2[0] == 'T')
            {
                if (AVALUE.find(TAS.arg2) == AVALUE.end())
                {
                    AVALUE[TAS.arg2] = vector<string>{ "M" };
                }
                else if (AVALUE[TAS.arg2].size() == 0)
                {
                    throw string("ERROR: Can't find the address of") + TAS.arg2 + string("\n");
                }

                if (AVALUE[TAS.arg2].size() == 1 && AVALUE[TAS.arg2][0] == "M")
                {
                    EMIT("lw $t9," + TEMP + "+" + to_string(4 * stoi(TAS.arg2.substr(1))));
                    Reg_arg2 = "$t9";
                }
                else
                {
                    for (auto i = 0; i < AVALUE[TAS.arg2].size(); i++)
                    {
                        if (AVALUE[TAS.arg2][i] != "M")
                            Reg_arg2 = AVALUE[TAS.arg2][i];
                    }
                }
            }

            if (TAS.op[0] == 'F' || TAS.op[0] == 'L')
            {
                EMIT(TAS.op + ":");
            }
            else if (TAS.op == "nop")
            {
                EMIT("nop");
            }
            else if (TAS.op == "j")
            {
                j_end = true;
                endBlock();
                EMIT("j " + TAS.result);
            }
            else if (TAS.op == "jal")
            {
                j_end = true;
                //endBlock();
                EMIT("jal " + TAS.result);
            }
            else if (TAS.op == "break")
            {
                j_end = true;
                endBlock();
                EMIT("break");
            }
            else if (TAS.op == "ret")
            {
                j_end = true;
                endBlock();
                EMIT("jr $ra");
            }
            else if (TAS.op == "jnz")
            {
                j_end = true;
                if (RVALUE.find(Reg_arg1) != RVALUE.end())
                    freshRA(MsgTable[i].arg1, Reg_arg1, TAS.arg1, false);
                endBlock();
                EMIT("bne " + Reg_arg1 + ",$zero," + TAS.result);
            }
            else if (TAS.op == "j<")
            {
                j_end = true;
                EMIT("addi $t8," + Reg_arg1 + ",1");
                EMIT("sub $t9," + Reg_arg2 + ",$t8");
                if (RVALUE.find(Reg_arg1) != RVALUE.end())
                    freshRA(MsgTable[i].arg1, Reg_arg1, TAS.arg1, false);
                if (RVALUE.find(Reg_arg2) != RVALUE.end())
                    freshRA(MsgTable[i].arg2, Reg_arg2, TAS.arg2, false);
                endBlock();
                EMIT("bgez $t9," + TAS.result);
            }
            else if (TAS.op == "j<=")
            {
                j_end = true;
                EMIT("sub $t9," + Reg_arg2 + "," + Reg_arg1);
                if (RVALUE.find(Reg_arg1) != RVALUE.end())
                    freshRA(MsgTable[i].arg1, Reg_arg1, TAS.arg1, false);
                if (RVALUE.find(Reg_arg2) != RVALUE.end())
                    freshRA(MsgTable[i].arg2, Reg_arg2, TAS.arg2, false);
                endBlock();
                EMIT("bgez $t9," + TAS.result);
            }
            else if (TAS.op == "j>")
            {
                j_end = true;
                EMIT("addi $t9," + Reg_arg2 + ",1");
                EMIT("sub  $t8," + Reg_arg1 + ",$t9");
                if (RVALUE.find(Reg_arg1) != RVALUE.end())
                    freshRA(MsgTable[i].arg1, Reg_arg1, TAS.arg1, false);
                if (RVALUE.find(Reg_arg2) != RVALUE.end())
                    freshRA(MsgTable[i].arg2, Reg_arg2, TAS.arg2, false);
                endBlock();
                EMIT("bgez $t8," + TAS.result);
            }
            else if (TAS.op == "j>=")
            {
                j_end = true;
                EMIT("sub $t8," + Reg_arg1 + "," + Reg_arg2);
                if (RVALUE.find(Reg_arg1) != RVALUE.end())
                    freshRA(MsgTable[i].arg1, Reg_arg1, TAS.arg1, false);
                if (RVALUE.find(Reg_arg2) != RVALUE.end())
                    freshRA(MsgTable[i].arg2, Reg_arg2, TAS.arg2, false);
                endBlock();
                EMIT("bgez $t8," + TAS.result);
            }
            else if (TAS.op == "j==")
            {
                j_end = true;
                if (RVALUE.find(Reg_arg1) != RVALUE.end())
                    freshRA(MsgTable[i].arg1, Reg_arg1, TAS.arg1, false);
                if (RVALUE.find(Reg_arg2) != RVALUE.end())
                    freshRA(MsgTable[i].arg2, Reg_arg2, TAS.arg2, false);
                endBlock();
                EMIT("beq " + Reg_arg1 + "," + Reg_arg2 + "," + TAS.result);
            }
            else if (TAS.op == "j!=")
            {
                j_end = true;
                if (RVALUE.find(Reg_arg1) != RVALUE.end())
                    freshRA(MsgTable[i].arg1, Reg_arg1, TAS.arg1, false);
                if (RVALUE.find(Reg_arg2) != RVALUE.end())
                    freshRA(MsgTable[i].arg2, Reg_arg2, TAS.arg2, false);
                endBlock();
                EMIT("bne " + Reg_arg1 + "," + Reg_arg2 + "," + TAS.result);
            }
            else if (TAS.op == ":=")
            {
                string R;
                if (TAS.result[0] == '$')
                {
                    R = TAS.result;
                    if (TAS.arg1 == "[$sp]")
                    {
                        EMIT("lw " + R + "," + Reg_arg1);
                    }
                    else
                    {
                        EMIT("add " + R + ",$zero," + Reg_arg1);
                    }
                }
                else if (TAS.result == "[$sp]")
                {
                    R = STACK + "($sp)";
                    if (TAS.arg1 == "[$sp]")
                    {
                        throw string("ERROR: [$sp] can't be assigned to [$sp]\n");
                    }
                    else
                    {
                        EMIT("sw " + Reg_arg1 + "," + R);
                    }
                }
                else
                {
                    R = getREG(TAS.result);
                    if (TAS.arg1 == "[$sp]")
                    {
                        EMIT("lw " + R + "," + Reg_arg1);
                    }
                    else
                    {
                        EMIT("add " + R + ",$zero," + Reg_arg1);
                    }
                }
                if (RVALUE.find(R) != RVALUE.end())
                    freshRA(MsgTable[i].result, R, TAS.result, true);
                if (RVALUE.find(Reg_arg1) != RVALUE.end())
                    freshRA(MsgTable[i].arg1, Reg_arg1, TAS.arg1, false);
            }
            else if (TAS.op == "[]=")
            {
                string base = TAS.result;
                if (TAS.result[0] == 'G')
                {
                    EMIT("sll $t9," + Reg_arg2 + ",2");
                    EMIT("addi $t9,$t9," + base.substr(1));
                    EMIT("sw " + Reg_arg1 + "," + DATA + "($t9)");
                }
                else if (TAS.result[0] == 'V')
                {
                    EMIT("sll $t9," + Reg_arg2 + ",2");
                    EMIT("addi $t9,$t9," + base.substr(1));
                    EMIT("addi $t9,$t9,4");
                    EMIT("add $t9,$t9,$fp");
                    EMIT("sw " + Reg_arg1 + "," + STACK + "($t9)");
                }
                else if (TAS.result[0] == 'T')
                {
                    EMIT("addi $t9," + Reg_arg2 + "," + base.substr(1));
                    EMIT("sll $t9,$t9,2");
                    EMIT("sw " + Reg_arg1 + "," + TEMP + "($t9)");
                }
                else
                {
                    throw string("ERROR: []=result is illegal\n");
                }
                if (RVALUE.find(Reg_arg1) != RVALUE.end())
                    freshRA(MsgTable[i].arg1, Reg_arg1, TAS.arg1, false);
                if (RVALUE.find(Reg_arg2) != RVALUE.end())
                    freshRA(MsgTable[i].arg2, Reg_arg2, TAS.arg2, false);
            }
            else if (TAS.op == "=[]")
            {
                string R;
                if (TAS.result[0] == '$')
                    R = TAS.result;
                else if (TAS.result == "[$sp]")
                    R = STACK + "($sp)";
                else
                    R = getREG(TAS.result);
                if (TAS.arg1[0] == 'G')
                {
                    EMIT("sll $t9," + Reg_arg2 + ",2");
                    EMIT("addi $t9,$t9," + TAS.arg1.substr(1));
                    EMIT("lw " + R + "," + DATA + "($t9)");
                }
                else if (TAS.arg1[0] == 'V')
                {
                    EMIT("sll $t9," + Reg_arg2 + ",2");
                    EMIT("addi $t9,$t9," + TAS.arg1.substr(1));
                    EMIT("addi $t9,$t9,4");
                    EMIT("add $t9,$t9,$fp");
                    EMIT("lw " + R + "," + STACK + "($t9)");
                }
                else if (TAS.arg1[0] == 'T')
                {
                    EMIT("addi $t9," + Reg_arg2 + "," + TAS.arg1.substr(1));
                    EMIT("sll $t9,$t9,2");
                    EMIT("lw " + R + "," + TEMP + "($t9)");
                }
                else
                {
                    throw string("ERROR: =[] arg1 is illegal\n");
                }
                if (RVALUE.find(R) != RVALUE.end())
                    freshRA(MsgTable[i].result, R, TAS.result, true);
                if (RVALUE.find(Reg_arg2) != RVALUE.end())
                    freshRA(MsgTable[i].arg2, Reg_arg2, TAS.arg2, false);
            }
            else if (TAS.op == "+")
            {
                string R;
                if (TAS.result[0] == '$')
                    R = TAS.result;
                else if (TAS.result == "[$sp]")
                    R = STACK + "($sp)";
                else
                    R = getREG(TAS.result);

                if (isNum(Reg_arg1))
                    EMIT("addi " + R + "," + Reg_arg2 + "," + Reg_arg1);
                else if (isNum(Reg_arg2))
                    EMIT("addi " + R + "," + Reg_arg1 + "," + Reg_arg2);
                else
                    EMIT("add " + R + "," + Reg_arg1 + "," + Reg_arg2);
                if (RVALUE.find(R) != RVALUE.end())
                    freshRA(MsgTable[i].result, R, TAS.result, true);
                if (RVALUE.find(Reg_arg1) != RVALUE.end())
                    freshRA(MsgTable[i].arg1, Reg_arg1, TAS.arg1, false);
                if (RVALUE.find(Reg_arg2) != RVALUE.end())
                    freshRA(MsgTable[i].arg2, Reg_arg2, TAS.arg2, false);
            }
            else if (TAS.op == "-")
            {
                string R;
                if (TAS.result[0] == '$')
                    R = TAS.result;
                else if (TAS.result == "[$sp]")
                    R = STACK + "($sp)";
                else
                    R = getREG(TAS.result);
                EMIT("sub " + R + "," + Reg_arg1 + "," + Reg_arg2);
                if (RVALUE.find(R) != RVALUE.end())
                    freshRA(MsgTable[i].result, R, TAS.result, true);
                if (RVALUE.find(Reg_arg1) != RVALUE.end())
                    freshRA(MsgTable[i].arg1, Reg_arg1, TAS.arg1, false);
                if (RVALUE.find(Reg_arg2) != RVALUE.end())
                    freshRA(MsgTable[i].arg2, Reg_arg2, TAS.arg2, false);
            }
            else if (TAS.op == "&")
            {
                string R;
                if (TAS.result[0] == '$')
                    R = TAS.result;
                else if (TAS.result == "[$sp]")
                    R = STACK + "($sp)";
                else
                    R = getREG(TAS.result);
                EMIT("and " + R + "," + Reg_arg1 + "," + Reg_arg2);
                if (RVALUE.find(R) != RVALUE.end())
                    freshRA(MsgTable[i].result, R, TAS.result, true);
                if (RVALUE.find(Reg_arg1) != RVALUE.end())
                    freshRA(MsgTable[i].arg1, Reg_arg1, TAS.arg1, false);
                if (RVALUE.find(Reg_arg2) != RVALUE.end())
                    freshRA(MsgTable[i].arg2, Reg_arg2, TAS.arg2, false);
            }
            else if (TAS.op == "|")
            {
                string R;
                if (TAS.result[0] == '$')
                    R = TAS.result;
                else if (TAS.result == "[$sp]")
                    R = STACK + "($sp)";
                else
                    R = getREG(TAS.result);
                EMIT("or " + R + "," + Reg_arg1 + "," + Reg_arg2);
                if (RVALUE.find(R) != RVALUE.end())
                    freshRA(MsgTable[i].result, R, TAS.result, true);
                if (RVALUE.find(Reg_arg1) != RVALUE.end())
                    freshRA(MsgTable[i].arg1, Reg_arg1, TAS.arg1, false);
                if (RVALUE.find(Reg_arg2) != RVALUE.end())
                    freshRA(MsgTable[i].arg2, Reg_arg2, TAS.arg2, false);
            }
            else if (TAS.op == "^")
            {
                string R;
                if (TAS.result[0] == '$')
                    R = TAS.result;
                else if (TAS.result == "[$sp]")
                    R = STACK + "($sp)";
                else
                    R = getREG(TAS.result);
                EMIT("xor " + R + "," + Reg_arg1 + "," + Reg_arg2);
                if (RVALUE.find(R) != RVALUE.end())
                    freshRA(MsgTable[i].result, R, TAS.result, true);
                if (RVALUE.find(Reg_arg1) != RVALUE.end())
                    freshRA(MsgTable[i].arg1, Reg_arg1, TAS.arg1, false);
                if (RVALUE.find(Reg_arg2) != RVALUE.end())
                    freshRA(MsgTable[i].arg2, Reg_arg2, TAS.arg2, false);
            }
            else if (TAS.op == "*")
            {
                string R;
                if (TAS.result[0] == '$')
                    R = TAS.result;
                else if (TAS.result == "[$sp]")
                    R = STACK + "($sp)";
                else
                    R = getREG(TAS.result);
                EMIT("mul " + R + "," + Reg_arg1 + "," + Reg_arg2);
                if (RVALUE.find(R) != RVALUE.end())
                    freshRA(MsgTable[i].result, R, TAS.result, true);
                if (RVALUE.find(Reg_arg1) != RVALUE.end())
                    freshRA(MsgTable[i].arg1, Reg_arg1, TAS.arg1, false);
                if (RVALUE.find(Reg_arg2) != RVALUE.end())
                    freshRA(MsgTable[i].arg2, Reg_arg2, TAS.arg2, false);
            }
            else if (TAS.op == "/")
            {
                string R;
                if (TAS.result[0] == '$')
                    R = TAS.result;
                else if (TAS.result == "[$sp]")
                    R = STACK + "($sp)";
                else
                    R = getREG(TAS.result);
                EMIT("div " + Reg_arg1 + "," + Reg_arg2);
                EMIT("mflo " + R);//Quotient in $lo
                if (RVALUE.find(R) != RVALUE.end())
                    freshRA(MsgTable[i].result, R, TAS.result, true);
                if (RVALUE.find(Reg_arg1) != RVALUE.end())
                    freshRA(MsgTable[i].arg1, Reg_arg1, TAS.arg1, false);
                if (RVALUE.find(Reg_arg2) != RVALUE.end())
                    freshRA(MsgTable[i].arg2, Reg_arg2, TAS.arg2, false);
            }
            else
            {
                throw string("ERROR:  Illegal code:") + TAS.op + string(" ") + TAS.arg1 + string(" ") + TAS.arg2 + string(" ") + TAS.result + string("\n");
            }
            aysHis.push_back({ TAS,reCode,RVALUE,AVALUE });
        }
        if (!j_end)
            endBlock();
    }
}
