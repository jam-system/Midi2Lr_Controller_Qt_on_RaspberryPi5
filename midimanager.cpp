
#include "midimanager.h"
#include <QMetaObject>
#include <QDebug>

MidiManager::MidiManager(QObject *parent)
    : QObject(parent)
{
    try {
        m_in  = std::make_unique<RtMidiIn>(RtMidi::UNSPECIFIED,  "QtMidiIn");
        m_out = std::make_unique<RtMidiOut>(RtMidi::UNSPECIFIED, "QtMidiOut");
        m_in->ignoreTypes(false, false, true); // don't ignore notes
    } catch (RtMidiError &e) {
        qWarning() << "RtMidi init error:" << e.getMessage().c_str();
    }
}

MidiManager::~MidiManager()
{
    if (m_in && m_in->isPortOpen())  m_in->closePort();
    if (m_out && m_out->isPortOpen()) m_out->closePort();
}

bool MidiManager::openPort(const std::string &nameSubstring)
{
    if (!m_in || !m_out) return false;

    // ---- OUT ----
    int outIndex = -1;
    unsigned int outCount = m_out->getPortCount();
    for (unsigned int i = 0; i < outCount; ++i) {
        std::string name = m_out->getPortName(i);
        if (name.find(nameSubstring) != std::string::npos) {
            outIndex = static_cast<int>(i);
            qDebug() << "Using MIDI OUT port" << i << ":" << name.c_str();
            break;
        }
    }
    if (outIndex < 0 && outCount > 0) {
        outIndex = 0;
        qWarning() << "BLE port not found, using first OUT port:"
                   << m_out->getPortName(0).c_str();
    }
    if (outIndex >= 0) {
        try {
            m_out->openPort(static_cast<unsigned int>(outIndex));
        } catch (RtMidiError &e) {
            qWarning() << "Failed to open MIDI OUT:" << e.getMessage().c_str();
        }
    }

    // ---- IN ----
    int inIndex = -1;
    unsigned int inCount = m_in->getPortCount();
    for (unsigned int i = 0; i < inCount; ++i) {
        std::string name = m_in->getPortName(i);
        if (name.find(nameSubstring) != std::string::npos) {
            inIndex = static_cast<int>(i);
            qDebug() << "Using MIDI IN port" << i << ":" << name.c_str();
            break;
        }
    }
    if (inIndex < 0 && inCount > 0) {
        inIndex = 0;
        qWarning() << "BLE port not found, using first IN port:"
                   << m_in->getPortName(0).c_str();
    }
    if (inIndex >= 0) {
        try {
            m_in->openPort(static_cast<unsigned int>(inIndex));
            m_in->setCallback(&MidiManager::midiInCallback, this);
        } catch (RtMidiError &e) {
            qWarning() << "Failed to open MIDI IN:" << e.getMessage().c_str();
        }
    }

    bool ok = isOpen();
    qDebug() << "MIDI open:" << ok;
    return ok;
}

bool MidiManager::isOpen() const
{
    return m_in && m_in->isPortOpen() && m_out && m_out->isPortOpen();
}

void MidiManager::sendNoteOn(int channel, int note, int velocity)
{
    if (!m_out || !m_out->isPortOpen()) return;

    std::vector<unsigned char> msg(3);
    msg[0] = static_cast<unsigned char>(0x90 | ((channel - 1) & 0x0F));
    msg[1] = static_cast<unsigned char>(note & 0x7F);
    msg[2] = static_cast<unsigned char>(velocity & 0x7F);
    try {
        m_out->sendMessage(&msg);
    } catch (RtMidiError &e) {
        qWarning() << "MIDI send error:" << e.getMessage().c_str();
    }
}

void MidiManager::sendNoteOff(int channel, int note, int velocity)
{
    if (!m_out || !m_out->isPortOpen()) return;

    std::vector<unsigned char> msg(3);
    msg[0] = static_cast<unsigned char>(0x80 | ((channel - 1) & 0x0F));
    msg[1] = static_cast<unsigned char>(note & 0x7F);
    msg[2] = static_cast<unsigned char>(velocity & 0x7F);
    try {
        m_out->sendMessage(&msg);
    } catch (RtMidiError &e) {
        qWarning() << "MIDI send error:" << e.getMessage().c_str();
    }
}

// static
void MidiManager::midiInCallback(double,
                                 std::vector<unsigned char> *message,
                                 void *userData)
{
    auto *self = static_cast<MidiManager *>(userData);
    if (!self || !message) return;

    auto msgCopy = *message;
    QMetaObject::invokeMethod(
        self,
        [self, msgCopy]() { self->handleIncoming(msgCopy); },
        Qt::QueuedConnection
    );
}

void MidiManager::handleIncoming(const std::vector<unsigned char> &msg)
{
    if (msg.size() < 3) return;

    unsigned char status = msg[0];
    unsigned char type   = status & 0xF0;
    int channel          = (status & 0x0F) + 1;
    int d1 = msg[1];
    int d2 = msg[2];

    if (type == 0x90 && d2 != 0) {
        emit noteOnReceived(channel, d1, d2);
    } else if (type == 0x80 || (type == 0x90 && d2 == 0)) {
        emit noteOffReceived(channel, d1, d2);
    }
}
