QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    generator.cpp \
    grammer.cpp \
    lexer.cpp \
    main.cpp \
    mainwindow.cpp \
    optimizer.cpp \
    parser.cpp \
    semantic.cpp

HEADERS += \
    Qsci/qsciapis.h \
    Qsci/qscilexercpp.h \
    Qsci/qsciscintilla.h \
    define.h \
    generator.h \
    grammar.h \
    lexer.h \
    mainwindow.h \
    optimizer.h \
    parser.h \
    semantic.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

CONFIG(debug, debug|release){
    LIBS += -Lqscintilla-Debug/debug/ -lqscintilla2_qt5d }
else {
    LIBS += -Lqscintilla-Release/release/ -lqscintilla2_qt5 }

RC_ICONS = res/icon.ico

RESOURCES += \
    res.qrc
