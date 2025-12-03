#include "Controller.h"
#include "seriallink.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

Controller::Controller(QObject *parent)
    : QObject(parent),
    m_currentSet(0),
    m_rotaryVals(8, 0),
    m_connected(false),
    m_serialLink(new SerialLink(this))
{
    // Connect serial link signals
    connect(m_serialLink, &SerialLink::opened,
            this, &Controller::onOpened);
    connect(m_serialLink, &SerialLink::closed,
            this, &Controller::onClosed);
    connect(m_serialLink, &SerialLink::serialError,
            this, &Controller::onSerialError);
    connect(m_serialLink, &SerialLink::buttonState,
            this, &Controller::onButtonState);
    connect(m_serialLink, &SerialLink::rotaryValue,
            this, &Controller::onRotaryValue);

    // Load profiles from JSON
    loadProfiles(QStringLiteral("/home/jam/Qt_Projects/Midi2LrController/profiles.json"));

    emit setCountChanged();
    emit layoutChanged();
    emit currentSetChanged();

    // Auto-open serial on startup
    m_serialLink->connectPort();
}

void Controller::loadProfiles(const QString &filePath)
{
    m_sets.clear();

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        emit logMessage(QStringLiteral("Could not open profiles file: %1").arg(filePath));
        return;
    }

    const QByteArray data = f.readAll();
    f.close();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        emit logMessage(QStringLiteral("Invalid JSON in %1: %2")
                            .arg(filePath, err.errorString()));
        return;
    }

    QJsonObject root = doc.object();
    QJsonArray sets = root.value(QStringLiteral("sets")).toArray();

    for (const QJsonValue &v : sets) {
        if (!v.isObject())
            continue;
        QJsonObject o = v.toObject();

        ProfileSet ps;
        ps.name = o.value(QStringLiteral("name")).toString(QStringLiteral("Unnamed"));

        // Buttons
        QJsonArray buttons = o.value(QStringLiteral("buttons")).toArray();
        for (int i = 0; i < 16; ++i) {
            ButtonDef b;
            if (i < buttons.size() && buttons.at(i).isObject()) {
                QJsonObject bo = buttons.at(i).toObject();
                b.text  = bo.value(QStringLiteral("text")).toString(QStringLiteral("B%1").arg(i));
                b.color = QColor(bo.value(QStringLiteral("color")).toString(QStringLiteral("#444444")));
            } else {
                b.text  = QStringLiteral("B%1").arg(i);
                b.color = QColor("#444444");
            }
            ps.buttons.append(b);
        }

        // Labels
        QJsonArray labels = o.value(QStringLiteral("labels")).toArray();
        for (int i = 0; i < 8; ++i) {
            if (i < labels.size())
                ps.labels.append(labels.at(i).toString());
            else
                ps.labels.append(QStringLiteral("Label %1").arg(i));
        }

        // Encoders accel (optional, not used yet)
        QJsonArray encoders = o.value(QStringLiteral("encoders")).toArray();
        for (int i = 0; i < 8; ++i) {
            EncoderDef ed;
            if (i < encoders.size() && encoders.at(i).isObject()) {
                QJsonObject eo = encoders.at(i).toObject();
                ed.accel = eo.value(QStringLiteral("accel")).toString(QStringLiteral("normal"));
            } else {
                ed.accel = QStringLiteral("normal");
            }
            ps.encoders.append(ed);
        }

        m_sets.append(ps);
    }

    if (m_sets.isEmpty()) {
        ProfileSet ps;
        ps.name = QStringLiteral("Default");
        for (int i = 0; i < 16; ++i) {
            ButtonDef b;
            b.text  = QStringLiteral("B%1").arg(i);
            b.color = QColor("#444444");
            ps.buttons.append(b);
        }
        for (int i = 0; i < 8; ++i)
            ps.labels.append(QStringLiteral("Label %1").arg(i));
        m_sets.append(ps);
    }

    if (m_currentSet >= m_sets.size())
        m_currentSet = 0;
}

QVariantList Controller::buttonTexts() const
{
    QVariantList list;
    if (m_sets.isEmpty())
        return list;

    const ProfileSet &ps = m_sets.at(m_currentSet);
    for (const ButtonDef &b : ps.buttons)
        list << b.text;
    return list;
}

QVariantList Controller::buttonColors() const
{
    QVariantList list;
    if (m_sets.isEmpty())
        return list;

    const ProfileSet &ps = m_sets.at(m_currentSet);
    for (const ButtonDef &b : ps.buttons)
        list << b.color;
    return list;
}

QVariantList Controller::labelTexts() const
{
    QVariantList list;
    if (m_sets.isEmpty())
        return list;

    const ProfileSet &ps = m_sets.at(m_currentSet);
    for (const QString &s : ps.labels)
        list << s;
    return list;
}

QVariantList Controller::rotaryValues() const
{
    QVariantList list;
    for (int v : m_rotaryVals)
        list << v;
    return list;
}

void Controller::setCurrentSet(int index)
{
    if (index < 0 || index >= m_sets.size())
        return;
    if (index == m_currentSet)
        return;

    m_currentSet = index;
    emit currentSetChanged();
    emit layoutChanged();
}

QString Controller::currentSetName() const
{
    if (m_sets.isEmpty())
        return QStringLiteral("No sets");
    return m_sets.at(m_currentSet).name;
}

void Controller::sendButton(int index, bool pressed)
{
    if (!m_serialLink)
        return;
    if (index < 0 || index >= 16)
        return;

    m_serialLink->sendButtonEvent(static_cast<quint8>(index),
                                  pressed);
}

void Controller::onOpened(const QString &port)
{
    m_connected = true;
    emit connectedChanged();
    emit logMessage(QStringLiteral("Serial opened: %1").arg(port));
}

void Controller::onClosed()
{
    m_connected = false;
    emit connectedChanged();
    emit logMessage(QStringLiteral("Serial closed"));
}

void Controller::onSerialError(const QString &msg)
{
    emit logMessage(msg);
}

void Controller::onButtonState(int id, bool pressed)
{
    Q_UNUSED(id)
    Q_UNUSED(pressed)
    // Currently we don't use Pico->Pi button states.
    // You can hook this later if you want LEDs etc.
}

void Controller::onRotaryValue(int id, int value)
{
    if (id < 0 || id >= m_rotaryVals.size())
        return;

    m_rotaryVals[id] = value;
    emit rotaryValuesChanged();
}
