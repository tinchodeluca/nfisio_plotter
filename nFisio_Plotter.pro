QT += core gui widgets serialport printsupport opengl openglwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

INCLUDEPATH += $$PWD/qcustomplot  # Agrega esta línea

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    qcustomplot/qcustomplot.cpp

HEADERS += \
    mainwindow.h \
    qcustomplot/qcustomplot.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DEFINES += QCUSTOMPLOT_USE_OPENGL

win32: LIBS += -lopengl32
