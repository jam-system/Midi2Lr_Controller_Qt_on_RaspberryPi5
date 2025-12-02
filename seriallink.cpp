#include "seriallink.h"
#include <QDebug>
#include <QStringList>

// --- Protocol constants ---
static const quint8 FRAME_HEADER_1   = 0xAA;
static const quint8 FRAME_HEADER_2   = 0x55;

static const quint8 MSG_BUTTON_EVENT = 0x01;
static const quint8 MSG_ENCODER_DELTA = 0x02;   // using delta name, but it's absolute value

SerialLink::SerialLink(QObject *parent)
    : QObject(parent)
{
    connect(&m_serial, &QSerialPort::readyRead,
            this, &SerialLink::handleReadyRead);
    connect(&m_serial, &QSerialPort::errorOccurred,
            this, &SerialLink::handleError);

    // init parser state
    m_state    = WaitHeader1;
    m_len      = 0;
    m_pos      = 0;
    m_checksum = 0;
}

bool SerialLink::open(const QString &portName, int baud)
{
    if (m_serial.isOpen())
        m_serial.close();

    m_serial.setPortName(portName);
    m_serial.setBaudRate(baud);
    m_serial.setDataBits(QSerialPort::Data8);
    m_serial.setParity(QSerialPort::NoParity);
    m_serial.setStopBits(QSerialPort::OneStop);
    m_serial.setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serial.open(QIODevice::ReadWrite)) {
        const QString msg = QStringLiteral("Serial open failed for %1: %2")
                                .arg(portName, m_serial.errorString());
        qWarning() << msg;
        emit errorOccurred(msg);
        return false;
    }

    qDebug() << "SerialLink opened:" << portName << "@" << baud;

    // reset parser
    m_state    = WaitHeader1;
    m_len      = 0;
    m_pos      = 0;
    m_checksum = 0;

    emit opened(portName);
    return true;
}

void SerialLink::close()
{
    if (m_serial.isOpen()) {
        m_serial.close();
        emit closed();
    }
}

// --- Pi <- Pico: read incoming bytes ---

void SerialLink::handleReadyRead()
{
    QByteArray data = m_serial.readAll();

    // RAW HEX DEBUG
    if (!data.isEmpty()) {
        QStringList hexBytes;
        hexBytes.reserve(data.size());
        for (char c : data) {
            unsigned char b = static_cast<unsigned char>(c);
            hexBytes << QStringLiteral("%1")
                        .arg(b, 2, 16, QLatin1Char('0'))
                        .toUpper();
        }
        qDebug() << "SerialLink RAW bytes:" << hexBytes.join(' ');
    }

    for (char c : data) {
        processByte(static_cast<quint8>(c));
    }
}

void SerialLink::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError)
        return;
    const QString msg = QStringLiteral("Serial error: %1")
                            .arg(m_serial.errorString());
    qWarning() << msg;
    emit errorOccurred(msg);
}

// --- Pi -> Pico: send button & encoder commands ---

void SerialLink::sendButtonEvent(quint8 buttonId, bool pressed)
{
    if (!m_serial.isOpen())
        return;

    quint8 payload[2];
    payload[0] = buttonId;
    payload[1] = pressed ? quint8(1) : quint8(0);

    quint8 frameLen = 1 + 2; // TYPE + 2 payload bytes

    quint8 checksum =
        frameLen +
        MSG_BUTTON_EVENT +
        payload[0] +
        payload[1];

    QByteArray frame;
    frame.reserve(2 + 1 + frameLen + 1);

    frame.append(char(FRAME_HEADER_1));
    frame.append(char(FRAME_HEADER_2));
    frame.append(char(frameLen));
    frame.append(char(MSG_BUTTON_EVENT));
    frame.append(char(payload[0]));
    frame.append(char(payload[1]));
    frame.append(char(checksum));

    m_serial.write(frame);
}

void SerialLink::sendEncoderDelta(quint8 encoderId, quint8 value)
{
    if (!m_serial.isOpen())
        return;

    quint8 payload[2];
    payload[0] = encoderId;
    payload[1] = value;   // 0..127

    quint8 frameLen = 1 + 2; // TYPE + 2 payload bytes

    quint8 checksum =
        frameLen +
        MSG_ENCODER_DELTA +
        payload[0] +
        payload[1];

    QByteArray frame;
    frame.reserve(2 + 1 + frameLen + 1);

    frame.append(char(FRAME_HEADER_1));
    frame.append(char(FRAME_HEADER_2));
    frame.append(char(frameLen));
    frame.append(char(MSG_ENCODER_DELTA));
    frame.append(char(payload[0]));
    frame.append(char(payload[1]));
    frame.append(char(checksum));

    m_serial.write(frame);
}

// --- Parser state machine: THIS is the complete switch ---

void SerialLink::processByte(quint8 b)
{
    switch (m_state) {

    case WaitHeader1:
        if (b == FRAME_HEADER_1) {
            m_state = WaitHeader2;
            // qDebug() << "Got header1";
        }
        break;

    case WaitHeader2:
        if (b == FRAME_HEADER_2) {
            m_state = WaitLen;
            // qDebug() << "Got header2";
        } else {
            // Not our header; restart
            m_state = WaitHeader1;
        }
        break;

    case WaitLen:
        m_len = b;
        if (m_len == 0 || m_len > sizeof(m_buf)) {
            // invalid length, resync
            qWarning() << "SerialLink: invalid LEN" << m_len;
            m_state = WaitHeader1;
        } else {
            m_pos = 0;
            m_checksum = m_len;  // include LEN in checksum
            m_state = WaitBody;
        }
        break;

    case WaitBody:
        m_buf[m_pos++] = b;
        m_checksum += b;

        if (m_pos >= m_len) {
            m_state = WaitChecksum;
        }
        break;

    case WaitChecksum:
        {
            quint8 received = b;
            quint8 expected = m_checksum & 0xFF;

            if (received == expected) {
                // full frame is valid
                // build a hex string for debugging
                QByteArray frame;
                frame.reserve(2 + 1 + m_len + 1);
                frame.append(char(FRAME_HEADER_1));
                frame.append(char(FRAME_HEADER_2));
                frame.append(char(m_len));
                frame.append(reinterpret_cast<const char*>(m_buf), m_len);
                frame.append(char(received));

                QStringList hexBytes;
                for (int i = 0; i < frame.size(); ++i) {
                    unsigned char byte = static_cast<unsigned char>(frame.at(i));
                    hexBytes << QStringLiteral("%1")
                                   .arg(byte, 2, 16, QLatin1Char('0'))
                                   .toUpper();
                }
                qDebug() << "SerialLink RX frame:" << hexBytes.join(' ');

                // now decode content
                quint8 type       = m_buf[0];
                quint8 payloadLen = m_len - 1;
                const quint8 *payload = m_buf + 1;
                handleFrame(type, payload, payloadLen);
            } else {
                qWarning() << "SerialLink: checksum error, got"
                           << QString::number(received, 16)
                           << "expected"
                           << QString::number(expected, 16);
            }

            // ready for next frame
            m_state = WaitHeader1;
        }
        break;
    }
}

// --- Interpret full frames ---

void SerialLink::handleFrame(quint8 type, const quint8 *payload, quint8 payloadLen)
{
    switch (type) {
    case MSG_BUTTON_EVENT:
        if (payloadLen >= 2) {
            int id = payload[0];
            bool pressed = payload[1] != 0;
            emit buttonState(id, pressed);
            // qDebug() << "BUTTON_EVENT id" << id << "pressed" << pressed;
        }
        break;

    case MSG_ENCODER_DELTA:   // here "delta" means encoder message; value is absolute
        if (payloadLen >= 2) {
            int id    = payload[0];
            int value = payload[1];   // 0..127
            emit rotaryDelta(id, value);
            // qDebug() << "ENCODER value id" << id << "value" << value;
        }
        break;

    default:
        qWarning() << "SerialLink: unknown frame type:" << type;
        break;
    }
}

