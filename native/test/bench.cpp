// bench <moving|idle> <secondes> <fps> [Scene.qml] [fps au repos]
#include <QDir>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QTimer>
#include <QUrl>
#include <QtMath>
#include <cstdio>
#include <sys/resource.h>

static double cpuSeconds()
{
    rusage r{};
    getrusage(RUSAGE_SELF, &r);
    return r.ru_utime.tv_sec + r.ru_utime.tv_usec / 1e6 + r.ru_stime.tv_sec +
           r.ru_stime.tv_usec / 1e6;
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    const QStringList a = app.arguments().mid(1);
    const bool moving = a.value(0) == QLatin1String("moving");
    const int seconds = a.value(1, QStringLiteral("10")).toInt();

    QQuickView view;
    view.engine()->addImportPath(QDir::homePath() +
                                 QStringLiteral("/.local/lib64/qml"));
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.resize(1536, 864);
    view.setSource(QUrl::fromLocalFile(a.value(3, QStringLiteral(SCENE_PATH))));
    if (view.status() != QQuickView::Ready)
        return 1;
    QObject *scene = view.rootObject();
    scene->setProperty("fps", a.value(2, QStringLiteral("30")).toInt());
    scene->setProperty("idleFps", a.value(4, QStringLiteral("15")).toInt());
    scene->setProperty("brightness", 0.8337778);
    view.show();

    int frames = 0;
    QObject::connect(&view, &QQuickWindow::frameSwapped, [&] { ++frames; });

    QElapsedTimer clock;
    clock.start();
    QTimer mouse;
    QObject::connect(&mouse, &QTimer::timeout, [&] {
        const double t = clock.elapsed() / 1000.0;
        scene->setProperty("cursor", QPointF(0.5 + 0.4 * qSin(t * 1.3),
                                             0.5 + 0.4 * qSin(t * 0.7)));
    });
    if (moving)
        mouse.start(16);

    const int warmup = moving ? 2000 : 12000;
    double cpu0 = 0;
    int ticks0 = 0, pub0 = 0;
    QObject *ctrl = scene->property("controller").value<QObject *>();
    QTimer::singleShot(warmup, [&] {
        cpu0 = cpuSeconds();
        frames = 0;
        ticks0 = ctrl->property("ticks").toInt();
        pub0 = ctrl->property("published").toInt();
    });
    QTimer::singleShot(warmup + seconds * 1000, [&] {
        const double cpu = cpuSeconds() - cpu0;
        const int ticks = ctrl->property("ticks").toInt() - ticks0;
        const int pub = ctrl->property("published").toInt() - pub0;
        std::printf(
            "%s: %.1f %% d'un cœur, %.1f images/s (%d tics, %d publiés)\n",
            moving ? "souris" : "repos", 100.0 * cpu / seconds,
            double(frames) / seconds, ticks, pub);
        app.quit();
    });
    return app.exec();
}
