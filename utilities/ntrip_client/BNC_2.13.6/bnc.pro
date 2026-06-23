QT += printer widgets
QT += svg

TEMPLATE = subdirs

CONFIG += c++17
CONFIG += ordered

SUBDIRS = newmat   \
          qwt      \
          qwtpolar \
          src
