/*
 ******************************************************************************

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation, either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, see <http://www.gnu.org/licenses/>.

 This library is pulse sensor density modules

 ******************************************************************************
 */
#ifndef PULSESENSOR_H
#define PULSESENSOR_H

#include "Arduino.h"

class PulseSensor
{
    public:
        PulseSensor(int pulsePin = A0);
        void begin(void);
        void Run();
        void GetBPM(int &bpm);
        int  GetBPM(void);

        void SerialOutputWhenBeatHappens();
        void SendDataToSerial(char symbol, int data );
        void SerialOutput();
        void SerialMonitorVisual(int data );
        void PulseISR(void);            // Pusle interrupt service Routine



    private:

        int _pulsePin;
        volatile int _BPM;                   // used to hold the pulse rate

        volatile unsigned long _sampleCounter;          // used to determine pulse timing
        volatile unsigned long _lastBeatTime;            // used to find IBI
        volatile int _thresh;                // used to find instant moment of heart beat, seeded
        volatile int _Peak;              // used to find peak in pulse wave, seeded
        volatile int _Trough;          // used to find trough in pulse wave, seeded
        volatile int _signal;                // holds the incoming raw data
        volatile int _IBI;                   // holds the time between beats, must be seeded!
        volatile int _amp;                   // used to hold amplitude of pulse waveform, seeded
        volatile int _rate[10];              // array to hold last ten IBI values

        volatile boolean _Pulse;             // true when pulse wave is high, false when it's low
        volatile boolean _QS;                // becomes true when Atom finds a beat.
        volatile boolean _firstBeat;         // used to seed rate array so we startup with reasonable BPM
        volatile boolean _secondBeat;        // used to seed rate array so we startup with reasonable BPM
};

#endif






