#include "eyebus.h"

#include <QDBusConnection>
#include <QDBusError>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QDebug>
#include <QJSEngine>

namespace {
const QString kService = QStringLiteral("org.priax.SignalisEye");
const QString kObjectPath = QStringLiteral("/");
constexpr int kCoveredTtlMs = 25000;

const QString kScreenSaverInterface =
    QStringLiteral("org.freedesktop.ScreenSaver");
const QString kScreenSaverSignal = QStringLiteral("ActiveChanged");
const QString kScreenSaverPaths[] = {
    QStringLiteral("/ScreenSaver"),
    QStringLiteral("/org/freedesktop/ScreenSaver"),
};

const QString kKWinService = QStringLiteral("org.kde.KWin");
const QString kKWinPath = QStringLiteral("/KWin");
const QString kKWinInterface = QStringLiteral("org.kde.KWin");
const QString kShowingDesktopProperty = QStringLiteral("showingDesktop");
const QString kShowingDesktopSignal = QStringLiteral("showingDesktopChanged");
const QString kPropertiesInterface =
    QStringLiteral("org.freedesktop.DBus.Properties");
} // namespace

EyeBus *EyeBus::instance()
{
    static EyeBus *bus = new EyeBus;
    return bus;
}

EyeBus *EyeBus::create(QQmlEngine *, QJSEngine *)
{
    QJSEngine::setObjectOwnership(instance(), QJSEngine::CppOwnership);
    return instance();
}

EyeBus::EyeBus(QObject *parent) : QObject(parent)
{
    auto bus = QDBusConnection::sessionBus();
    registerOnBus(bus);
    watchScreenSaver(bus);
    watchShowingDesktop(bus);

    m_coveredExpiry.setSingleShot(true);
    m_coveredExpiry.setInterval(kCoveredTtlMs);
    connect(&m_coveredExpiry, &QTimer::timeout, this,
            [this] { setCovered({}); });
}

void EyeBus::registerOnBus(QDBusConnection &bus)
{
    const bool objectRegistered = bus.registerObject(
        kObjectPath, this, QDBusConnection::ExportScriptableSlots);
    if (!objectRegistered) {
        qWarning() << "SignalisEye: objet D-Bus" << kObjectPath
                   << "refusé:" << bus.lastError().message();
        return;
    }

    const bool serviceRegistered = bus.registerService(kService);
    if (!serviceRegistered)
        qWarning() << "SignalisEye: nom" << kService
                   << "déjà pris:" << bus.lastError().message();
}

void EyeBus::watchScreenSaver(QDBusConnection &bus)
{
    for (const QString &path : kScreenSaverPaths) {
        bus.connect(QString(), path, kScreenSaverInterface, kScreenSaverSignal,
                    this, SLOT(onScreenSaverActive(bool)));
    }
}

void EyeBus::watchShowingDesktop(QDBusConnection &bus)
{
    bus.connect(kKWinService, kKWinPath, kKWinInterface, kShowingDesktopSignal,
                this, SLOT(onShowingDesktopChanged(bool)));

    QDBusMessage get = QDBusMessage::createMethodCall(
        kKWinService, kKWinPath, kPropertiesInterface, QStringLiteral("Get"));
    get << kKWinInterface << kShowingDesktopProperty;

    auto *watcher = new QDBusPendingCallWatcher(bus.asyncCall(get), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this](QDBusPendingCallWatcher *w) {
                const QDBusPendingReply<QDBusVariant> reply = *w;
                if (reply.isValid())
                    onShowingDesktopChanged(reply.value().variant().toBool());
                w->deleteLater();
            });
}

void EyeBus::Cursor(int x, int y)
{
    const QPoint p(x, y);
    if (p == m_cursor)
        return;
    m_cursor = p;
    Q_EMIT cursorChanged();
}

void EyeBus::Cursor(double x, double y)
{
    Cursor(qRound(x), qRound(y));
}

void EyeBus::Covered(const QString &screens)
{
    m_coveredExpiry.start();
    setCovered(screens.split(u';', Qt::SkipEmptyParts));
}

void EyeBus::setCovered(const QStringList &screens)
{
    if (screens == m_covered)
        return;
    m_covered = screens;
    Q_EMIT coveredChanged();
}

void EyeBus::onShowingDesktopChanged(bool showing)
{
    if (showing == m_showingDesktop)
        return;
    m_showingDesktop = showing;
    Q_EMIT coveredChanged();
}

void EyeBus::onScreenSaverActive(bool active)
{
    if (active == m_locked)
        return;
    m_locked = active;
    Q_EMIT lockedChanged();
}
