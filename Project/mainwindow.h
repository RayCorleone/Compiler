#pragma once
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "parser.h"
#include "optimizer.h"
#include "generator.h"
#include <sstream>
#include <QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>

#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexercpp.h>
#include <Qsci/qsciapis.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent*event);

private:
    Ui::MainWindow *ui;     //界面
    QsciScintilla *codeEditor;  //编辑器

    bool saved = true;
    int stackSize = 128;
    string editName = "未命名";
    const string fileName = "未知";
    const string appName = "小型编译器";

    void updateApp();
    void initTable();
    void initEditor(QsciScintilla *editor);

private slots:
    ///工具栏槽
    void on_actOpen_triggered();    //打开文件
    void on_actNew_triggered();     //新建文件
    bool on_actSave_triggered();    //保存
    bool on_actSaveas_triggered();  //另存为
    void on_actUndo_triggered();    //撤销
    void on_actRedo_triggered();    //重做
    void on_actCopy_triggered();    //复制
    void on_actCut_triggered();     //剪切
    void on_actPaste_triggered();   //粘贴
    void on_actBegin_triggered();   //分析

    ///其他信号
    void codeEdited();   //文本变化
};

#endif // MAINWINDOW_H
