#include "seriallink.h"
#include <QDebug>

static const quint8 FRAME_HEADER_1 = 0xAA;
static const quint8 FRAME_HEADER_2 = 0x55;

// Message types (must match Pico)
static const quint8 MSG_BUTTON_EVENT   = 0x01; // both directions: [id, state]
static const quint8 MSG_ENCODER_VALUE  = 0x02; // Pico -> Pi: [id, value]

SerialLink::SerialLink(QObject *parent)
    : QObject(parent),
    m_portName(QStringLiteral("/dev/ttyAMA0")),
    m_baudRate(115200),
    m_state(WaitHeader1),
    m_len(0),
    m_type(0),
    m_checksumAccum(0)
{
    connect(&m_serial, &QSerialPort::readyRead,
            this, &SerialLink::handleReadyRead);

    connect(&m_serial, &QSerialPort::errorOccurred,
            this, &SerialLink::handleError);
}

void SerialLink::connectPort()
{
    if (m_serial.isOpen())
        return;

    m_serial.setPortName(m_portName);
    m_serial.setBaudRate(m_baudRate);

    if (m_serial.open(QIODevice::ReadWrite)) {
        qDebug() << "SerialLink opened" << m_portName;
        emit opened(m_portName);
    } else {
        emit serialError(QStringLiteral("Cannot open %1: %2")
                             .arg(m_portName, m_serial.errorString()));
    }
}

void SerialLink::disconnectPort()
{
    if (!m_serial.isOpen())
        return;

    m_serial.close();
    emit closed();
}

void SerialLink::sendButtonEvent(quint8 buttonId, bool pressed)
{
    if (!m_serial.isOpen())
        return;

    quint8 state = pressed ? 1u : 0u;
    quint8 payloadLen = 2; // id + state
    quint8 frameLen = 1 + payloadLen; // TYPE + payload

    quint8 checksum = frameLen + MSG_BUTTON_EVENT + buttonId + state;

    QByteArray frame;
    frame.reserve(2 + 1 + frameLen + 1);

    frame.append(char(FRAME_HEADER_1));
    frame.append(char(FRAME_HEADER_2));
    frame.append(char(frameLen));
    frame.append(char(MSG_BUTTON_EVENT));
    frame.append(char(buttonId));
    frame.append(char(state));
    frame.append(char(checksum));

    m_serial.write(frame);
}

void SerialLink::handleReadyRead()
{
    const QByteArray data = m_serial.readAll();
    if (!data.isEmpty())
        emit rawBytes(data);

    for (quint8 b : data)
        processByte(b);
}

void SerialLink::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError)
        return;

    emit serialError(QStringLiteral("Serial error: %1").arg(m_serial.errorString()));

    if (m_serial.isOpen()) {
        m_serial.close();
        emit closed();
    }
}

void SerialLink::processByte(quint8 b)
{
    switch (m_state) {
    case WaitHeader1:
        if (b == FRAME_HEADER_1)
            m_state = WaitHeader2;
        break;

    case WaitHeader2:
        if (b == FRAME_HEADER_2) {
            m_state = WaitLen;
        } else {
            m_state = WaitHeader1;
        }
        break;

    case WaitLen:
        m_len = b;
        m_checksumAccum = m_len;
        m_state = WaitType;
        break;

    case WaitType:
        m_type = b;
        m_checksumAccum += m_type;
        m_payload.clear();
        m_state = (m_len > 1) ? WaitPayload : WaitChecksum;
        break;

    case WaitPayload:
        m_payload.append(char(b));
        m_checksumAccum += b;
        if (m_payload.size() >= m_len - 1) {
            m_state = WaitChecksum;
        }
        break;

    case WaitChecksum:
        if (m_checksumAccum == b) {
            handleFrame(m_type, m_payload);
        } else {
            qWarning() << "SerialLink checksum error";
        }
        m_state = WaitHeader1;
        break;
    }
}

void SerialLink::handleFrame(quint8 type, const QByteArray &payload)
{
    switch (type) {
    case MSG_BUTTON_EVENT:
        if (payload.size() >= 2) {
            int id = quint8(payload.at(0));
            bool pressed = (quint8(payload.at(1)) != 0);
            emit buttonState(id, pressed);
        }
        break;

    case MSG_ENCODER_VALUE:
        if (payload.size() >= 2) {
            int id = quint8(payload.at(0));
            int value = quint8(payload.at(1)); // 0..127
            emit rotaryValue(id, value);
        }
        break;

    default:
        qDebug() << "SerialLink: unknown frame type" << type << "len" << payload.size();
        break;
    }
}
