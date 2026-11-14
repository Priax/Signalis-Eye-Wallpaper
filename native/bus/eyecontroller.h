#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QPointF>
#include <QQuaternion>
#include <QRectF>
#include <QTimer>
#include <QVector3D>
#include <QtQml/qqmlregistration.h>

class EyeController : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool followBus MEMBER m_followBus WRITE setFollowBus NOTIFY
                   followBusChanged)
    Q_PROPERTY(qreal screenX READ screenX WRITE setScreenX NOTIFY
                   screenGeometryChanged)
    Q_PROPERTY(qreal screenY READ screenY WRITE setScreenY NOTIFY
                   screenGeometryChanged)
    Q_PROPERTY(qreal screenWidth READ screenWidth WRITE setScreenWidth NOTIFY
                   screenGeometryChanged)
    Q_PROPERTY(qreal screenHeight READ screenHeight WRITE setScreenHeight NOTIFY
                   screenGeometryChanged)
    Q_PROPERTY(QPointF cursor READ cursor WRITE setCursor NOTIFY cursorChanged)
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
    Q_PROPERTY(int fps MEMBER m_fps NOTIFY fpsChanged)
    Q_PROPERTY(int idleFps MEMBER m_idleFps NOTIFY fpsChanged)

    // sorties, toutes publiées ensemble
    Q_PROPERTY(QVector3D eyePosition READ eyePosition NOTIFY frameChanged)
    Q_PROPERTY(QQuaternion lookRotation READ lookRotation NOTIFY frameChanged)
    Q_PROPERTY(QVector3D holeCenter READ holeCenter NOTIFY frameChanged)
    Q_PROPERTY(QVector3D holeDir READ holeDir NOTIFY frameChanged)
    Q_PROPERTY(bool lidDownTrans READ lidDownTrans NOTIFY frameChanged)
    Q_PROPERTY(bool lidDownLook READ lidDownLook NOTIFY frameChanged)
    Q_PROPERTY(bool lidHalf READ lidHalf NOTIFY frameChanged)
    Q_PROPERTY(bool lidClosed READ lidClosed NOTIFY frameChanged)
    Q_PROPERTY(qreal falloff READ falloff NOTIFY frameChanged)
    Q_PROPERTY(qreal hairTime READ hairTime NOTIFY frameChanged)

    Q_PROPERTY(int ticks READ ticks)
    Q_PROPERTY(int published READ published)

  public:
    enum Lid { NoLid, DownTrans, DownLook, Half, Closed };
    Q_ENUM(Lid)

    explicit EyeController(QObject *parent = nullptr);

    QPointF cursor() const
    {
        return m_cursor;
    }

    void setCursor(QPointF c);

    bool paused() const
    {
        return m_paused;
    }

    void setPaused(bool p);

    void setFollowBus(bool f);

    QVector3D eyePosition() const
    {
        return m_out.eyePos;
    }

    QQuaternion lookRotation() const
    {
        return m_out.look;
    }

    QVector3D holeCenter() const;

    QVector3D holeDir() const;

    bool lidDownTrans() const
    {
        return m_out.lid == DownTrans;
    }

    bool lidDownLook() const
    {
        return m_out.lid == DownLook;
    }

    bool lidHalf() const
    {
        return m_out.lid == Half;
    }

    bool lidClosed() const
    {
        return m_out.lid == Closed;
    }

    qreal screenX() const
    {
        return m_screen.x();
    }

    qreal screenY() const
    {
        return m_screen.y();
    }

    qreal screenWidth() const
    {
        return m_screen.width();
    }

    qreal screenHeight() const
    {
        return m_screen.height();
    }

    void setScreenX(qreal v)
    {
        if (v != m_screen.x()) {
            m_screen.moveLeft(v);
            Q_EMIT screenGeometryChanged();
        }
    }

    void setScreenY(qreal v)
    {
        if (v != m_screen.y()) {
            m_screen.moveTop(v);
            Q_EMIT screenGeometryChanged();
        }
    }

    void setScreenWidth(qreal v)
    {
        if (v != m_screen.width()) {
            m_screen.setWidth(v);
            Q_EMIT screenGeometryChanged();
        }
    }

    void setScreenHeight(qreal v)
    {
        if (v != m_screen.height()) {
            m_screen.setHeight(v);
            Q_EMIT screenGeometryChanged();
        }
    }

    qreal falloff() const
    {
        return m_out.falloff;
    }

    qreal hairTime() const
    {
        return m_out.hairTime;
    }

    int ticks() const
    {
        return m_ticks;
    }

    int published() const
    {
        return m_published;
    }

  Q_SIGNALS:
    void followBusChanged();
    void screenGeometryChanged();
    void cursorChanged();
    void pausedChanged();
    void fpsChanged();
    void frameChanged();

  private:
    struct Frame {
        QVector3D eyePos;
        QQuaternion look;
        int lid = NoLid;
        double falloff = 0.6;
        double hairTime = 0;
    };

    void tick();
    void step(double dt);
    Lid updateLid(double t);
    bool visiblyDifferent(const Frame &a, const Frame &b) const;
    void updateFromBus();
    void reschedule();
    bool idle() const;

    bool m_followBus = false;
    QRectF m_screen;
    QPointF m_cursor{0.5, 0.5};
    bool m_paused = false;
    bool m_busPaused = false;
    int m_fps = 30;
    int m_idleFps = 30;

    QTimer m_timer;
    QElapsedTimer m_clock;
    double m_last = 0;
    double m_t = 0;
    double m_lastMove = -1e9;

    double m_tx = 0, m_ty = 3, m_vx = 0, m_vy = 0;
    double m_fx = 0, m_fy = 0, m_nextFlicker = 0;
    double m_nextBlink = 6, m_blinkStart = -1;

    Frame m_cur;
    Frame m_out;
    int m_ticks = 0;
    int m_published = 0;
};
