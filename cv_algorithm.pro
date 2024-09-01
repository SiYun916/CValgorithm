QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        AlignmentAlgorithm.cpp \
        CorrectionAlgorithm.cpp \
        main.cpp

HEADERS += \
    AlignmentAlgorithm.h \
    CorrectionAlgorithm.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target



unix|win32: LIBS += -L$$PWD/../../../../CodeTools/OpenCV3.4.16/opencv/buildNew/install/x64/mingw/lib/ -llibopencv_world3416.dll

INCLUDEPATH += $$PWD/../../../../CodeTools/OpenCV3.4.16/opencv/buildNew/install/include
DEPENDPATH += $$PWD/../../../../CodeTools/OpenCV3.4.16/opencv/buildNew/install/include
