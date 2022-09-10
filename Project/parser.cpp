#include "parser.h"

Parser::Parser(){
    line = 1;
    strStack.clear();
    strState.clear();
    strSymbol.clear();
    stackTable.clear();
}

Parser::~Parser(){
    line = 1;
    strStack.clear();
    strState.clear();
    strSymbol.clear();
    stackTable.clear();
}

// 初始化LR1表
void Parser::initLR1(){
    G.initGrammar();
    G.initFirst();
    G.initCollection();
    G.initLR1Table();
}

// 获得输入字符
void Parser::parserOpen(string input){
    L.lexerOpen(input);
}

// 语法分析
void Parser::parserAnalyse(){
    /// 变量初始化
    line = 1;
    vector<int> state;
    vector<pair<string, int>> symbol;
    pair<string, int> nextToken;
    pair<string, int> token = L.lexerAnalyse();
    pair<string, int> bubble = pair<string, int>(EPSILON, -1);
    TreeNode* tp;
    stack<TreeNode*> treeNodeStack;

    /// 初始压栈
    state.push_back(0);
    symbol.push_back(pair<string, int>(string("#"), -1));

    /// 循环语法分析
    while (token.first != "ERROR"){
        //换行重读
        if (token.first == "NL"){
            line++;
            token = L.lexerAnalyse();
            continue;
        }
        //出错终止
        if (find(G.VT.begin(), G.VT.end(), token.first) == G.VT.end()){
            string msg = string("ERROR: ") + token.first + string(" is not a terminator.\n");
            throw msg;
        }

        //动作状态机
        Action item = G.ACTION[state.back()][find(G.VT.begin(), G.VT.end(), token.first) - G.VT.begin()];
        switch (item.s){
        case ACT_DONE:  //成功
            return;
            break;

        case ACT_ERROR: //出错
            if (token != bubble){
                nextToken = token;
                token = bubble;
            }
            else{
                string msg;
                msg = string("ERROR: Parser detected error on line ") + to_string(line) + string(" (") + token.first + string(").\n");
                throw msg;
            }
            break;

        case ACT_NEXT:  //移进
            tp = new(nothrow)TreeNode;
            if (!tp) {
                string msg;
                msg = string("ERROR: Memory crashed on line ") + to_string(line) + string(" (") + token.first + string(").\n");
                throw msg;
            }
            tp->data = token;
            treeNodeStack.push(tp);

            //压栈
            state.push_back(item.nextState);
            symbol.push_back(token);

            if (token == bubble)
                token = nextToken;
            else
                token = L.lexerAnalyse();
            break;

        case ACT_REDUCE:  //归约
            for (int i = item.p.second.size() - 1; i >= 0; i--){
                if (symbol.back().first == item.p.second[i]){
                    symbol.pop_back();
                    state.pop_back();
                }
                else{
                    string msg;
                    msg = string("ERROR: Parser detected error on line ") + to_string(line) + string(" (") + token.first + string(").\n");
                    msg += string("-Note：") + symbol.back().first + string(" is different from ") + item.p.second[i] + string(".\n");
                    throw msg;
                }
            }

            ///////////////////////////////////////////////
            /// 语义分析
            string token_t = "<" + item.p.first + ">::=";
            for (int i = 0; i < item.p.second.size(); i++) {
                if (find(G.VN.begin(), G.VN.end(), item.p.second[i]) != G.VN.end())
                    token_t += "<" + item.p.second[i] + ">";
                else
                    token_t += "'" + item.p.second[i] + "'";
            }

            tp = new(nothrow)TreeNode;
            if (!tp) {
                string msg;
                msg = string("ERROR: Memory crashed on line ") + to_string(line) + string(" (") + token.first + string(").\n");
                throw msg;
            }
            tp->data = pair<string, int>(item.p.first, -1);
            for (int i = 0; i < item.p.second.size(); i++){
                treeNodeStack.top()->parent = tp;
                tp->children.push_back(treeNodeStack.top());
                treeNodeStack.pop();
            }
            reverse(tp->children.begin(), tp->children.end());
            treeNodeStack.push(tp);

            try {
                S.semanticAnalyse(token_t, tp, L.nameTable);
            }
            catch (string msg) {
                msg = string("ERROR: SemanticAnalyse detected error on line ") + to_string(line) + string(" (") + token.first + string(").\n")+msg;
                throw msg;
            }
            ///////////////////////////////////////////////

            symbol.push_back(pair<string, int>(item.p.first, -1));
            state.push_back(G.GOTO[state.back()][find(G.VN.begin(), G.VN.end(), symbol.back().first) - G.VN.begin()]);
            break;
        }

        //保存这一步
        strState.clear();
        strSymbol.clear();
        strStack.clear();
        for (int i = 0; i < state.size(); i++)
            strState.push_back(to_string(state[i]));
        for (int i = 0; i < symbol.size(); i++)
            strSymbol.push_back(symbol[i].first);
        strStack.push_back(token.first);

        stackTable.push_back({ strState, strSymbol, strStack });
    };
}
