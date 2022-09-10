#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ///界面UI设置
    ui->setupUi(this);
    this->resize(2000, 1200);

    ///Qsci代码编辑器
    codeEditor=new QsciScintilla(this);
    initEditor(codeEditor);
    auto editorLayout = new QVBoxLayout(ui->widEditor);
    editorLayout->addWidget(codeEditor);
    editorLayout->setContentsMargins(0,0,0,0);

    /// 表宽设置
    ui->wordTable->setColumnWidth(0,200);
    ui->wordTable->setColumnWidth(1,150);
    ui->wordTable->setColumnWidth(2,500);
    ui->nameTable->setColumnWidth(0,150);
    ui->nameTable->setColumnWidth(1,500);
    ui->firstTable->setColumnWidth(0,300);
    ui->firstTable->setColumnWidth(1,500);
    ui->stackTable->setColumnWidth(0,300);
    ui->stackTable->setColumnWidth(1,150);
    ui->stackTable->setColumnWidth(2,400);
    ui->midTable->setColumnWidth(0,150);
    ui->midTable->setColumnWidth(1,200);
    ui->midTable->setColumnWidth(2,200);
    ui->midTable->setColumnWidth(3,250);
    ui->optTable->setColumnWidth(0,150);
    ui->optTable->setColumnWidth(1,200);
    ui->optTable->setColumnWidth(2,200);
    ui->optTable->setColumnWidth(3,250);
    ui->actTable->setColumnWidth(0,250);
    ui->actTable->setColumnWidth(1,150);
    ui->actTable->setColumnWidth(2,150);
    ui->actTable->setColumnWidth(3,150);
    ui->regTable->setColumnWidth(0,250);
    ui->regTable->setColumnWidth(1,200);
    ui->regTable->setColumnWidth(2,150);
    ui->regTable->setColumnWidth(3,150);

    ///信号连接
    connect(codeEditor,SIGNAL(textChanged()),this,SLOT(codeEdited()));
    connect(ui->actQuit,SIGNAL(triggered()),this,SLOT(close()));

    ///标题更新
    updateApp();
}

MainWindow::~MainWindow(){
    delete ui;
}

void MainWindow::initEditor(QsciScintilla *editor){
    //设置词法分析器
    QsciLexerCPP *textLexer = new QsciLexerCPP;
    textLexer->setColor(QColor(105,105,105),QsciLexerCPP::Default);
    textLexer->setColor(QColor(34,139,34),QsciLexerCPP::Comment);
    textLexer->setColor(QColor(0,139,139),QsciLexerCPP::Number);
    textLexer->setColor(QColor(255,99,71),QsciLexerCPP::DoubleQuotedString);
    textLexer->setColor(QColor(255,99,71),QsciLexerCPP::SingleQuotedString);
    textLexer->setColor(QColor(25,25,112),QsciLexerCPP::Keyword);
    textLexer->setColor(QColor(139,0,139),QsciLexerCPP::Identifier);
    editor->setLexer(textLexer);

    //设置光标所在行背景色
    editor->setCaretLineBackgroundColor(QColor(232,232,232));

    //自动折叠区域
    editor->setFolding(QsciScintilla::BoxedTreeFoldStyle);
    editor->setMarginType(2, QsciScintilla::SymbolMargin);
    editor->setFoldMarginColors(QColor(181,181,181),QColor(232,232,232));

    //行号显示
    editor->setMarginLineNumbers(0,true);
    editor->setMarginWidth(0,40);
    editor->setMarginSensitivity(2, true);

    //自动补全
    QsciAPIs *apis = new QsciAPIs(textLexer);
    apis->add(QString("int"));
    apis->add(QString("void"));
    apis->add(QString("if"));
    apis->add(QString("else"));
    apis->add(QString("while"));
    apis->add(QString("return"));
    apis->prepare();
    editor->setAutoIndent(true);
    editor->setCaretLineVisible(true);
    editor->setAutoCompletionSource(QsciScintilla::AcsDocument);
    editor->setAutoCompletionCaseSensitivity(true);
    editor->setAutoCompletionThreshold(2);
    editor->setFont(QFont("Courier New"));
    editor->SendScintilla(QsciScintilla::SCI_SETCODEPAGE,QsciScintilla::SC_CP_UTF8);

    return;
}

void MainWindow::initTable(){
    ///清空表格
    ui->wordTable->clearContents();
    ui->nameTable->clearContents();
    ui->firstTable->clearContents();
    ui->LRTable->clearContents();
    ui->stackTable->clearContents();

    ///读取数据
    QString str=codeEditor->text();
    string fstring = string(str.toLocal8Bit());
    ui->Message->append("///////////////////////////////////////////////////////");
    ui->Message->append("【开始分析】\n");

    ///开始分析
    Parser parser;              //语法分析器
    Optimizer optimizer;        //代码优化器
    Generator MIPSgenerator;    //汇编生成器

    try {
        parser.initLR1();
        ui->Message->append("-语法读取成功\n");

        parser.parserOpen(fstring);
        ui->Message->append("-文件录入成功\n");

        parser.parserAnalyse();
        ui->Message->append("-语法及语义分析成功\n");

        optimizer.init(parser.L.nameTable, parser.S.glbTable, parser.S.midList);
        ui->Message->append("-中间代码生成成功\n");

        double optRate = optimizer.optimizerAnalyse();
        ui->Message->append("-中间代码优化成功，优化率:"+QString::number(optRate, 'f', 2)+"%\n");

        MIPSgenerator.init(optimizer.interCode, optimizer.blockGroup,stackSize);
        MIPSgenerator.genObjCode();
        ui->Message->append("-汇编代码生成成功\n");

        string tasFileName = editName.substr(0,editName.find_last_of(".")) + "_ori.txt";
        fstream fout1(tasFileName, ios::out);
        for(int i=0;i < parser.S.midList.size(); i++){
            string temp = "("+parser.S.midList[i].op+", "+parser.S.midList[i].arg1+", "+parser.S.midList[i].arg2+", "+parser.S.midList[i].result+")";
            fout1 << temp << endl;
        }
        fout1.close();
        string tasOut = "-中间代码保存成功, 文件:"+tasFileName+"\n";
        ui->Message->append(tasOut.data());

        string optFileName = editName.substr(0,editName.find_last_of(".")) + "_opt.txt";
        fstream fout2(optFileName, ios::out);
        for(int i=0;i < optimizer.interCode.size();i++){
            string temp = "("+optimizer.interCode[i].op+", "+optimizer.interCode[i].arg1+", "+optimizer.interCode[i].arg2+", "+optimizer.interCode[i].result+")";
            fout2 << temp << endl;
        }
        fout2.close();
        string optOut = "-优化代码保存成功, 文件:"+optFileName+"\n";
        ui->Message->append(optOut.data());

        string objFileName = editName.substr(0,editName.find_last_of(".")) + ".s";
        fstream fout3(objFileName, ios::out);
        for (int i = 0; i < MIPSgenerator.objCode.size(); i++)
            fout3 << MIPSgenerator.objCode[i] << endl;
        fout3.close();
        string objOut = "-汇编代码保存成功, 文件:"+objFileName+"\n";
        ui->Message->append(objOut.data());

        ui->Message->append("【编译结束】");
        ui->Message->append("///////////////////////////////////////////////////////\n");
    } catch (string expmsg) {
        QString Qexpmsg = QString::fromLocal8Bit(expmsg.data());
        ui->Message->append(Qexpmsg);
        ui->Message->append("【编译失败】");
        ui->Message->append("///////////////////////////////////////////////////////\n");
    }

    ///绘制表格
    // 1-单词表
    ui->wordTable->clearContents();
    ui->wordTable->setRowCount(parser.L.wordTable.size());
    for (int i = 0; i < parser.L.wordTable.size();i++){
        ui->wordTable->setItem(i,0,new QTableWidgetItem(("$"+parser.L.wordTable[i].second.first).data()));
        ui->wordTable->setItem(i,1,new QTableWidgetItem(to_string(parser.L.wordTable[i].second.second).data()));
        ui->wordTable->setItem(i,2,new QTableWidgetItem(parser.L.wordTable[i].first.data()));
    }

    // 2-符号表
    ui->nameTable->clearContents();
    ui->nameTable->setRowCount(parser.L.nameTable.size());
    int name_table_count = 0;
    for (map<int, string>::iterator iter = parser.L.nameTable.begin(); iter != parser.L.nameTable.end(); iter++){
        ui->nameTable->setItem(name_table_count,0,new QTableWidgetItem(to_string(iter->first).data()));
        ui->nameTable->setItem(name_table_count,1,new QTableWidgetItem(iter->second.data()));
        name_table_count++;
    }

    // 3-FIRST表
    ui->firstTable->clearContents();
    ui->firstTable->setRowCount(parser.G.FIRST.size());
    int firstCnt = 0;
    int secondCnt = 0;
    string secondStr = "";
    for (map<string, vector<string>>::iterator p = parser.G.FIRST.begin(); p != parser.G.FIRST.end(); p++){
        ui->firstTable->setItem(firstCnt,0,new QTableWidgetItem(("First["+p->first+"]").data()));
        for (secondCnt = 0; secondCnt < (p->second.size()-1); secondCnt++)
            secondStr = secondStr + p->second[secondCnt] + ", ";
        secondStr = secondStr + p->second[secondCnt];
        ui->firstTable->setItem(firstCnt,1,new QTableWidgetItem(secondStr.data()));
        secondStr.clear();
        firstCnt++;
    }

    // 4-LR1表
    ui->LRTable->clearContents();
    ui->LRTable->setRowCount(parser.G.ACTION.size());
    ui->LRTable->setColumnCount(parser.G.VN.size()+parser.G.VT.size());
    QStringList headers;
    for (int i=0;i<parser.G.VT.size();i++)
        headers << QString::fromLocal8Bit(parser.G.VT[i].data());
    for (int i=0;i<parser.G.VN.size();i++)
        headers << QString::fromLocal8Bit(parser.G.VN[i].data());
    ui->LRTable->setHorizontalHeaderLabels(headers);
    for (int i=0;i<parser.G.ACTION.size();i++){
        for (int j=0;j<parser.G.ACTION[i].size();j++){
            switch (parser.G.ACTION[i][j].s){

            case ACT_ERROR:
                break;

            case ACT_DONE:
                ui->LRTable->setItem(i,j,new QTableWidgetItem("ACC"));
                break;

            case ACT_NEXT:
                ui->LRTable->setItem(i,j,new QTableWidgetItem(("s"+to_string(parser.G.ACTION[i][j].nextState)).data()));
                break;

            case ACT_REDUCE:
                string context;
                context="r:"+parser.G.ACTION[i][j].p.first+"->";
                for (int k = 0; k < parser.G.ACTION[i][j].p.second.size(); k++)
                    context+=parser.G.ACTION[i][j].p.second[k]+" ";
                ui->LRTable->setItem(i,j,new QTableWidgetItem(context.data()));
                break;
            }
        }

        for (int j=0;j<parser.G.GOTO[i].size();j++){
            if(parser.G.GOTO[i][j]!=-1)
                ui->LRTable->setItem(i,j+parser.G.ACTION[i].size(),new QTableWidgetItem(to_string(parser.G.GOTO[i][j]).data()));
        }
    }

    // 5-语法分析过程
    ui->stackTable->clearContents();
    ui->stackTable->setRowCount(parser.stackTable.size());
    for (int i=0;i<parser.stackTable.size();i++){

        string context1;
        if(parser.stackTable[i][0].size()==0)
            context1 = "";
        else if(parser.stackTable[i][0].size()==1)
            context1 = parser.stackTable[i][0][0];
        else if(parser.stackTable[i][0].size()==2)
            context1 = parser.stackTable[i][0][0]+" "+parser.stackTable[i][0][1];
        else if(parser.stackTable[i][0].size()==3)
            context1 = parser.stackTable[i][0][0]+" "+parser.stackTable[i][0][1]+" "+parser.stackTable[i][0][2];
        else
            context1 = "..." + parser.stackTable[i][0][parser.stackTable[i][0].size()-3]
                    + " " + parser.stackTable[i][0][parser.stackTable[i][0].size()-2]
                    + " " + parser.stackTable[i][0][parser.stackTable[i][0].size()-1];
        ui->stackTable->setItem(i,2,new QTableWidgetItem(context1.data()));

        string context2;
        if(parser.stackTable[i][1].size()==0)
            context2="";
        else if(parser.stackTable[i][1].size()==1)
            context2=parser.stackTable[i][1][0];
        else if(parser.stackTable[i][1].size()==2)
            context2=parser.stackTable[i][1][0]+" "+parser.stackTable[i][1][1];
        else if(parser.stackTable[i][1].size()==3)
            context2=parser.stackTable[i][1][0]+" "+parser.stackTable[i][1][1]+" "+parser.stackTable[i][1][2];
        else
            context2="..."+parser.stackTable[i][1][parser.stackTable[i][1].size()-3]
                    +" "+parser.stackTable[i][1][parser.stackTable[i][1].size()-2]
                    +" "+parser.stackTable[i][1][parser.stackTable[i][1].size()-1];
        ui->stackTable->setItem(i,0,new QTableWidgetItem(context2.data()));

        if(parser.stackTable[i][2][0]=="#")
            ui->stackTable->setItem(i,1,new QTableWidgetItem((parser.stackTable[i][2][0]).data()));
        else
            ui->stackTable->setItem(i,1,new QTableWidgetItem((parser.stackTable[i][2][0]+"...").data()));
    }

    // 6-中间代码
    ui->midTable->clearContents();
    ui->midTable->setRowCount(parser.S.midList.size());
    for(int i=0;i<parser.S.midList.size();i++){
        ui->midTable->setItem(i,0,new QTableWidgetItem(parser.S.midList[i].op.data()));
        ui->midTable->setItem(i,1,new QTableWidgetItem(parser.S.midList[i].arg1.data()));
        ui->midTable->setItem(i,2,new QTableWidgetItem(parser.S.midList[i].arg2.data()));
        ui->midTable->setItem(i,3,new QTableWidgetItem(parser.S.midList[i].result.data()));
    }

    // 7-优化后的中间代码
    ui->optTable->clearContents();
    ui->optTable->setRowCount(optimizer.interCode.size());
    for(int i=0;i < optimizer.interCode.size();i++){
        ui->optTable->setItem(i,0,new QTableWidgetItem(optimizer.interCode[i].op.data()));
        ui->optTable->setItem(i,1,new QTableWidgetItem(optimizer.interCode[i].arg1.data()));
        ui->optTable->setItem(i,2,new QTableWidgetItem(optimizer.interCode[i].arg2.data()));
        ui->optTable->setItem(i,3,new QTableWidgetItem(optimizer.interCode[i].result.data()));
    }

    // 8-待用活跃表
    ui->actTable->clearContents();
    ui->actTable->setRowCount(MIPSgenerator.msgTabHis.size());
    for (int tno = 0; tno < MIPSgenerator.msgTabHis.size(); tno++) {
        MsgTableItem msgTable = MIPSgenerator.msgTabHis[tno];

        string value = "";
        value = "("+msgTable.TAS.op+", "+msgTable.TAS.arg1+", "+msgTable.TAS.arg2+", "+msgTable.TAS.result+")";
        ui->actTable->setItem(tno,0,new QTableWidgetItem(value.data()));

        string leftValue = "";
        if(msgTable.result.second) {
            if(msgTable.result.first==INT_MAX)
                leftValue=string("(^,y)");
            else
                leftValue=string("("+to_string(msgTable.result.first)+",y)");
        }
        else
            leftValue=string("(^,^)");
        ui->actTable->setItem(tno,1,new QTableWidgetItem(leftValue.data()));

        string leftArg = "";
        if(msgTable.arg1.second){
            if(msgTable.arg1.first==INT_MAX)
                leftArg=string("(^,y)");
            else
                leftArg=string("("+to_string(msgTable.arg1.first)+",y)");
        }
        else
            leftArg=string("(^,^)");
        ui->actTable->setItem(tno,2,new QTableWidgetItem(leftArg.data()));

        string rightArg="";
        if(msgTable.arg2.second){
            if(msgTable.arg2.first==INT_MAX)
                rightArg=string("(^,y)");
            else
                rightArg=string("("+to_string(msgTable.arg2.first)+",y)");
        }
        else
            rightArg=string("(^,^)");
        ui->actTable->setItem(tno,3,new QTableWidgetItem(rightArg.data()));
    }

    // 9-寄存器分配表
    ui->regTable->clearContents();
    ui->regTable->setRowCount(MIPSgenerator.aysHis.size());
    for (auto i=0;i<MIPSgenerator.aysHis.size();i++) {
        string value = "";
        value = "("+MIPSgenerator.aysHis[i].TAS.op+", "+MIPSgenerator.aysHis[i].TAS.arg1+", "+MIPSgenerator.aysHis[i].TAS.arg2+", "+MIPSgenerator.aysHis[i].TAS.result+")";
        ui->regTable->setItem(i,0,new QTableWidgetItem(value.data()));

        string object_codes="";
        for (auto j = 0; j < MIPSgenerator.aysHis[i].objCode.size(); j++)
            object_codes+=MIPSgenerator.aysHis[i].objCode[j]+(j==MIPSgenerator.aysHis[i].objCode.size()-1? "":"\n");
        ui->regTable->setItem(i,1,new QTableWidgetItem(object_codes.data()));

        string RVALUE="";
        for (map<string, vector<pair<string, int>>>::iterator iter = MIPSgenerator.aysHis[i].RVALUE.begin(); iter != MIPSgenerator.aysHis[i].RVALUE.end(); iter++){
            RVALUE+=iter->first+": ";
            for (auto k = 0; k < iter->second.size(); k++)
                RVALUE+=iter->second[k].first+" ";
            map<string, vector<pair<string, int>>>::iterator olditer=iter;
            iter++;
            RVALUE+=(iter==MIPSgenerator.aysHis[i].RVALUE.end()? "":"\n");
            iter=olditer;
        }
        ui->regTable->setItem(i,2,new QTableWidgetItem(RVALUE.data()));

        string AVALUE="";
        for (map<string, vector<string>>::iterator iter = MIPSgenerator.aysHis[i].AVALUE.begin(); iter != MIPSgenerator.aysHis[i].AVALUE.end(); iter++){
            AVALUE+=iter->first+": ";
            for (auto k = 0; k < iter->second.size(); k++)
                AVALUE+=iter->second[k]+" ";
            map<string, vector<string>>::iterator olditer=iter;
            iter++;
            AVALUE+=(iter==MIPSgenerator.aysHis[i].AVALUE.end()? "":"\n");
            iter=olditer;
        }
        ui->regTable->setItem(i,3,new QTableWidgetItem(AVALUE.data()));
    }

    // 10-汇编程序
    ui->objText->clear();
    for (int i = 0; i < MIPSgenerator.objCode.size(); i++)
        ui->objText->append(QString::fromLocal8Bit(MIPSgenerator.objCode[i].data()));

    return;
}

void MainWindow::on_actOpen_triggered(){
    if(!saved){
        QMessageBox::StandardButton msgBox;
        msgBox=QMessageBox::question(this, "提示", "文件未保存，是否保存文件？",
                                     QMessageBox::Yes|QMessageBox::No |QMessageBox::Cancel,QMessageBox::Yes);
        if (msgBox==QMessageBox::Yes){
            if(!on_actSave_triggered())
                return;
        }
        else if(msgBox==QMessageBox::No){}
        else
            return;
    }

    QString tempFile = QFileDialog::getOpenFileName(this, tr("打开文件"), "./", tr("源文件(*.c *.cpp);;txt文件(*.txt)"));
    if (tempFile.isEmpty())
        return;

    editName = string(tempFile.toLocal8Bit());
    fstream f(editName, ios::in);
    stringstream str;
    string fstring;
    str << f.rdbuf();
    fstring = str.str();
    f.close();

    QString editString = QString::fromLocal8Bit(fstring.data());
    codeEditor->setText(editString);
    saved = true;

    updateApp();
}

void MainWindow::on_actNew_triggered(){
    if(!saved){
        QMessageBox::StandardButton msgBox;
        msgBox=QMessageBox::question(this, "提示", "文件未保存，是否保存文件？",
                                     QMessageBox::Yes|QMessageBox::No |QMessageBox::Cancel,QMessageBox::Yes);
        if (msgBox==QMessageBox::Yes){
            if(!on_actSave_triggered())
                return;
        }
        else if(msgBox==QMessageBox::No){}
        else
            return;
    }

    editName=fileName;
    codeEditor->clear();
    saved = true;

    updateApp();
}

bool MainWindow::on_actSave_triggered(){
    QString str=codeEditor->text();
    string fstring = string(str.toLocal8Bit());

    if(editName==fileName){
        if(!on_actSaveas_triggered())
            return false;
    }
    else{
        fstream fout(editName,ios::out);
        fout<<fstring;
        fout.close();
    }

    saved = true;
    updateApp();

    return true;
}

bool MainWindow::on_actSaveas_triggered(){
    QString str=codeEditor->text();
    string fstring = string(str.toLocal8Bit());

    QString newFile=QFileDialog::getSaveFileName(this,"保存文件","./","类C语言文件(.c)");
    if (newFile.isEmpty())
        return false;
    fstream fout(string(newFile.toLocal8Bit())+".c",ios::out);
    fout<<fstring;
    fout.close();

    editName=string(newFile.toLocal8Bit());
    saved = true;

    updateApp();
    return true;
}

void MainWindow::on_actUndo_triggered() {   codeEditor->undo(); }

void MainWindow::on_actRedo_triggered() {   codeEditor->redo(); }

void MainWindow::on_actCopy_triggered() {   codeEditor->copy(); }

void MainWindow::on_actCut_triggered()  {   codeEditor->cut();  }

void MainWindow::on_actPaste_triggered(){   codeEditor->paste();}

void MainWindow::on_actBegin_triggered(){
    if(on_actSave_triggered())
        initTable();
}

void MainWindow::codeEdited(){
    saved = false;
    updateApp();
}

void MainWindow::closeEvent (QCloseEvent*event){
    if(!saved){
        QMessageBox::StandardButton msgBox;
        msgBox = QMessageBox::question(this, "提示", "文件未保存，是否保存文件？",
                                       QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel,QMessageBox::Yes);
        if (msgBox==QMessageBox::Yes){
            if(!on_actSave_triggered())
                event->ignore();
        }
        else if(msgBox==QMessageBox::No){}
        else
            event->ignore();
    }
}

void MainWindow::updateApp(){
    if(saved)
        this->setWindowTitle(QObject::tr((appName + " - " + editName).c_str()));
    else
        this->setWindowTitle(QObject::tr((appName + "* - " + editName).c_str()));
}
