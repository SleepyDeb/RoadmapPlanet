
LIBS += -L"$$_PRO_FILE_PWD_/lib/" -lkdchart
INCLUDEPATH += "$$_PRO_FILE_PWD_/include/" \
    "$$_PRO_FILE_PWD_/include/KDChart/" \
    "$$_PRO_FILE_PWD_/include/KDGantt/"

QT += printsupport

include( RoadmapPlanet.pri )
