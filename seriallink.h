#pragma once

#include <QObject>
#include <QSerialPort>

class SerialLink : public QObject
{
    Q_OBJECT
public:
    explicit SerialLink(QObject *parent = nullptr);

    bool open(const QString &portName, int baud = 115200);
    void close();
    bool isOpen() const { return m_serial.isOpen(); }

    void sendButtonEvent(quint8 buttonId, bool pressed);
    void sendEncoderDelta(quint8 encoderId, quint8 value); // 0..127

signals:
    void rotaryDelta(int id, int value);      // value is absolute 0..127
    void buttonState(int id, bool pressed);
    void errorOccurred(const QString &msg);
    void opened(const QString &portName);
    void closed();

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);

private:
    void processByte(quint8 b);
    void handleFrame(quint8 type, const quint8 *payload, quint8 payloadLen);

    QSerialPort m_serial;

    enum FrameState {
        WaitHeader1,
        WaitHeader2,
        WaitLen,
        WaitBody,
        WaitChecksum
    };
    FrameState m_state;
    quint8     m_len;
    quint8     m_buf[64];
    quint8     m_pos;
    quint8     m_checksum;
};
