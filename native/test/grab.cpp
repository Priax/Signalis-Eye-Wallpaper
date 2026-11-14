// grab <sortie.png> <cx> <cy> [raw]
#include <QDir>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QTimer>
#include <QUrl>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    const QStringList a = app.arguments().mid(1);
    QQuickView view;
    view.engine()->addImportPath(QDir::homePath() +
                                 QStringLiteral("/.local/lib64/qml"));
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.resize(1280, 720);
    view.setSource(QUrl::fromLocalFile(QStringLiteral(SCENE_PATH)));
    if (view.status() != QQuickView::Ready)
        return 1;
    QObject *scene = view.rootObject();
    scene->setProperty("fps", 60);
    scene->setProperty("brightness", 0.8337778);
    scene->setProperty("showRaw", a.value(3) == QLatin1String("raw"));
    scene->setProperty("cursor",
                       QPointF(a.value(1).toDouble(), a.value(2).toDouble()));
    view.show();
    QTimer::singleShot(2500, [&] {
        scene->setProperty("paused", true);
        QTimer::singleShot(200, [&] {
            view.grabWindow().save(a.value(0));
            app.quit();
        });
    });
    return app.exec();
}
