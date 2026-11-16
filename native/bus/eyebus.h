#pragma once

#include <QObject>
#include <QPoint>
#include <QStringList>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

class QDBusConnection;
class QQmlEngine;
class QJSEngine;

class EyeBus : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_CLASSINFO("D-Bus Interface", "org.priax.SignalisEye")

    Q_PROPERTY(QPoint cursor READ cursor NOTIFY cursorChanged)
    Q_PROPERTY(
        QStringList coveredScreens READ coveredScreens NOTIFY coveredChanged)
    Q_PROPERTY(bool locked READ locked NOTIFY lockedChanged)

  public:
    static EyeBus *instance();
    static EyeBus *create(QQmlEngine *, QJSEngine *);

    QPoint cursor() const
    {
        return m_cursor;
    }

    bool hasCursor() const
    {
        return m_hasCursor;
    }

    QStringList coveredScreens() const
    {
        return m_showingDesktop ? QStringList() : m_covered;
    }

    bool locked() const
    {
        return m_locked;
    }

  public Q_SLOTS:
    Q_SCRIPTABLE void Cursor(int x, int y);
    Q_SCRIPTABLE void Cursor(double x, double y);
    Q_SCRIPTABLE void Covered(const QString &screens);

  Q_SIGNALS:
    void cursorChanged();
    void coveredChanged();
    void lockedChanged();

  private Q_SLOTS:
    void onScreenSaverActive(bool active);
    void onShowingDesktopChanged(bool showing);

  private:
    explicit EyeBus(QObject *parent = nullptr);
    void registerOnBus(QDBusConnection &bus);
    void watchScreenSaver(QDBusConnection &bus);
    void watchShowingDesktop(QDBusConnection &bus);
    void setCovered(const QStringList &screens);

    QPoint m_cursor;
    bool m_hasCursor = false;
    QStringList m_covered;
    bool m_locked = false;
    bool m_showingDesktop = false;
    QTimer m_coveredExpiry;
};
