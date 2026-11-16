#include "eyecontroller.h"
#include "eyebus.h"

#include <QRandomGenerator>
#include <QtMath>
#include <cmath>

namespace {
constexpr double kLimitX = 30;
constexpr double kLimitY = 15;
constexpr double kOffsetY = -1;
constexpr double kSmoothDampTime = 0.2;
constexpr double kReturnToIdleSpeed = 0.5;
constexpr double kIdleDelay = 8;

constexpr double kBreathingIntensity = 0.5;
constexpr double kBreathingSpeed = 0.7;
constexpr double kShakeIntensity = 0.2;
constexpr double kShakeSpeed = 0.01;
constexpr double kEyeFlickering = 0.1;
constexpr double kEyeFlickerTime = 0.1;

constexpr double kBlinkTime = 6;
constexpr double kLidCloseTime = 0.02;
constexpr double kLidClosedTime = 0.1;
constexpr double kLookDownThreshold = -2;
constexpr double kLookDownFullThreshold = -5;

constexpr double kVignetteFalloff = 0.6;
constexpr double kVignetteSpeed = 1;
constexpr double kVignettePulse = 0.1;

const QVector3D kEyePos(-1.45f, -0.7f, 105.f);
const QQuaternion kEyeRot(0.7071068f, 0.7071068f, 0, 0);
const QQuaternion kPupilRot(0, 0, 0.7071068f, -0.7071068f);

constexpr double kPixel = 0.19;
constexpr double kPositionThreshold = 0.5 * kPixel;
constexpr double kAngleThreshold = 0.5 * kPixel / 29.0;
constexpr double kHairAmplitude = 5.0;
constexpr double kHairThreshold = kPixel;
constexpr double kFalloffThreshold = 0.008;

double smoothDamp(double cur, double target, double &vel, double smoothTime,
                  double dt)
{
    const double omega = 2 / smoothTime;
    const double x = omega * dt;
    const double e = 1 / (1 + x + 0.48 * x * x + 0.235 * x * x * x);
    const double change = cur - target;
    const double temp = (vel + omega * change) * dt;
    vel = (vel - omega * temp) * e;
    return target + (change + temp) * e;
}

double noise1(double t, double seed)
{
    return std::sin(t * 1.7 + seed) * 0.5 +
           std::sin(t * 2.9 + seed * 3.1) * 0.3 +
           std::sin(t * 4.3 + seed * 1.3) * 0.2;
}

double rnd()
{
    return QRandomGenerator::global()->generateDouble();
}

QQuaternion unityLookRotation(QVector3D forward)
{
    forward.normalize();
    const QVector3D worldUp(0, 1, 0);
    const QVector3D right =
        QVector3D::crossProduct(worldUp, forward).normalized();
    const QVector3D up = QVector3D::crossProduct(forward, right);
    return QQuaternion::fromAxes(right, up, forward);
}
} // namespace

EyeController::EyeController(QObject *parent) : QObject(parent)
{
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &EyeController::tick);
    m_nextBlink = kBlinkTime * (0.5 + rnd());
    m_clock.start();
    step(1.0 / 60);
    m_out = m_cur;
    reschedule();
}

void EyeController::setCursor(QPointF c)
{
    if (c == m_cursor)
        return;
    m_cursor = c;
    m_lastMove = m_t;
    reschedule();
    Q_EMIT cursorChanged();
}

void EyeController::setCursorSilently(QPointF c)
{
    if (c == m_cursor)
        return;
    m_cursor = c;
    Q_EMIT cursorChanged();
}

void EyeController::setPaused(bool p)
{
    if (p == m_paused)
        return;
    m_paused = p;
    reschedule();
    Q_EMIT pausedChanged();
}

void EyeController::setFollowBus(bool f)
{
    if (f == m_followBus)
        return;
    m_followBus = f;
    if (f) {
        auto *bus = EyeBus::instance();
        connect(bus, &EyeBus::cursorChanged, this,
                [this] { updateFromBus(true); });
        connect(bus, &EyeBus::coveredChanged, this,
                [this] { updateFromBus(); });
        connect(bus, &EyeBus::lockedChanged, this, [this] { updateFromBus(); });
        connect(this, &EyeController::screenGeometryChanged, this,
                [this] { updateFromBus(); });
        updateFromBus();
    }
    Q_EMIT followBusChanged();
}

void EyeController::updateFromBus(bool moved)
{
    const auto *bus = EyeBus::instance();
    if (bus->hasCursor() && m_screen.width() > 0 && m_screen.height() > 0) {
        const QPoint p = bus->cursor();
        const double x = (p.x() - m_screen.x()) / m_screen.width();
        const double y = (p.y() - m_screen.y()) / m_screen.height();
        const bool first = !m_gotBusCursor;
        m_gotBusCursor = true;
        if (moved && !first)
            setCursor(QPointF(x, y));
        else
            setCursorSilently(QPointF(x, y));
    }
    const QString key = QStringLiteral("%1,%2,%3,%4")
                            .arg(m_screen.x())
                            .arg(m_screen.y())
                            .arg(m_screen.width())
                            .arg(m_screen.height());
    const bool covered = bus->coveredScreens().contains(key);
    const bool hidden = bus->locked() || covered;
    if (hidden != m_busPaused) {
        m_busPaused = hidden;
        reschedule();
    }
}

bool EyeController::idle() const
{
    return m_t - m_lastMove > kIdleDelay + 2;
}

void EyeController::reschedule()
{
    if (m_paused || m_busPaused) {
        m_timer.stop();
        return;
    }
    const int fps = std::max(1, idle() ? std::min(m_fps, m_idleFps) : m_fps);
    const int interval = 1000 / fps;
    if (!m_timer.isActive()) {
        m_last = m_clock.nsecsElapsed() / 1e9;
        m_timer.start(interval);
    } else if (m_timer.interval() != interval) {
        m_timer.setInterval(interval);
    }
}

void EyeController::tick()
{
    const double now = m_clock.nsecsElapsed() / 1e9;
    const double dt = std::min(0.1, now - m_last);
    m_last = now;
    m_t += dt;
    ++m_ticks;
    step(dt);
    if (visiblyDifferent(m_cur, m_out)) {
        m_out = m_cur;
        ++m_published;
        Q_EMIT frameChanged();
    }
    reschedule();
}

void EyeController::step(double dt)
{
    const double t = m_t;
    const bool lookIdle = t - m_lastMove > kIdleDelay;
    double dx = 0;
    double dy = 3;
    if (!lookIdle) {
        const double menuX = (m_cursor.x() - 0.5) * 64;
        const double menuY = (0.5 - m_cursor.y()) * 36;
        dx = std::clamp(menuX, -kLimitX, kLimitX);
        dy = std::clamp(menuY, -kLimitY, kLimitY) + kOffsetY;
    }
    if (t > m_nextFlicker) {
        m_nextFlicker = t + kEyeFlickerTime * (0.5 + rnd());
        m_fx = (rnd() - 0.5) * 2 * kEyeFlickering;
        m_fy = (rnd() - 0.5) * 2 * kEyeFlickering;
    }
    const double smoothTime =
        lookIdle ? kSmoothDampTime / kReturnToIdleSpeed : kSmoothDampTime;
    m_tx = smoothDamp(m_tx, dx + m_fx, m_vx, smoothTime, dt);
    m_ty = smoothDamp(m_ty, dy + m_fy, m_vy, smoothTime, dt);

    const double breathe =
        std::sin(t * kBreathingSpeed * M_PI) * kBreathingIntensity;
    const double shakeT = t * kShakeSpeed * 60;
    const double shakeX = noise1(shakeT, 1) * kShakeIntensity;
    const double shakeY = noise1(shakeT, 7) * kShakeIntensity;
    m_cur.eyePos =
        QVector3D(float(kEyePos.x() + shakeX),
                  float(kEyePos.y() + breathe + shakeY), kEyePos.z());

    const QVector3D target(float(m_tx), float(m_ty), 0);
    const QQuaternion worldLook = unityLookRotation(target - m_cur.eyePos);
    m_cur.look = kEyeRot.conjugated() * worldLook;

    m_cur.lid = updateLid(t);
    m_cur.falloff =
        kVignetteFalloff + std::sin(t * kVignetteSpeed) * kVignettePulse;
    m_cur.hairTime = t;
}

EyeController::Lid EyeController::updateLid(double t)
{
    if (m_blinkStart < 0 && t > m_nextBlink)
        m_blinkStart = t;

    Lid lid = NoLid;
    if (m_ty < kLookDownFullThreshold)
        lid = DownLook;
    else if (m_ty < kLookDownThreshold)
        lid = DownTrans;

    if (m_blinkStart >= 0) {
        const double elapsed = t - m_blinkStart;
        const double closedAt = kLidCloseTime;
        const double openingAt = closedAt + kLidClosedTime;
        const double openAt = openingAt + kLidCloseTime;

        if (elapsed < closedAt)
            lid = Half;
        else if (elapsed < openingAt)
            lid = Closed;
        else if (elapsed < openAt)
            lid = Half;
        else {
            m_blinkStart = -1;
            m_nextBlink = t + kBlinkTime * (0.5 + rnd());
        }
    }
    return lid;
}

bool EyeController::visiblyDifferent(const Frame &a, const Frame &b) const
{
    if (a.lid != b.lid)
        return true;

    const double moved = (a.eyePos - b.eyePos).length();
    if (moved > kPositionThreshold)
        return true;

    const double dot =
        std::abs(double(QQuaternion::dotProduct(a.look, b.look)));
    const double angle = 2 * std::acos(std::min(1.0, dot));
    if (angle > kAngleThreshold)
        return true;

    const double hairShift =
        kHairAmplitude * std::abs(std::cos(a.hairTime) - std::cos(b.hairTime));
    if (hairShift > kHairThreshold)
        return true;

    return std::abs(a.falloff - b.falloff) > kFalloffThreshold;
}

QVector3D EyeController::holeCenter() const
{
    return QVector3D(m_out.eyePos.x(), m_out.eyePos.y(), -m_out.eyePos.z());
}

QVector3D EyeController::holeDir() const
{
    const QQuaternion pupil = kEyeRot * m_out.look * kPupilRot;
    const QVector3D d = pupil.rotatedVector(QVector3D(0, -1, 0));
    return QVector3D(d.x(), d.y(), -d.z()).normalized();
}
