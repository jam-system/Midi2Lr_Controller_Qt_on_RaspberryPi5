#pragma once

#include <QObject>
#include <memory>
#include <vector>
#include <string>

#include "RtMidi.h"

class MidiManager : public QObject
{
    Q_OBJECT
public:
    explicit MidiManager(QObject *parent = nullptr);
    ~MidiManager();

    bool openPort(const std::string &nameSubstring = "RPi-BLE-MIDI");
    bool isOpen() const;

    void sendNoteOn(int channel, int note, int velocity);
    void sendNoteOff(int channel, int note, int velocity);

signals:
    void noteOnReceived(int channel, int note, int velocity);
    void noteOffReceived(int channel, int note, int velocity);

private:
    std::unique_ptr<RtMidiIn>  m_in;
    std::unique_ptr<RtMidiOut> m_out;

    static void midiInCallback(double timeStamp,
                               std::vector<unsigned char> *message,
                               void *userData);
    void handleIncoming(const std::vector<unsigned char> &msg);
};
