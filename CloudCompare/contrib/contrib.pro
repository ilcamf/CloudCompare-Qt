######################################################################
# Automatically generated by qmake (3.0) Tue Sep 20 10:48:13 2016
######################################################################

#表示这个是Qt跨目录,由多个子项目组成的大项目
TEMPLATE    =   subdirs
#大项目包含的各个子项目
SUBDIRS =   dxflib-3.3.4 \
            shapelib-1.3.0 \

#CONFIG选项要求各个子项目按顺序编译，子目录的编译顺序在SUBDIRS中指明
CONFIG  +=  ordered

unix
{
    QMAKE_CC    =   gcc
    QMAKE_CXX   =   g++
}


#指定生成路径
#DESTDIR =
