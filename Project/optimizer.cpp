#include "optimizer.h"

Optimizer::Optimizer(){
}

Optimizer::~Optimizer(){
}

void Optimizer::init(map<int, string> nt, SymbolTable* gt, vector<Quaternion> ic){
    nameTable = nt;
    glbTable = gt;
    interCode = ic;
}

bool Optimizer::isNum(string str){
    stringstream sin(str);
    int d;
    char c;
    if (!(sin >> d))
        return false;
    if (sin >> c)
        return false;
    return true;
}

bool Optimizer::preOptimize(){
    int m_id = -1;
    int m_offset;

    for (map<int, string>::iterator iter = nameTable.begin(); iter != nameTable.end(); iter++){
        if (iter->second == "main" && m_id == -1)
            m_id = iter->first;
        else if (iter->second == "main" && m_id != -1)
            throw("ERROR: There can only be one main function.\n");
    }

    if (m_id == -1)
        throw("ERROR: Main function not define.\n");

    for (auto i = 0; i < glbTable->table.size(); i++){
        if (glbTable->table[i].id == m_id){
            m_offset = glbTable->table[i].offset;
            break;
        }
    }

    labelTable[m_offset] = "Fmain";
    int normal_label_count = 0;
    int function_label_count = 0;
    for (auto i = 0; i < interCode.size(); i++){
        Quaternion* e = &interCode[i];
        if (e->op == "jal"){
            if (labelTable.find(stoi(e->result)) == labelTable.end())
                labelTable[stoi(e->result)] = "F" + to_string(function_label_count++);
            e->result = labelTable[stoi(e->result)];
        }
        else if (e->op[0] == 'j')
        {
            if (labelTable.find(stoi(e->result)) == labelTable.end())
                labelTable[stoi(e->result)] = "L" + to_string(normal_label_count++);
            e->result = labelTable[stoi(e->result)];
        }
    }

    vector<Quaternion> newcode;
    for (auto i = 0; i < interCode.size(); i++){
        Quaternion e = interCode[i];
        if (labelTable.find(i) != labelTable.end()){
            Quaternion label = { labelTable[i],"","","" };
            newcode.push_back(label);
        }
        newcode.push_back(e);
    }

    interCode = newcode;
    return true;
}

void Optimizer::partition(){
    Block block;
    for (auto i = 0; i < interCode.size(); i++){
        Quaternion e = interCode[i];
        bool jmp_flag = (i - 1 >= 0 && (interCode[i - 1].op[0] == 'j' || interCode[i - 1].op == "ret"));
        if (i == 0 || e.op[0] == 'L' || e.op[0] == 'F' || jmp_flag){
            block.begin = i;
            block.waitVar.clear();
            block.activeVar.clear();
        }

        if ((e.result[0] == 'V' || e.result[0] == 'T') && find(block.waitVar.begin(), block.waitVar.end(), e.result) == block.waitVar.end())
            block.waitVar.push_back(e.result);
        if ((e.arg1[0] == 'V' || e.arg1[0] == 'T') && find(block.waitVar.begin(), block.waitVar.end(), e.arg1) == block.waitVar.end())
            block.waitVar.push_back(e.arg1);
        if ((e.arg2[0] == 'V' || e.arg2[0] == 'T') && find(block.waitVar.begin(), block.waitVar.end(), e.arg2) == block.waitVar.end())
            block.waitVar.push_back(e.arg2);
        if ((e.arg1[0] == 'V' || e.arg1[0] == 'T') && find(block.activeVar.begin(), block.activeVar.end(), e.arg1) == block.activeVar.end())
            block.activeVar.push_back(e.arg1);
        if ((e.arg2[0] == 'V' || e.arg2[0] == 'T') && find(block.activeVar.begin(), block.activeVar.end(), e.arg2) == block.activeVar.end())
            block.activeVar.push_back(e.arg2);

        bool enter_flag = ((i + 1 < interCode.size() && (interCode[i + 1].op[0] == 'L' || interCode[i + 1].op[0] == 'F')) || e.op[0] == 'j' || e.op == "ret" || e.op == "break");
        if (enter_flag){
            block.end = i;
            blockGroup.push_back(block);
        }
    }

    // 对于while语句的补丁
    map<string, int> label_loc;
    for (auto pos = 0; pos < interCode.size(); pos++){
        if (interCode[pos].op[0] == 'L')
            label_loc[interCode[pos].op] = pos;
    }

    for (auto i = 0; i < blockGroup.size(); i++){
        Block* e = &blockGroup[i];
        // 如果这个基本块是函数返回块，那么这个基本块内的赋值变量肯定不会被其他函数内语句块所用到
        if (interCode[e->end].op == "ret"){
            e->uselessVar = e->waitVar;
            e->waitVar.clear();
        }
        // 这个基本块不发生返回
        else{
            vector<string> real_wait_variable;
            // 对于while语句的补丁
            int pos = blockGroup[i + 1].begin;
            int prepos = pos - 1;
            while (prepos < interCode.size()){
                if (label_loc.find(interCode[prepos].result) != label_loc.end() && label_loc[interCode[prepos].result] < pos){
                    pos = label_loc[interCode[prepos].result];
                    prepos = label_loc[interCode[prepos].result];
                }
                prepos++;
            }

            while (pos < interCode.size() && interCode[pos].op[0] != 'F'){
                if (find(e->waitVar.begin(), e->waitVar.end(), interCode[pos].arg1) != e->waitVar.end() && find(real_wait_variable.begin(), real_wait_variable.end(), interCode[pos].arg1) == real_wait_variable.end())
                    real_wait_variable.push_back(interCode[pos].arg1);
                if (find(e->waitVar.begin(), e->waitVar.end(), interCode[pos].arg2) != e->waitVar.end() && find(real_wait_variable.begin(), real_wait_variable.end(), interCode[pos].arg2) == real_wait_variable.end())
                    real_wait_variable.push_back(interCode[pos].arg2);
                pos++;
            }

            for (auto j = 0; j < e->waitVar.size(); j++){
                if (find(real_wait_variable.begin(), real_wait_variable.end(), e->waitVar[j]) == real_wait_variable.end())
                    e->uselessVar.push_back(e->waitVar[j]);
            }
            e->waitVar = real_wait_variable;
        }
    }
}

vector<DAGItem> Optimizer::genDAG(int block_no){
    vector<DAGItem> DAG;
    Block* block = &blockGroup[block_no];
    for (auto pos = block->begin; pos <= block->end; pos++){
        string op = interCode[pos].op;
        string B = interCode[pos].arg1;
        string C = interCode[pos].arg2;
        string A = interCode[pos].result;

        int element_count;
        if (op == "nop" || op[0] == 'F' || op[0] == 'L') element_count = -1;
        else if (A[0] == '$' || A == "[$sp]") element_count = -1;
        else if (op == ":=") element_count = 0;
        else if (op == "=[]") element_count = 2;
        else if (op == "[]=") element_count = 3;
        else if (op == "j<" || op == "j<=" || op == "j>" || op == "j>=" || op == "j==" || op == "j!=") element_count = -1;
        else if (op == "jnz") element_count = -1;
        else if (op == "j" || op == "jal" || op == "break" || op == "ret") element_count = -1;
        else element_count = 2;

        // 不做DAG转化的中间代码
        if (element_count == -1){
            DAGItem newDAG;
            newDAG.isRemain = true;
            newDAG.code = interCode[pos];
            DAG.push_back(newDAG);

            // 保证每一个叶结点都有值，过期的值前面加-，防止再次被选为源操作数
            if (A[0] == '$' || A == "[$sp]"){
                for (auto i = 0; i < DAG.size(); i++){
                    if (DAG[i].isLeaf && DAG[i].value == A){
                        DAG[i].value = "-" + A;
                        break;
                    }
                }
            }
            continue;
        }

        // 对该中间代码生成DAG
        int state = 1;
        int n;
        int A_no;
        bool new_A;
        int B_no;
        bool new_B;
        int C_no;
        bool new_C;
        while (state > 0){
            switch (state){
            case 1:
            {
                //在已有DAG节点中寻找B
                B_no = -1;
                for (auto i = 0; i < DAG.size(); i++){
                    if ((DAG[i].isLeaf && DAG[i].value == B) || find(DAG[i].label.begin(), DAG[i].label.end(), B) != DAG[i].label.end()){
                        B_no = i;
                        new_B = false;
                        break;
                    }
                }

                //已有DAG中没有B则新建B的DAG节点
                if (B_no == -1){
                    DAGItem newDAG;
                    newDAG.isLeaf = true;
                    newDAG.value = B;
                    B_no = DAG.size();
                    new_B = true;
                    DAG.push_back(newDAG);
                }

                if (element_count == 0){
                    n = B_no;
                    state = 4;
                }

                else if (element_count == 1)
                    state = 21;
                else if (element_count == 2){
                    // 在已有DAG节点中寻找C
                    C_no = -1;
                    for (auto i = 0; i < DAG.size(); i++){
                        if ((DAG[i].isLeaf && DAG[i].value == C) || find(DAG[i].label.begin(), DAG[i].label.end(), C) != DAG[i].label.end()){
                            C_no = i;
                            new_C = false;
                            break;
                        }
                    }

                    //已有DAG中没有C则新建C的DAG节点
                    if (C_no == -1){
                        DAGItem newDAG;
                        newDAG.isLeaf = true;
                        newDAG.value = C;
                        C_no = DAG.size();
                        new_C = true;
                        DAG.push_back(newDAG);
                    }
                    state = 22;
                }

                else if (element_count == 3){
                    //在已有DAG节点中寻找C
                    C_no = -1;
                    for (auto i = 0; i < DAG.size(); i++){
                        if ((DAG[i].isLeaf && DAG[i].value == C) || find(DAG[i].label.begin(), DAG[i].label.end(), C) != DAG[i].label.end()){
                            C_no = i;
                            new_C = false;
                            break;
                        }
                    }

                    //已有DAG中没有C则新建C的DAG节点
                    if (C_no == -1){
                        DAGItem newDAG;
                        newDAG.isLeaf = true;
                        newDAG.value = C;
                        C_no = DAG.size();
                        new_C = true;
                        DAG.push_back(newDAG);
                    }

                    //在已有DAG节点中寻找A
                    A_no = -1;
                    for (auto i = 0; i < DAG.size(); i++){
                        if ((DAG[i].isLeaf && DAG[i].value == A) || find(DAG[i].label.begin(), DAG[i].label.end(), A) != DAG[i].label.end()){
                            A_no = i;
                            new_A = false;
                            break;
                        }
                    }

                    //已有DAG中没有A则新建A的DAG节点
                    if (A_no == -1){
                        DAGItem newDAG;
                        newDAG.isLeaf = true;
                        newDAG.value = A;
                        A_no = DAG.size();
                        new_A = true;
                        DAG.push_back(newDAG);
                    }

                    DAGItem newDAG;
                    newDAG.isLeaf = false;
                    newDAG.op = op;
                    newDAG.lChild = B_no;
                    newDAG.rChild = C_no;
                    newDAG.tChild = A_no;
                    n = DAG.size();
                    DAG.push_back(newDAG);
                    DAG[B_no].parent = n;
                    DAG[C_no].parent = n;
                    DAG[A_no].parent = n;

                    //其他值为该数组中任意元素的叶结点失效
                    for (auto i = 0; i < DAG.size(); i++){
                        if (DAG[i].isLeaf && DAG[i].value == A){
                            DAG[i].value = "-" + A;
                            break;
                        }
                    }
                    state = -1;
                }
                else
                    state = -1;
                break;
            }

            case 21:
            {
                if (DAG[B_no].isLeaf && isNum(DAG[B_no].value)){
                    //B是立即数
                    state = 23;
                }
                else
                    state = 31;
                break;
            }

            case 22:
            {
                if ((DAG[B_no].isLeaf && isNum(DAG[B_no].value)) && (DAG[C_no].isLeaf && isNum(DAG[C_no].value))){
                    //B和C是立即数
                    state = 24;
                }
                else
                    state = 32;
                break;
            }

            case 23:
            {
                //实际上不存在单目运算
                state = -1;
                break;
            }

            case 24:
            {
                int B = stoi(DAG[B_no].value);
                int C = stoi(DAG[C_no].value);
                int P;
                if (op == "+")
                    P = B + C;
                else if (op == "-")
                    P = B - C;
                else if (op == "&")
                    P = B & C;
                else if (op == "|")
                    P = B | C;
                else if (op == "^")
                    P = B ^ C;
                else if (op == "*")
                    P = B * C;
                else if (op == "/")
                    P = B / C;

                DAGItem tmpB = DAG[B_no], tmpC = DAG[C_no];
                //如果B是新建的则无需新建B的DAG节点
                if (new_B){
                    vector<DAGItem>::iterator i;
                    i = find(DAG.begin(), DAG.end(), tmpB);
                    DAG.erase(i);
                }

                //如果C是新建的则无需新建C的DAG节点
                if (new_C){
                    vector<DAGItem>::iterator i;
                    i = find(DAG.begin(), DAG.end(), tmpC);
                    DAG.erase(i);
                }

                //寻找计算结果是否已经有DAG节点
                n = -1;
                for (auto i = 0; i < DAG.size(); i++){
                    if ((DAG[i].isLeaf && DAG[i].value == to_string(P)) || find(DAG[i].label.begin(), DAG[i].label.end(), to_string(P)) != DAG[i].label.end())
                    {
                        n = i;
                        break;
                    }
                }

                //否则新建计算结果的叶节点
                if (n == -1){
                    DAGItem newDAG;
                    newDAG.isLeaf = true;
                    newDAG.value = to_string(P);
                    n = DAG.size();
                    DAG.push_back(newDAG);
                }
                state = 4;
                break;
            }

            case 31:
            {
                //寻找是否有相同运算的DAG
                n = -1;
                for (auto i = 0; i < DAG.size(); i++)
                {
                    if (!DAG[i].isLeaf && DAG[i].lChild == B_no && DAG[i].op == op)
                    {
                        n = i;
                        break;
                    }
                }
                //没有则新建根节点
                if (n == -1)
                {
                    DAGItem newDAG;
                    newDAG.isLeaf = false;
                    newDAG.op = op;
                    newDAG.lChild = B_no;
                    n = DAG.size();
                    DAG.push_back(newDAG);
                    DAG[B_no].parent = n;
                }
                state = 4;
                break;
            }

            case 32:
            {
                //寻找是否有相同运算的DAG
                n = -1;
                for (auto i = 0; i < DAG.size(); i++)
                {
                    if (!DAG[i].isLeaf && DAG[i].lChild == B_no && DAG[i].rChild == C_no && DAG[i].op == op)
                    {
                        n = i;
                        break;
                    }
                }
                //没有则新建根节点
                if (n == -1)
                {
                    DAGItem newDAG;
                    newDAG.isLeaf = false;
                    newDAG.op = op;
                    newDAG.lChild = B_no;
                    newDAG.rChild = C_no;
                    n = DAG.size();
                    DAG.push_back(newDAG);
                    DAG[B_no].parent = n;
                    DAG[C_no].parent = n;
                }
                state = 4;
                break;
            }

            case 4:
            {
                //如果A已经有DAG节点则从这些节点中去除A,但要保证每一个叶结点都有值，过期的值前面加-，防止再次被选为源操作数
                for (auto i = 0; i < DAG.size(); i++)
                {
                    if (DAG[i].isLeaf && DAG[i].value == A)
                    {
                        DAG[i].value = "-" + A;
                        break;
                    }
                    else if (find(DAG[i].label.begin(), DAG[i].label.end(), A) != DAG[i].label.end())
                    {
                        vector<string>::iterator iter;
                        iter = find(DAG[i].label.begin(), DAG[i].label.end(), A);
                        DAG[i].label.erase(iter);
                        break;
                    }
                }
                DAG[n].label.push_back(A);
                state = -1;
                break;
            }

            default:
                break;
            }
        }
    }
    return DAG;
}

void Optimizer::_utilizeChildren(vector<DAGItem>& DAG, int now){
    DAG[now].useful = true;
    if (!DAG[now].isLeaf){
        if (DAG[now].rChild != -1)
            _utilizeChildren(DAG, DAG[now].rChild);
        if (DAG[now].lChild != -1)
            _utilizeChildren(DAG, DAG[now].lChild);
        if (DAG[now].tChild != -1)
            _utilizeChildren(DAG, DAG[now].tChild);
    }
}

string Optimizer::newtemp(){
    return string("S") + to_string(tempCnt++);
}

void Optimizer::optimize(){
    vector<Quaternion> optimized_code;
    for (int block_no = 0; block_no < blockGroup.size(); block_no++){
        vector<DAGItem> DAG = genDAG(block_no);
        DAGGroup.push_back(DAG);
        Block newblock;
        newblock.begin = optimized_code.size();
        Block block = blockGroup[block_no];
        vector<string> wait_variable = block.waitVar;
        wait_variable.push_back("$gp");
        wait_variable.push_back("$sp");
        wait_variable.push_back("$fp");
        wait_variable.push_back("$v0");
        wait_variable.push_back("$t0");
        wait_variable.push_back("$t1");
        wait_variable.push_back("$t2");
        wait_variable.push_back("$t3");
        wait_variable.push_back("$t4");
        wait_variable.push_back("$t5");
        wait_variable.push_back("$t6");
        wait_variable.push_back("$t7");
        wait_variable.push_back("[$sp]");
        for (auto i = 0; i < DAG.size(); i++){
            if (DAG[i].isRemain){
                if (DAG[i].code.arg1 != "" && find(wait_variable.begin(), wait_variable.end(), DAG[i].code.arg1) == wait_variable.end())
                    wait_variable.push_back(DAG[i].code.arg1);
                if (DAG[i].code.arg2 != "" && find(wait_variable.begin(), wait_variable.end(), DAG[i].code.arg2) == wait_variable.end())
                    wait_variable.push_back(DAG[i].code.arg2);
            }
        }
        for (auto i = 0; i < DAG.size(); i++){
            if (!DAG[i].isRemain){
                if (DAG[i].tChild == -1){
                    vector<string> new_label;
                    for (auto j = 0; j < DAG[i].label.size(); j++){
                        if (DAG[i].label[j][0] == 'G' || find(wait_variable.begin(), wait_variable.end(), DAG[i].label[j]) != wait_variable.end()){
                            new_label.push_back(DAG[i].label[j]);
                            DAG[i].useful = true;
                        }
                    }

                    DAG[i].label = new_label;
                    if (DAG[i].useful)
                        _utilizeChildren(DAG, i);
                    if (!DAG[i].isLeaf && DAG[i].label.size() == 0)
                        DAG[i].label.push_back(newtemp());
                }

                else{
                    DAG[i].useful = true;
                    _utilizeChildren(DAG, i);
                }
            }
        }

        for (auto i = 0; i < DAG.size(); i++){
            if (DAG[i].isRemain)
                optimized_code.push_back(DAG[i].code);
            else{
                if (DAG[i].isLeaf){
                    for (auto j = 0; j < DAG[i].label.size(); j++){
                        string v;
                        if (DAG[i].value[0] == '-')
                            v = DAG[i].value.substr(1);
                        else
                            v = DAG[i].value;
                        Quaternion newTAS = { ":=",v,"",DAG[i].label[j] };
                        optimized_code.push_back(newTAS);
                    }
                }
                else{
                    string lv;
                    if (DAG[DAG[i].lChild].isLeaf){
                        if (DAG[DAG[i].lChild].value[0] == '-')
                            lv = DAG[DAG[i].lChild].value.substr(1);
                        else
                            lv = DAG[DAG[i].lChild].value;
                    }

                    else{
                        lv = DAG[DAG[i].lChild].label[0];
                    }

                    string rv;
                    if (DAG[DAG[i].rChild].isLeaf){
                        if (DAG[DAG[i].rChild].value[0] == '-')
                            rv = DAG[DAG[i].rChild].value.substr(1);
                        else
                            rv = DAG[DAG[i].rChild].value;
                    }
                    else{
                        rv = DAG[DAG[i].rChild].label[0];
                    }

                    if (DAG[i].tChild != -1){
                        string tri_v;
                        if (DAG[DAG[i].tChild].isLeaf){
                            if (DAG[DAG[i].tChild].value[0] == '-')
                                tri_v = DAG[DAG[i].tChild].value.substr(1);
                            else
                                tri_v = DAG[DAG[i].tChild].value;
                        }
                        else{
                            tri_v = DAG[DAG[i].tChild].label[0];
                        }
                        Quaternion newTAS = { DAG[i].op,lv,rv,tri_v };
                        optimized_code.push_back(newTAS);
                    }

                    else{
                        Quaternion newTAS = { DAG[i].op,lv,rv,DAG[i].label[0] };
                        optimized_code.push_back(newTAS);
                        for (auto label_no = 1; label_no < DAG[i].label.size(); label_no++){
                            Quaternion newTAS = { ":=",DAG[i].label[0],"",DAG[i].label[label_no] };
                            optimized_code.push_back(newTAS);
                        }
                    }
                }
            }
        }

        for(auto i=newblock.begin;i<optimized_code.size();i++)
        {
            Quaternion e=optimized_code[i];
            if(e.op=="+"&&e.arg1=="$sp"&&isNum(e.arg2)&&e.result=="$sp")
            {
                int sum=atoi(e.arg2.c_str());
                while(i+1<optimized_code.size()&&optimized_code[i+1].op=="+"&&optimized_code[i+1].arg1=="$sp"&&isNum(optimized_code[i+1].arg2)&&optimized_code[i+1].result=="$sp")
                {
                    sum+=atoi(optimized_code[i+1].arg2.c_str());
                    optimized_code.erase(optimized_code.begin()+i+1);
                }
                optimized_code[i].arg2=to_string(sum);
            }
        }
        newblock.end = optimized_code.size() - 1;
    }

    map<string, string> tmpV_map;
    int newtemp_counter = 0;
    for (auto pos = 0; pos < optimized_code.size(); pos++){
        Quaternion TAS = optimized_code[pos];
        if ((TAS.arg1[0] == 'T' || TAS.arg1[0] == 'S') && tmpV_map.find(TAS.arg1) == tmpV_map.end())
            tmpV_map[TAS.arg1] = "T" + to_string(newtemp_counter++);
        if ((TAS.arg2[0] == 'T' || TAS.arg2[0] == 'S') && tmpV_map.find(TAS.arg2) == tmpV_map.end())
            tmpV_map[TAS.arg2] = "T" + to_string(newtemp_counter++);
        if ((TAS.result[0] == 'T' || TAS.result[0] == 'S') && tmpV_map.find(TAS.result) == tmpV_map.end())
            tmpV_map[TAS.result] = "T" + to_string(newtemp_counter++);
    }

    for (auto pos = 0; pos < optimized_code.size(); pos++){
        Quaternion* pTAS = &optimized_code[pos];
        if (tmpV_map.find(pTAS->arg1) != tmpV_map.end())
            pTAS->arg1 = tmpV_map[pTAS->arg1];
        if (tmpV_map.find(pTAS->arg2) != tmpV_map.end())
            pTAS->arg2 = tmpV_map[pTAS->arg2];
        if (tmpV_map.find(pTAS->result) != tmpV_map.end())
            pTAS->result = tmpV_map[pTAS->result];
    }
    oriBlock = blockGroup;
    oriCode = interCode;
    blockGroup.clear();
    interCode = optimized_code;
    partition();
}

double Optimizer::optimizerAnalyse(){
    preOptimize();

    partition();

    int route = 0;
    int original_size = interCode.size();
    int optimize_size = 0;

    while(oriCode.size() != interCode.size()){
        optimize();
        route++;
    }
    optimize_size = interCode.size();

    return 100.0 * double(optimize_size) / double(original_size);
}
