######################################################################
# Automatically generated by qmake (3.0) Tue Sep 20 10:58:23 2016
######################################################################

QT  +=  widgets printsupport

TARGET = qcustomplot
TEMPLATE = lib
INCLUDEPATH += .

# Input
HEADERS += qcustomplot.h
SOURCES += qcustomplot.cpp

macx
{
# mac only

# 编译时候指定libs查找位置
QMAKE_LFLAGS_RELEASE += -Wl,-rpath,$$PWD/../../../Release/libs -Wl
QMAKE_LFLAGS_DEBUG += -Wl,-rpath,$$PWD/../../../Release/libs -Wl

#指定生成路径
DESTDIR = $$PWD/../../../Release/libs

}

unix:!macx{
# linux only

# 编译时候指定libs查找位置
QMAKE_LFLAGS_RELEASE += -Wl,-rpath=$$PWD/../../../Release/libs -Wl,-Bsymbolic
QMAKE_LFLAGS_DEBUG += -Wl,-rpath=$$PWD/../../../Release/libs -Wl,-Bsymbolic

#指定生成路径
DESTDIR = $$PWD/../../../Release/libs

}

win32 {
# windows only

}
