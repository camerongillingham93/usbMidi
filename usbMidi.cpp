/** Simple example of using USB MIDI 
 * 
 *  When the project boots up, a 100Hz sine wave will emit from both outputs,
 *  and the Daisy should appear as an Audio/MIDI device on a connected host.
 * 
 *  To keep the example short, only note on messages are handled, and there
 *  is only a single oscillator voice that tracks the most recent note message.
 */

#include "daisy_seed.h"
#include "daisysp.h"
#include <vector>

using namespace daisy;
using namespace daisysp;

DaisySeed      hw;
MidiUsbHandler midi;
Oscillator     osc;

// Vector to Store Midi note events
std::vector<daisy::NoteOnEvent> keys;

// Function Prototypes
void handleMidiMessage(daisy::MidiEvent m);
void keyPressed(daisy::NoteOnEvent on);
void keyReleased(daisy::NoteOnEvent off);
void setOsc();

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    // Set the oscillator frequency and amplitude based on MIDI note events
    setOsc();
    
    // Process the audio with the oscillator
    for(size_t i = 0; i < size; i++)
        out[0][i] = out[1][i] = osc.Process();
}

int main(void)
{
    /** Basic initialization of Daisy hardware */
    hw.Configure();
    hw.Init();

    /** Initialize USB Midi 
     *  By default, this is set to use the built-in (USB FS) peripheral.
     *  By setting midi_cfg.transport_config.periph = MidiUsbTransport::Config::EXTERNAL,
     *  the USB HS pins can be used (as FS) for MIDI 
     */
    MidiUsbHandler::Config midi_cfg;
    midi_cfg.transport_config.periph = MidiUsbTransport::Config::INTERNAL;
    midi.Init(midi_cfg);
    midi.StartReceive();

    /** Initialize our test tone */
    osc.Init(hw.AudioSampleRate());

    /** Start the audio callback */
    hw.StartAudio(AudioCallback);

    

    while(1)
    {
        /** Listen to MIDI for new changes */
        midi.Listen();

        while(midi.HasEvents())
        {
            // Handle MIDI events
            handleMidiMessage(midi.PopEvent());
        }
    }
}

// Function Definitions

void handleMidiMessage(daisy::MidiEvent m)
{
    switch(m.type)
    {
        // Handle Note On events
        case daisy::NoteOn:
        {
            keyPressed(m.AsNoteOn());
        }
        break;
        // Handle Note Off events
        case NoteOff:
        {
            keyReleased(m.AsNoteOn());
        }
        break;
        default: break;
    }
}

void keyPressed(daisy::NoteOnEvent on)
{
    // Store the Note On event in the keys vector
    keys.push_back(on);
}

void keyReleased(daisy::NoteOnEvent off)
{
    keys.erase(std::remove_if(keys.begin(),
                              keys.end(),
                              [off](const daisy::NoteOnEvent& k)
                              { return k.note == off.note; }),
               keys.end());
}


void setOsc()
{
    // Iterate through the stored Note On events and set the oscillator parameters
    for(const auto& key : keys)
    {
        // Accessing the note number of the key
        int noteNumber = key.note;

        // Accessing the velocity/amplitude of the key
        float amplitude = static_cast<float>(key.velocity) / 127.0f;

        // Set the oscillator frequency and amplitude
        osc.SetFreq(mtof(noteNumber));
        osc.SetAmp(amplitude);
    }
}

