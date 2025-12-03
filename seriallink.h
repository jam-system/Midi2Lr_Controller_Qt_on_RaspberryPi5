#pragma once

#include <QObject>
#include <QSerialPort>

class SerialLink : public QObject
{
    Q_OBJECT
public:
    explicit SerialLink(QObject *parent = nullptr);

    void setPortName(const QString &name) { m_portName = name; }
    QString portName() const { return m_portName; }

    void setBaudRate(int baud) { m_baudRate = baud; }
    int baudRate() const { return m_baudRate; }

    Q_INVOKABLE void connectPort();
    Q_INVOKABLE void disconnectPort();

    // Pi -> Pico: send button pressed / released
    Q_INVOKABLE void sendButtonEvent(quint8 buttonId, bool pressed);

signals:
    void opened(const QString &portName);
    void closed();
    void serialError(const QString &message);

    // Pico -> Pi
    void buttonState(int id, bool pressed);
    void rotaryValue(int id, int value);  // value 0..127
    void rawBytes(const QByteArray &bytes);

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);

private:
    void processByte(quint8 b);
    void handleFrame(quint8 type, const QByteArray &payload);

    QSerialPort m_serial;
    QString m_portName;
    int m_baudRate;

    enum RxState {
        WaitHeader1,
        WaitHeader2,
        WaitLen,
        WaitType,
        WaitPayload,
        WaitChecksum
    };

    RxState m_state;
    quint8 m_len;
    quint8 m_type;
    QByteArray m_payload;
    quint8 m_checksumAccum;
};
