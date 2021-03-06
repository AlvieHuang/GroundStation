#include <QApplication>
#include <QSplashScreen>
#include <QtSvg>
//#include "mainmdidisplay.h"
#ifndef OLD_GUI
#include "mainmdidisplay.h"
#else
#include "mainwindow.h"
#endif

#include "mission.h"

#include "qttabtest.h"

int main(int argc, char *argv[]) {

    static const int splash_width = 812;
    static const int splash_height = 219;
    static const QString filename(":/res/images/UAV Forge Logo 2015.svg");

    QApplication a(argc, argv);
    //Construct and load the SVG image
    QSvgRenderer svg(filename);
    //Calculate the scaled size within a bounded size
    QSize SplashSize = svg.viewBox().size();
    SplashSize.scale(splash_width, splash_height, Qt::KeepAspectRatio);
    //Construct the pixmap to contain the SVG image
    QPixmap pixmap(SplashSize);
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    //Render the SVG to the pixmap
    svg.render(&painter, pixmap.rect());
    //Construct and show the splash screen
    QSplashScreen splash(pixmap);
    splash.show();

    MainMDIDisplay x;
    x.showFullScreen();

    //Hide the splash screen.
    splash.finish(&x);
    return a.exec();
}

