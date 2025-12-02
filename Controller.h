#pragma once

#include <QObject>
#include <QVariantList>
#include <QVector>
#include <QColor>
#include "seriallink.h"

class Controller : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList buttonStates READ buttonStates NOTIFY buttonStatesChanged)
    Q_PROPERTY(QVariantList rotaryValues READ rotaryValues NOTIFY rotaryValuesChanged)
    Q_PROPERTY(QString portName READ portName WRITE setPortName NOTIFY portNameChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
	
	Q_PROPERTY(QVariantList buttonTexts READ buttonTexts NOTIFY layoutChanged)
	Q_PROPERTY(QVariantList buttonColors READ buttonColors NOTIFY layoutChanged)
	Q_PROPERTY(QVariantList labelTexts READ labelTexts NOTIFY layoutChanged)

	Q_PROPERTY(int currentSet READ currentSet WRITE setCurrentSet NOTIFY currentSetChanged)
	Q_PROPERTY(int setCount READ setCount NOTIFY setCountChanged)
	Q_PROPERTY(QString currentSetName READ currentSetName NOTIFY currentSetChanged)

public:
    explicit Controller(QObject *parent = nullptr);

    QVariantList buttonStates() const;
    QVariantList rotaryValues() const;

    QString portName() const { return m_portName; }
    void setPortName(const QString &name);

    bool isConnected() const { return m_serial.isOpen(); }

    Q_INVOKABLE void connectSerial();
    Q_INVOKABLE void sendButton(int id, bool pressed);
    
    // profile accessors for QML
    QVariantList buttonTexts() const;
    QVariantList buttonColors() const;
    QVariantList labelTexts() const;

    int currentSet() const { return m_currentSet; }
    void setCurrentSet(int index);

    int setCount() const { return m_sets.size(); }
    QString currentSetName() const;



signals:
    void buttonStatesChanged();
    void rotaryValuesChanged();
    void portNameChanged();
    void connectedChanged();
    void logMessage(const QString &msg);
    
    // new:
    void layoutChanged();
    void currentSetChanged();
    void setCountChanged();


private slots:
    void onButtonState(int id, bool pressed);
    void onRotaryDelta(int id, int delta);
    void onRawLine(const QString &line);      // <--- add back to match moc
    void onSerialError(const QString &msg);
    void onOpened(const QString &portName);
    void onClosed();

private:
    SerialLink   m_serial;
    QVector<int> m_buttonStates;   // 0/1
    QVector<int> m_rotaryValues;   // accumulated deltas
    QString      m_portName;
    
    struct ButtonDef {
        QString text;
        QColor  color;
    };
    struct ProfileSet {
        QString            name;
        QVector<ButtonDef> buttons;
        QStringList        labels;
    };

    QVector<ProfileSet> m_sets;
    int m_currentSet = 0;

    void loadProfiles(const QString &filePath);

};
