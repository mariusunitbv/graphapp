#include <pch.h>

#include "form/GraphApp.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    GraphApp window;
    window.show();

    return app.exec();
}
