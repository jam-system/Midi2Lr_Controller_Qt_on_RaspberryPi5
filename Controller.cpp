#include "Controller.h"
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>


Controller::Controller(QObject *parent)
    : QObject(parent),
    // 16 UI buttons (0/1)
    m_buttonStates(16, 0),
    // 8 rotaries (0..127)
    m_rotaryValues(8, 0),
    m_portName(QStringLiteral("/dev/ttyAMA0"))   // adjust if needed
{
    // Pico -> Pi
    connect(&m_serial, &SerialLink::buttonState,
            this, &Controller::onButtonState);
    connect(&m_serial, &SerialLink::rotaryDelta,
            this, &Controller::onRotaryDelta);

    // Optional: if SerialLink has some raw/hex signal you want to log
    // connect(&m_serial, &SerialLink::rawFrameHex,
    //         this, &Controller::onRawLine);

    // Error & state
    connect(&m_serial, &SerialLink::errorOccurred,
            this, &Controller::onSerialError);
    connect(&m_serial, &SerialLink::opened,
            this, &Controller::onOpened);
    connect(&m_serial, &SerialLink::closed,
            this, &Controller::onClosed);
            
    // Load profiles from file
    loadProfiles(QStringLiteral("/home/jam/Qt_Projects/Midi2LrController/profiles.json"));
}

// --- Q_PROPERTY accessors ---

QVariantList Controller::buttonStates() const
{
    QVariantList list;
    list.reserve(m_buttonStates.size());
    for (int v : m_buttonStates)
        list.push_back(v);
    return list;
}

QVariantList Controller::rotaryValues() const
{
    QVariantList list;
    list.reserve(m_rotaryValues.size());
    for (int v : m_rotaryValues)
        list.push_back(v);
    return list;
}

void Controller::setPortName(const QString &name)
{
    if (name == m_portName)
        return;
    m_portName = name;
    emit portNameChanged();
}

// --- QML-callable functions ---

void Controller::connectSerial()
{
    if (m_serial.isOpen()) {
        m_serial.close();
        return;
    }

    if (!m_serial.open(m_portName, 115200)) {
        emit logMessage(QStringLiteral("Failed to open %1").arg(m_portName));
    }
}

// Called from QML buttons: send Pressed/Released to Pico
void Controller::sendButton(int id, bool pressed)
{
    if (id < 0 || id >= 16)
        return;

    if (!m_serial.isOpen()) {
        emit logMessage(QStringLiteral("sendButton: serial not open"));
        return;
    }

    m_serial.sendButtonEvent(static_cast<quint8>(id), pressed);

    // optional: mirror state in UI
    if (id < m_buttonStates.size()) {
        m_buttonStates[id] = pressed ? 1 : 0;
        emit buttonStatesChanged();
    }
}

// --- slots receiving data from SerialLink (Pico -> Pi) ---

void Controller::onButtonState(int id, bool pressed)
{
    // Only use this if Pico ever sends its own button states.
    // You said you don't want Pico buttons on the Pi, but
    // keeping this is harmless and may help later.
    if (id < 0 || id >= m_buttonStates.size())
        return;

    m_buttonStates[id] = pressed ? 1 : 0;
    emit buttonStatesChanged();

    qDebug() << "Controller: button" << id << "->" << (pressed ? "pressed" : "released");
}

void Controller::onRotaryDelta(int id, int value)
{
    // Here 'value' is an ABSOLUTE encoder value (0..127) from Pico
    if (id < 0 || id >= m_rotaryValues.size())
        return;

    int v = value;
    if (v < 0)   v = 0;
    if (v > 127) v = 127;

    m_rotaryValues[id] = v;
    emit rotaryValuesChanged();

    qDebug() << "Controller: rotary" << id << "value" << v;
}

// Optional: log raw text/hex if you have a signal connected
void Controller::onRawLine(const QString &line)
{
    emit logMessage(QStringLiteral("RX: %1").arg(line));
}

// --- SerialLink / error + state handling ---

void Controller::onSerialError(const QString &msg)
{
    qWarning() << "Serial error:" << msg;
    emit logMessage(QStringLiteral("ERROR: %1").arg(msg));
}

void Controller::onOpened(const QString &portName)
{
    emit logMessage(QStringLiteral("Opened %1").arg(portName));
    emit connectedChanged();
}

void Controller::onClosed()
{
    emit logMessage(QStringLiteral("Serial closed"));
    emit connectedChanged();
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
        if (!v.isObject()) continue;
        QJsonObject o = v.toObject();

        ProfileSet ps;
        ps.name = o.value(QStringLiteral("name")).toString(QStringLiteral("Unnamed"));

        // buttons
        QJsonArray buttons = o.value(QStringLiteral("buttons")).toArray();
        for (const QJsonValue &bv : buttons) {
            if (!bv.isObject()) continue;
            QJsonObject bo = bv.toObject();
            ButtonDef b;
            b.text  = bo.value(QStringLiteral("text")).toString(QString());
            b.color = QColor(bo.value(QStringLiteral("color")).toString(QStringLiteral("#444444")));
            ps.buttons.append(b);
        }

        // Ensure 16 buttons
        while (ps.buttons.size() < 16) {
            ButtonDef b;
            b.text  = QStringLiteral("B%1").arg(ps.buttons.size());
            b.color = QColor("#444444");
            ps.buttons.append(b);
        }

        // labels
        QJsonArray labels = o.value(QStringLiteral("labels")).toArray();
        for (const QJsonValue &lv : labels) {
            ps.labels.append(lv.toString());
        }
        // Ensure 8 labels
        while (ps.labels.size() < 8)
            ps.labels.append(QStringLiteral("Label %1").arg(ps.labels.size()));

        m_sets.append(ps);
    }

    if (m_sets.isEmpty()) {
        emit logMessage(QStringLiteral("No sets found in %1").arg(filePath));
    } else {
        emit logMessage(QStringLiteral("Loaded %1 sets from %2")
                        .arg(m_sets.size())
                        .arg(filePath));
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
