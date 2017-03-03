#include <QtWidgets/QApplication>
#include "RoadmapMainWnd.hpp"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv); // QApplication prepara l'environment della Runtime di Qt
    RoadmapMainWnd w; // Dichiaro la finestra di entry point
    w.show(); // La mostro
    w.resize(800, 600); // La ridimensiono
    return a.exec(); // Avvio il render loop
}
