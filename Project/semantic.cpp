#include "semantic.h"

Semantic::Semantic(){
    midList.clear();
    tempVarCnt = -1;
    offsetStack.clear();
    symTableStack.clear();
    currentSymTable = NULL;
}

Semantic::~Semantic() {
    midList.clear();
    tempVarCnt = -1;
    offsetStack.clear();
    symTableStack.clear();
    currentSymTable = NULL;
}

string Semantic::newtemp() {
    string temp_name = string("V") + to_string(offsetStack.back());
    symTableStack.back()->insert(tempVarCnt--, INT, VAR, offsetStack.back());
    offsetStack.back() += 4;

    emit("+", "$sp", to_string(4), "$sp");
    return temp_name;
}

string Semantic::entry(int id) {
    SymbolTable* tp = symTableStack.back();
    int offset;
    while (tp){
        for (int i = 0; i < tp->table.size(); i++){
            if (tp->table[i].id == id){
                offset = tp->table[i].offset;
                if (tp->table[i].k == VAR || tp->table[i].k == ARRAY){
                    if (tp->parent)
                        return string("V") + to_string(offset);
                    else
                        return string("G") + to_string(offset);
                }
            }
        }
        tp = tp->parent;
    }
    return "";
}

Symbol* Semantic::find(int id) {
    SymbolTable* tp = symTableStack.back();
    while (tp){
        for (int i = 0; i < tp->table.size(); i++){
            if (tp->table[i].id == id)
                return &(tp->table[i]);
        }
        tp = tp->parent;
    }
    return NULL;
}

void Semantic::emit(string op, string arg1, string arg2, string result) {
    Quaternion t = { op,arg1,arg2,result };
    midList.push_back(t);
}

void Semantic::semanticAnalyse(string token, TreeNode* root, map<int, string> nameTable) {
    // 程序归约结束
    if (token == "<PROGRAM>::=<M><VARS>"){
        // 弹出两栈
        glbTable = symTableStack.back();
        symTableStack.pop_back();
        glbTable->width = offsetStack.back();
        offsetStack.pop_back();
    }

    // 单个程序块结束(更换当前处理的栈表)
    else if (token == "<M>::='"+string(EPSILON)+"'"){
        root->pos = midList.size();

        SymbolTable* t = new(nothrow)SymbolTable;
        if (!t)
            throw string("ERROR: create symbol table failed.\n");
        if (!symTableStack.empty())
            t->parent = symTableStack.back();

        currentSymTable = t;
        symTableStack.push_back(t);
        offsetStack.push_back(0);
    }

    // 全局int型变量声明
    else if (token == "<VAR>::='INT''ID'<VARTYPE>';'"){
        // root->children[2]为<VARTYPE>
        root->t = INT;
        root->k = root->children[2]->k;
        root->n = root->children[2]->n;
        root->width = 4 * root->n;

        symTableStack.back()->insert(root->children[1]->data.second, root->t, root->k, offsetStack.back());

        if (root->k == ARRAY){
            root->dm = root->children[2]->dm;
            symTableStack.back()->insertDM(root->children[1]->data.second, root->dm);
        }

        offsetStack.back() += root->width;
    }

    // 函数块变量声明
    else if (token == "<VAR>::=<FUNC><SENBLOCK>"){
        root->t = root->children[0]->t;
        root->k = root->children[0]->k;
        root->n = root->children[0]->n;
        root->width = root->children[0]->width;

        SymbolTable* t = symTableStack.back();
        symTableStack.pop_back();
        t->width = t->table.empty() ? 0 : offsetStack.back() - t->table[0].offset;
        offsetStack.pop_back();
    }

    // 变量归约结束
    else if (token == ("<VARTYPE>::='"+string(EPSILON)+"'")){
        root->k = VAR;
        root->n = 1;
    }

    // 数组型变量
    else if (token == "<VARTYPE>::=<ARRAYVAR>"){
        root->k = ARRAY;
        root->n = root->children[0]->n;
        root->dm = root->children[0]->dm;
        reverse(root->dm.begin(), root->dm.end());
    }

    // 空型函数块变量
    else if (token == "<FUNC>::='VOID''ID'<M>'('<PARAM>')'"){
        root->t = VOID;
        root->k = FUNC;
        root->n = 1;
        root->width = -1 * root->n;

        SymbolTable* t = symTableStack.back();
        symTableStack.pop_back();
        int tempOffset = offsetStack.back();
        offsetStack.pop_back();

        symTableStack.back()->insert(root->children[1]->data.second, root->t, root->k, root->children[2]->pos);
        offsetStack.back() += 0;
        symTableStack.back()->insertFunc(root->children[1]->data.second, t);
        symTableStack.back()->insertDM(root->children[1]->data.second, root->children[4]->dm);

        symTableStack.push_back(t);
        offsetStack.push_back(tempOffset);
    }

    // 整型函数块变量
    else if (token == "<FUNC>::='INT''ID'<M>'('<PARAM>')'"){
        root->t = INT;
        root->k = FUNC;
        root->n = 1;
        root->width = -1 * root->n;

        SymbolTable* t = symTableStack.back();
        symTableStack.pop_back();
        int tempOffset = offsetStack.back();
        offsetStack.pop_back();

        symTableStack.back()->insert(root->children[1]->data.second, root->t, root->k, root->children[2]->pos);
        offsetStack.back() += 0;
        symTableStack.back()->insertFunc(root->children[1]->data.second, t);
        symTableStack.back()->insertDM(root->children[1]->data.second, root->children[4]->dm);

        symTableStack.push_back(t);
        offsetStack.push_back(tempOffset);
    }

    // 数组(单维度)
    else if (token == "<ARRAYVAR>::='[''NUM'']'"){
        root->k = ARRAY;
        root->n = root->children[1]->data.second;
        root->dm.push_back(root->children[1]->data.second);
    }

    // 数组(多维度)
    else if (token == "<ARRAYVAR>::='[''NUM'']'<ARRAYVAR>"){
        root->k = ARRAY;
        root->n = root->children[1]->data.second * root->children[3]->n;
        root->dm = root->children[3]->dm;
        root->dm.push_back(root->children[1]->data.second);
    }

    // 函数形式参数表
    else if (token == "<PARAM>::=<PARAMLIST>"){
        root->dm = root->children[0]->dm;
    }

    // 函数形式参数空
    else if (token == "<PARAM>::='VOID'" || token == "<PARAM>::='"+string(EPSILON)+"'"){
        root->dm.push_back(0);
    }

    // 函数形式参数整型
    else if (token == "<PARAMLIST>::='INT''ID'"){
        root->t = INT;
        root->k = VAR;
        root->n = 1;
        root->width = 4 * root->n;
        root->dm.push_back(1);

        symTableStack.back()->insert(root->children[1]->data.second, root->t, root->k, offsetStack.back());
        offsetStack.back() += root->width;
    }

    // 函数形式参数多个
    else if (token == "<PARAMLIST>::='INT''ID'','<PARAMLIST>"){
        root->t = INT;
        root->k = VAR;
        root->n = 1;
        root->width = 4 * root->n;
        root->dm = root->children[3]->dm;
        root->dm[0] += 1;

        symTableStack.back()->insert(root->children[1]->data.second, root->t, root->k, offsetStack.back());
        offsetStack.back() += root->width;
    }

    // 整型单个变量的声明
    else if (token == "<INVARDEF>::='INT''ID'"){
        root->t = INT;
        root->k = VAR;
        root->n = 1;
        root->width = 4 * root->n;
        symTableStack.back()->insert(root->children[1]->data.second, root->t, root->k, offsetStack.back());
        offsetStack.back() += root->width;

        emit("+", "$sp", to_string(root->width), "$sp");
    }

    // 整型数组变量的声明
    else if (token == "<INVARDEF>::='INT''ID'<ARRAYVAR>"){
        root->t = INT;
        root->k = ARRAY;
        root->n = root->children[2]->n;
        root->width = 4 * root->n;
        root->dm = root->children[2]->dm;
        reverse(root->dm.begin(), root->dm.end());
        symTableStack.back()->insert(root->children[1]->data.second, root->t, root->k, offsetStack.back());
        symTableStack.back()->insertDM(root->children[1]->data.second, root->dm);
        offsetStack.back() += root->width;

        emit("+", "$sp", to_string(root->width), "$sp");
    }

    // 变量赋值语句
    else if (token == "<ASSIGNMENT>::='ID''ASSIGN'<EXPRESSION>"){
        string p = entry(root->children[0]->data.second);

        if (p == "")
            throw string("ERROR: ") + nameTable[root->children[0]->data.second] + string(" is undefineded.\n");
        else{
            emit(":=", root->children[2]->place, "", p);
            root->place = newtemp();
            emit(":=", root->children[2]->place, "", root->place);
        }
    }

    // 数组赋值语句
    else if (token == "<ASSIGNMENT>::=<ARRAY>'ASSIGN'<EXPRESSION>"){
        if (root->children[0]->dm.size() != 1)
            throw string("ERROR: Array is missing index.\n");

        string p = entry(root->children[0]->data.second);
        if (p == "")
            throw string("ERROR: ") + nameTable[root->children[0]->data.second] + string(" is undefineded.\n");
        else{
            emit("[]=", root->children[2]->place, root->children[0]->place, p);
            root->place = newtemp();
            emit(":=", root->children[2]->place, "", root->place);
        }
    }

    // 返回语句(有返回值)
    else if (token == "<RETSEN>::='RET'<EXPRESSION>"){
        emit(":=", root->children[1]->place, "", "$v0");
        emit("ret", "", "", "");
    }

    // 返回语句(无返回值)
    else if (token == "<RETSEN>::='RET'"){
        emit(":=", to_string(0), "", "$v0");
        emit("ret", "", "", "");
    }

    // 循环语句
    else if (token == "<WHILESEN>::=<B>'WHILE''('<CTRL>')'<T><SENBLOCK>"){
        SymbolTable* t = symTableStack.back();
        symTableStack.pop_back();
        t->width = t->table.empty() ? 0 : offsetStack.back() - t->table[0].offset;
        offsetStack.pop_back();

        emit("-", "$sp", to_string(t->width), "$sp");

        emit("j", "", "", to_string(root->children[0]->pos));
        midList[root->children[3]->TList].result = to_string(root->children[5]->pos);
        midList[root->children[3]->FList].result = to_string(midList.size());
    }

    // 循环语句记录循环位置
    else if (token == "<B>::='"+string(EPSILON)+"'"){
        root->pos = midList.size();
    }

    // 条件判断语句(只有真块)
    else if (token == "<IFSEN>::='IF''('<CTRL>')'<T><SENBLOCK>"){
        SymbolTable* t = symTableStack.back();
        symTableStack.pop_back();
        t->width = t->table.empty() ? 0 : offsetStack.back() - t->table[0].offset;
        offsetStack.pop_back();

        emit("-", "$sp", to_string(t->width), "$sp");

        midList[root->children[2]->TList].result = to_string(root->children[4]->pos);
        midList[root->children[2]->FList].result = to_string(midList.size());
    }

    // 条件判断语句(真假块都有)
    else if (token == "<IFSEN>::='IF''('<CTRL>')'<T><SENBLOCK>'ELSE'<N><SENBLOCK>"){
        SymbolTable* t = symTableStack.back();
        symTableStack.pop_back();
        t->width = t->table.empty() ? 0 : offsetStack.back() - t->table[0].offset;
        offsetStack.pop_back();

        emit("-", "$sp", to_string(t->width), "$sp");

        midList[root->children[2]->TList].result = to_string(root->children[4]->pos);
        midList[root->children[2]->FList].result = to_string(root->children[7]->pos);
        midList[root->children[7]->TList].result = to_string(midList.size());
    }

    // 真假出口
    else if (token == "<CTRL>::=<EXPRESSION>"){
        root->TList = midList.size();
        emit("jnz", root->children[0]->place, "", to_string(0));
        root->FList = midList.size();
        emit("j", "", "", to_string(0));
    }

    // 真归约
    else if (token == "<T>::='"+string(EPSILON)+"'"){
        root->pos = midList.size();
        SymbolTable* t = new(nothrow)SymbolTable;
        if (!t)
            throw string("ERROR: create symbol table failed.\n");
        if (!symTableStack.empty())
            t->parent = symTableStack.back();

        currentSymTable = t;
        symTableStack.push_back(t);
        if (offsetStack.empty())
            offsetStack.push_back(0);
        else{
            int back_offset = offsetStack.back();
            offsetStack.push_back(back_offset);
        }
    }

    // 假归约
    else if (token == "<N>::='"+string(EPSILON)+"'"){
        SymbolTable* t = symTableStack.back();
        symTableStack.pop_back();
        t->width = t->table.empty() ? 0 : offsetStack.back() - t->table[0].offset;
        offsetStack.pop_back();

        emit("-", "$sp", to_string(t->width), "$sp");

        t = new(nothrow)SymbolTable;
        if (!t)
            throw string("ERROR: create symbol table failed.\n");
        if (!symTableStack.empty())
            t->parent = symTableStack.back();
        currentSymTable = t;
        symTableStack.push_back(t);

        if (offsetStack.empty())
            offsetStack.push_back(0);
        else{
            int back_offset = offsetStack.back();
            offsetStack.push_back(back_offset);
        }

        root->TList = midList.size();
        emit("j", "", "", to_string(0));
        root->pos = midList.size();
    }

    // AND表达式
    else if (token == "<EXPRESSION>::=<BAND>"){
        root->place = root->children[0]->place;
    }

    // AND表达式 or 多表达式
    else if (token == "<EXPRESSION>::=<BAND>'OR'<EXPRESSION>"){
        root->place = newtemp();
        emit("jnz", root->children[0]->place, "", to_string(midList.size() + 4));
        emit("jnz", root->children[2]->place, "", to_string(midList.size() + 3));
        emit(":=", to_string(0), "", root->place);
        emit("j", "", "", to_string(midList.size() + 2));
        emit(":=", to_string(1), "", root->place);
    }

    // NOT表达式
    else if (token == "<BAND>::=<BNOT>"){
        root->place = root->children[0]->place;
    }

    // NOT表达式 and AND表达式
    else if (token == "<BAND>::=<BNOT>'AND'<BAND>"){
        root->place = newtemp();
        emit("jnz", root->children[0]->place, "", to_string(midList.size() + 2));
        emit("j", "", "", to_string(midList.size() + 2));
        emit("jnz", root->children[2]->place, "", to_string(midList.size() + 3));
        emit(":=", to_string(0), "", root->place);
        emit("j", "", "", to_string(midList.size() + 2));
        emit(":=", to_string(1), "", root->place);
    }

    // 低级表达式
    else if (token == "<BNOT>::=<COMP>"){
        root->place = root->children[0]->place;
    }

    // not 低级表达式
    else if (token == "<BNOT>::='NOT'<COMP>"){
        root->place = newtemp();
        emit("jnz", root->children[1]->place, "", to_string(midList.size() + 3));
        emit(":=", to_string(1), "", root->place);
        emit("j", "", "", to_string(midList.size() + 2));
        emit(":=", to_string(0), "", root->place);
    }

    // 低级表达式
    else if (token == "<COMP>::=<PLUSEX>"){
        root->place = root->children[0]->place;
    }

    // 比较表达式
    else if (token == "<COMP>::=<PLUSEX>'CMP'<COMP>"){
        root->place = newtemp();
        switch (root->children[1]->data.second){
        case 0:
            emit("j<", root->children[0]->place, root->children[2]->place, to_string(midList.size() + 3)); break;
        case 1:
            emit("j<=", root->children[0]->place, root->children[2]->place, to_string(midList.size() + 3)); break;
        case 2:
            emit("j>", root->children[0]->place, root->children[2]->place, to_string(midList.size() + 3)); break;
        case 3:
            emit("j>=", root->children[0]->place, root->children[2]->place, to_string(midList.size() + 3)); break;
        case 4:
            emit("j==", root->children[0]->place, root->children[2]->place, to_string(midList.size() + 3)); break;
        case 5:
            emit("j!=", root->children[0]->place, root->children[2]->place, to_string(midList.size() + 3)); break;
        }
        emit(":=", to_string(0), "", root->place);
        emit("j", "", "", to_string(midList.size() + 2));
        emit(":=", to_string(1), "", root->place);
    }

    // 运算归约
    else if (token == "<PLUSEX>::=<TERM>"){
        root->place = root->children[0]->place;
    }

    // 运算
    else if (token == "<PLUSEX>::=<TERM>'OP1'<PLUSEX>"){
        root->place = newtemp();
        switch (root->children[1]->data.second){
        case 0:
            emit("+", root->children[0]->place, root->children[2]->place, root->place); break;
        case 1:
            emit("-", root->children[0]->place, root->children[2]->place, root->place); break;
        case 2:
            emit("&", root->children[0]->place, root->children[2]->place, root->place); break;
        case 3:
            emit("|", root->children[0]->place, root->children[2]->place, root->place); break;
        case 4:
            emit("^", root->children[0]->place, root->children[2]->place, root->place); break;
        }
    }

    // 单项归约
    else if (token == "<TERM>::=<FACTOR>"){
        root->place = root->children[0]->place;
    }

    // 运算单项
    else if (token == "<TERM>::=<FACTOR>'OP2'<TERM>"){
        root->place = newtemp();
        switch (root->children[1]->data.second){
        case 0:
            emit("*", root->children[0]->place, root->children[2]->place, root->place); break;
        case 1:
            emit("/", root->children[0]->place, root->children[2]->place, root->place); break;
        }
    }

    // 数字归约
    else if (token == "<FACTOR>::='NUM'"){
        root->place = newtemp();
        emit(":=", to_string(root->children[0]->data.second), "", root->place);
    }

    // 表达式归约
    else if (token == "<FACTOR>::='('<EXPRESSION>')'"){
        root->place = root->children[1]->place;
    }

    // ID符号归约
    else if (token == "<FACTOR>::='ID'"){
        string p = entry(root->children[0]->data.second);
        if (p == "")
            throw string("ERROR: ") + nameTable[root->children[0]->data.second] + string(" is undefineded.\n");
        else
            root->place = p;
    }

    // 数组索引
    else if (token == "<FACTOR>::=<ARRAY>"){
        if (root->children[0]->dm.size() != 1)
            throw string("ERROR: Array is missing index.\n");

        string p = entry(root->children[0]->data.second);
        if (p == "")
            throw string("ERROR: ") + nameTable[root->children[0]->data.second] + string(" is undefineded.\n");
        else{
            root->place = newtemp();
            emit("=[]", p, root->children[0]->place, root->place);
        }
    }

    // 过程调用
    else if (token == "<FACTOR>::='ID'<CALL>"){
        Symbol* f = find(root->children[0]->data.second);

        if (f == NULL)
            throw string("ERROR: ") + nameTable[root->children[0]->data.second] + string(" is undefineded.\n");
        if (f->dm[0] != root->children[1]->params.size())
            throw string("ERROR: ") + nameTable[f->id] + string(" is missing params.\n");

        emit(":=", "$ra", "", "[$sp]");
        emit("+", "$sp", to_string(4), "$sp");
        emit(":=", "$t0", "", "[$sp]");
        emit("+", "$sp", to_string(4), "$sp");
        emit(":=", "$t1", "", "[$sp]");
        emit("+", "$sp", to_string(4), "$sp");
        emit(":=", "$t2", "", "[$sp]");
        emit("+", "$sp", to_string(4), "$sp");
        emit(":=", "$t3", "", "[$sp]");
        emit("+", "$sp", to_string(4), "$sp");
        emit(":=", "$t4", "", "[$sp]");
        emit("+", "$sp", to_string(4), "$sp");
        emit(":=", "$t5", "", "[$sp]");
        emit("+", "$sp", to_string(4), "$sp");
        emit(":=", "$t6", "", "[$sp]");
        emit("+", "$sp", to_string(4), "$sp");
        emit(":=", "$t7", "", "[$sp]");
        emit("+", "$sp", to_string(4), "$sp");
        emit(":=", "$sp", "", "$s0");
        emit(":=", "$fp", "", "[$sp]");
        emit("+", "$sp", to_string(4), "$sp");
        emit(":=", "$s0", "", "$fp");

        for (auto i = 0; i < root->children[1]->params.size(); i++){
            emit(":=", root->children[1]->params[i], "", "[$sp]");
            emit("+", "$sp", to_string(4), "$sp");
        }
        emit("jal", "", "", to_string(f->offset));
        emit(":=", "$fp", "", "$sp");
        emit(":=", "[$sp]", "", "$fp");
        emit("-", "$sp", to_string(4), "$sp");
        emit(":=", "[$sp]", "", "$t7");
        emit("-", "$sp", to_string(4), "$sp");
        emit(":=", "[$sp]", "", "$t6");
        emit("-", "$sp", to_string(4), "$sp");
        emit(":=", "[$sp]", "", "$t5");
        emit("-", "$sp", to_string(4), "$sp");
        emit(":=", "[$sp]", "", "$t4");
        emit("-", "$sp", to_string(4), "$sp");
        emit(":=", "[$sp]", "", "$t3");
        emit("-", "$sp", to_string(4), "$sp");
        emit(":=", "[$sp]", "", "$t2");
        emit("-", "$sp", to_string(4), "$sp");
        emit(":=", "[$sp]", "", "$t1");
        emit("-", "$sp", to_string(4), "$sp");
        emit(":=", "[$sp]", "", "$t0");
        emit("-", "$sp", to_string(4), "$sp");
        emit(":=", "[$sp]", "", "$ra");

        root->place = newtemp();
        emit(":=", "$v0", "", root->place);
    }

    // 赋值语句归约
    else if (token == "<FACTOR>::='('<ASSIGNMENT>')'"){
        root->place = root->children[1]->place;
    }

    // 过程调用实参
    else if (token == "<CALL>::='('<ACTUALPARAM>')'"){
        root->params = root->children[1]->params;
    }

    // 数组变量索引
    else if (token == "<ARRAY>::='ID''['<EXPRESSION>']'"){
        Symbol* e = find(root->children[0]->data.second);
        if (e == NULL)
            throw string("ERROR: ") + nameTable[root->children[0]->data.second] + string(" is undefineded.\n");

        root->data = root->children[0]->data;
        root->k = ARRAY;
        root->dm = e->dm;

        if (root->dm.size() == 0)
            throw string("ERROR: Array's index is wrong.");
        else if (root->dm.size() == 1)
            root->place = root->children[2]->place;
        else{
            int dim_len = root->dm[1];
            for (int i = 2; i < root->dm.size(); i++)
                dim_len *= root->dm[i];
            string p = newtemp();
            emit(":=", to_string(dim_len), "", p);
            root->place = newtemp();
            emit("*", p, root->children[2]->place, root->place);
        }
    }

    // 数组变量索引(多维)
    else if (token == "<ARRAY>::=<ARRAY>'['<EXPRESSION>']'"){
        root->data = root->children[0]->data;
        root->k = ARRAY;
        root->dm = root->children[0]->dm;
        root->dm.erase(root->dm.begin());

        if (root->dm.size() == 0)
            throw string("ERROR: Array's index is wrong.");
        else if (root->dm.size() == 1){
            root->place = newtemp();
            emit("+", root->children[0]->place, root->children[2]->place, root->place);
        }
        else{
            int dim_len = root->dm[1];
            for (int i = 2; i < root->dm.size(); i++)
                dim_len *= root->dm[i];
            string p1 = newtemp();
            emit(":=", to_string(dim_len), "", p1);
            string p2 = newtemp();
            emit("*", p1, root->children[2]->place, p2);
            root->place = newtemp();
            emit("+", root->children[0]->place, p2, root->place);
        }
    }

    // 过程调用实参列表
    else if (token == "<ACTUALPARAM>::=<ACTUALPARAMLIST>"){
        root->params = root->children[0]->params;
    }

    // 过程调用实参归约
    else if (token == "<ACTUALPARAMLIST>::=<EXPRESSION>"){
        root->params.push_back(root->children[0]->place);
    }

    // 过程调用多实参归约
    else if (token == "<ACTUALPARAMLIST>::=<EXPRESSION>','<ACTUALPARAMLIST>"){
        root->params = root->children[2]->params;
        root->params.push_back(root->children[0]->place);
    }

    // 无作为的产生式
    else if (token == "<VARS>::=<VAR>") {}
    else if (token == "<VARS>::=<VAR><VARS>") {}
    else if (token == "<SENBLOCK>::='{'<INVARDEFS><SENSEQ>'}'") {}
    else if (token == "<INVARDEFS>::=<INVARDEF>';'<INVARDEFS>") {}
    else if (token == "<INVARDEFS>::='"+string(EPSILON)+"'") {}
    else if (token == "<SENSEQ>::=<SENTENCE>") {}
    else if (token == "<SENSEQ>::=<SENTENCE><SENSEQ>") {}
    else if (token == "<SENTENCE>::=<IFSEN>") {}
    else if (token == "<SENTENCE>::=<WHILESEN>") {}
    else if (token == "<SENTENCE>::=<RETSEN>';'") {}
    else if (token == "<SENTENCE>::=<ASSIGNMENT>';'") {}
    else if (token == "<ACTUALPARAM>::='"+string(EPSILON)+"'"){}

    // 出错
    else{
        throw string("ERROR: Semantic can not find ") + token + string(" 's sub-program.");
    }
}
