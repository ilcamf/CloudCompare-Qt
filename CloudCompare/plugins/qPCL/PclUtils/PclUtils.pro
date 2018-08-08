######################################################################
# Automatically generated by qmake (3.0) Tue Sep 20 23:28:19 2016
######################################################################
QT  +=  opengl concurrent

TARGET = PCLUTILS
TEMPLATE = lib
CONFIG += staticlib
INCLUDEPATH +=  .\
                ./filters \
                ./utils \
                ./filters/dialogs \
                $$PWD/../../

# Input
HEADERS += filters/BaseFilter.h \
           filters/ExtractSIFT.h \
           filters/MLSSmoothingUpsampling.h \
           filters/NormalEstimation.h \
           filters/StatisticalOutliersRemover.h \
           utils/cc2sm.h \
           utils/copy.h \
           utils/my_point_types.h \
           utils/PCLCloud.h \
           utils/PCLConv.h \
           utils/sm2cc.h \
           filters/dialogs/MLSDialog.h \
           filters/dialogs/NormalEstimationDlg.h \
           filters/dialogs/SIFTExtractDlg.h \
           filters/dialogs/StatisticalOutliersRemoverDlg.h

FORMS += filters/dialogs/MLSDialog.ui \
         filters/dialogs/NormalEstimationDlg.ui \
         filters/dialogs/SIFTExtractDlg.ui \
         filters/dialogs/StatisticalOutliersRemoverDlg.ui

SOURCES += filters/BaseFilter.cpp \
           filters/ExtractSIFT.cpp \
           filters/MLSSmoothingUpsampling.cpp \
           filters/NormalEstimation.cpp \
           filters/StatisticalOutliersRemover.cpp \
           utils/cc2sm.cpp \
           utils/copy.cpp \
           utils/sm2cc.cpp \
           filters/dialogs/MLSDialog.cpp \
           filters/dialogs/NormalEstimationDlg.cpp \
           filters/dialogs/SIFTExtractDlg.cpp \
           filters/dialogs/StatisticalOutliersRemoverDlg.cpp

#CC
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../Release/libs/ -lCC_CORE_LIB
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../Release/libs/ -lCC_CORE_LIB
else:unix: LIBS += -L$$PWD/../../../../Release/libs/ -lCC_CORE_LIB

INCLUDEPATH += $$PWD/../../../CC/include
DEPENDPATH += $$PWD/../../../CC

#qCC_db
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../Release/libs/ -lQCC_DB_LIB
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../Release/libs/ -lQCC_DB_LIB
else:unix: LIBS += -L$$PWD/../../../../Release/libs/ -lQCC_DB_LIB

INCLUDEPATH += $$PWD/../../../libs/qCC_db
DEPENDPATH += $$PWD/../../../libs/qCC_db

macx
{

# 编译时候指定libs查找位置
QMAKE_LFLAGS_RELEASE += -Wl,-rpath,$$PWD/../../../../Release/libs -Wl
QMAKE_LFLAGS_DEBUG += -Wl,-rpath,$$PWD/../../../../Release/libs -Wl

#指定生成路径
DESTDIR = $$PWD/../../../../Release/libs

#boost
INCLUDEPATH +=  /usr/local/Cellar/boost/1.62.0/include

#eigen
INCLUDEPATH +=  /usr/local/Cellar/eigen/3.3.3/include/eigen3

#flann
INCLUDEPATH +=  /usr/local/Cellar/flann/1.9.1_1/include

#PCL
INCLUDEPATH +=  /usr/local/include/pcl-1.8
INCLUDEPATH += /usr/local/include/

LIBS    +=  -L$$PWD/../../../../../../../../usr/local/Cellar/pcl/1.8.0_7/lib/ -lpcl_apps \
            -L$$PWD/../../../../../../../../usr/local/Cellar/pcl/1.8.0_7/lib/ -lpcl_common \
            -L$$PWD/../../../../../../../../usr/local/Cellar/pcl/1.8.0_7/lib/ -lpcl_filters \
            -L$$PWD/../../../../../../../../usr/local/Cellar/pcl/1.8.0_7/lib/ -lpcl_keypoints \
            -L$$PWD/../../../../../../../../usr/local/Cellar/pcl/1.8.0_7/lib/ -lpcl_kdtree \
            -L$$PWD/../../../../../../../../usr/local/Cellar/pcl/1.8.0_7/lib/ -lpcl_search \
            -L$$PWD/../../../../../../../../usr/local/Cellar/pcl/1.8.0_7/lib/ -lpcl_features \
            -L$$PWD/../../../../../../../../usr/local/Cellar/pcl/1.8.0_7/lib/ -lpcl_io \
            -L$$PWD/../../../../../../../../usr/local/Cellar/pcl/1.8.0_7/lib/ -lpcl_io_ply \
            -L$$PWD/../../../../../../../../usr/local/Cellar/pcl/1.8.0_7/lib/ -lpcl_visualization

}

unix:!macx{
# linux only

# 编译时候指定libs查找位置
QMAKE_LFLAGS_RELEASE += -Wl,-rpath=$$PWD/../../../../Release/libs -Wl,-Bsymbolic
QMAKE_LFLAGS_DEBUG += -Wl,-rpath=$$PWD/../../../../Release/libs -Wl,-Bsymbolic

#指定生成路径
DESTDIR = $$PWD/../../../../Release/libs

# lib
LIBS += -lpcl_apps \
        -lpcl_common \
        -lpcl_filters \
        -lpcl_keypoints \
        -lpcl_kdtree \
        -lpcl_search \
        -lpcl_surface \
        -lpcl_features \
        -lpcl_io \
        -lpcl_io_ply \
        -lpcl_visualization

#boost
INCLUDEPATH +=  /usr/include/boost

#eigen
INCLUDEPATH +=  /usr/include/eigen3

#flann
INCLUDEPATH +=  /usr/include/flann

#PCL
INCLUDEPATH +=  /usr/include/pcl-1.7
INCLUDEPATH += /usr/include/
}

win32 {
# windows only

}

