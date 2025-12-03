#pragma once

#include <QObject>
#include <QVector>
#include <QVariantList>
#include <QStringList>
#include <QColor>

class SerialLink;

class Controller : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList buttonTexts   READ buttonTexts   NOTIFY layoutChanged)
    Q_PROPERTY(QVariantList buttonColors  READ buttonColors  NOTIFY layoutChanged)
    Q_PROPERTY(QVariantList labelTexts    READ labelTexts    NOTIFY layoutChanged)
    Q_PROPERTY(QVariantList rotaryValues  READ rotaryValues  NOTIFY rotaryValuesChanged)
    Q_PROPERTY(bool         connected     READ connected     NOTIFY connectedChanged)
    Q_PROPERTY(QString      currentSetName READ currentSetName NOTIFY currentSetChanged)
    Q_PROPERTY(int          currentSet    READ currentSet    WRITE setCurrentSet NOTIFY currentSetChanged)
    Q_PROPERTY(int          setCount      READ setCount      NOTIFY setCountChanged)

public:
    explicit Controller(QObject *parent = nullptr);

    // QML-accessible
    Q_INVOKABLE void sendButton(int index, bool pressed);

    QVariantList buttonTexts() const;
    QVariantList buttonColors() const;
    QVariantList labelTexts() const;
    QVariantList rotaryValues() const;

    bool connected() const { return m_connected; }

    int currentSet() const { return m_currentSet; }
    void setCurrentSet(int index);

    int setCount() const { return m_sets.size(); }
    QString currentSetName() const;

signals:
    void layoutChanged();
    void rotaryValuesChanged();
    void connectedChanged();
    void currentSetChanged();
    void setCountChanged();

    void logMessage(const QString &msg);

private slots:
    void onOpened(const QString &port);
    void onClosed();
    void onSerialError(const QString &msg);
    void onButtonState(int id, bool pressed);
    void onRotaryValue(int id, int value);

private:
    void loadProfiles(const QString &filePath);

    struct ButtonDef {
        QString text;
        QColor  color;
    };
    struct EncoderDef {
        QString accel;  // not used yet but fine to keep
    };
    struct ProfileSet {
        QString            name;
        QVector<ButtonDef> buttons;
        QStringList        labels;
        QVector<EncoderDef> encoders; // unused now but ready
    };

    QVector<ProfileSet> m_sets;
    int m_currentSet = 0;

    QVector<int> m_rotaryVals; // size 8
    bool m_connected = false;

    SerialLink *m_serialLink;
};
