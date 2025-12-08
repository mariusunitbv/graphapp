#include <pch.h>

#include "form/GraphApp.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 2);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);

    GraphApp window;
    window.show();

    return app.exec();
}
