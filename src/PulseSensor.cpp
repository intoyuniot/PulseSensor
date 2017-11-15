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


#include "PulseSensor.h"
// IntervalTimer uses hardware timers that are otherwise allocated for PIN functions (ADC/PWM)
// The first allocated timer is TIMR2 which is assigned to A0 & A1 ADC/PWM channels
// So use A2 (though A0 & A1 ADC should still work) for pulse input, D7 (onboard LED) for blink LED and D6 for fancy LED
//  VARIABLES

static bool serialVisual = false;     // Set to 'false' by Default.  Re-set to 'true' to see Arduino Serial Monitor ASCII Visual Pulse


PulseSensor::PulseSensor(int pulsePin)
{
    _pulsePin = pulsePin;

    _sampleCounter = 0;
    _lastBeatTime = 0;
    _thresh = 2048;
    _Peak =2048;
    _Trough = 2048;
    _IBI = 600;
    _amp = 410;
    _Pulse = false;
    _QS = false;
    _firstBeat = true;
    _secondBeat = false;

}

void PulseSensor::begin(void)
{
    pinMode(_pulsePin,OUTPUT);
}


// THIS IS THE TIMER 2 INTERRUPT SERVICE ROUTINE.
// Timer 2 makes sure that we take a reading every 2 miliseconds
void PulseSensor::PulseISR(void)
{
    noInterrupts();
    _signal = analogRead(_pulsePin);              // read the Pulse Sensor
    _sampleCounter += 2;                         // keep track of the time in mS with this variable
    int N = _sampleCounter - _lastBeatTime;       // monitor the time since the last beat to avoid noise

    //  find the peak and trough of the pulse wave
    if(_signal < _thresh && N > (_IBI/5)*3)
    {       // avoid dichrotic noise by waiting 3/5 of last IBI
        if (_signal < _Trough)
        {                        // _T is the trough
            _Trough = _signal;                         // keep track of lowest point in pulse wave
        }
    }

    if(_signal > _thresh && _signal > _Peak)
    {          // thresh condition helps avoid noise
        _Peak = _signal;                             // _P is the peak
    }                                        // keep track of highest point in pulse wave

    //  NOW IT'S TIME TO LOOK FOR THE HEART BEAT
    // _signal surges up in value every time there is a pulse
    if (N > 250)
    {

        // avoid high frequency noise
        if ( (_signal > _thresh) && (_Pulse == false) && (N > (_IBI/5)*3) )
        {
            _Pulse = true;                               // set the Pulse flag when we think there is a pulse
            _IBI = _sampleCounter - _lastBeatTime;         // measure time between beats in mS
            _lastBeatTime = _sampleCounter;               // keep track of time for next pulse

            if(_secondBeat)
            {                        // if this is the second beat, if _secondBeat == TRUE
                _secondBeat = false;                  // clear _secondBeat flag
                for(int i=0; i<=9; i++)
                {             // seed the running total to get a realisitic BPM at startup
                    _rate[i] = _IBI;
                }
            }

            if(_firstBeat)
            {                         // if it's the first time we found a beat, if _firstBeat == TRUE
                _firstBeat = false;                   // clear _firstBeat flag
                _secondBeat = true;                   // set the _second beat flag
                interrupts();
                return;                              // IBI value is unreliable so discard it
            }


            // keep a running total of the last 10 IBI values
            uint16_t runningTotal = 0;              // clear the runningTotal variable

            for(int i=0; i<=8; i++)
            {                // shift data in the rate array
                _rate[i] = _rate[i+1];                  // and drop the oldest IBI value
                runningTotal += _rate[i];              // add up the 9 oldest IBI values
            }

            _rate[9] = _IBI;                          // add the latest IBI to the rate array
            runningTotal += _rate[9];                // add the latest IBI to runningTotal
            runningTotal /= 10;                     // average the last 10 IBI values
            _BPM = 60000/runningTotal;               // how many beats can fit into a minute? that's BPM!
            _QS = true;                              // set Quantified Self flag
            // QS FLAG IS NOT CLEARED INSIDE THIS ISR
        }
    }

    if (_signal < _thresh && _Pulse == true)
    {   // when the values are going down, the beat is over
        _Pulse = false;                         // reset the Pulse flag so we can do it again
        _amp = _Peak - _Trough;                           // get amplitude of the pulse wave
        _thresh = _amp/2 + _Trough;                    // set thresh at 50% of the amplitude
        _Peak = _thresh;                            // reset these for next time
        _Trough = _thresh;
    }

    if (N > 2500)
    {                           // if 2.5 seconds go by without a beat
        _thresh = 512;                          // set thresh default
        _Peak = 512;                               // set P default
        _Trough = 512;                               // set T default
        _lastBeatTime = _sampleCounter;          // bring the lastBeatTime up to date
        _firstBeat = true;                      // set these to avoid noise
        _secondBeat = false;                    // when we get the heartbeat back
    }

    interrupts();
}// end isr


void PulseSensor::Run()
{
    SerialOutput();
    if (_QS == true)
    {     //  A Heartbeat Was Found
        // BPM and IBI have been Determined
        // Quantified Self "QS" true when arduino finds a heartbeat
        SerialOutputWhenBeatHappens();   // A Beat Happened, Output that to serial.
        _QS = false;                      // reset the Quantified Self flag for next time
    }
    delay(20);                             //  take a break
}

void PulseSensor::GetBPM(int &bpm)
{
    Run();
    bpm = _BPM;
}

int PulseSensor::GetBPM(void)
{
    Run();
    return _BPM;
}


//  Decides How To OutPut BPM and IBI Data
void PulseSensor::SerialOutputWhenBeatHappens()
{
    if (serialVisual == true)
    {            //  Code to Make the Serial Monitor Visualizer Work
        SerialUSB.print("*** Heart-Beat Happened *** ");  //ASCII Art Madness
        SerialUSB.print("BPM: ");
        SerialUSB.print(_BPM);
        SerialUSB.print("  ");
    }
    else
    {
        SendDataToSerial('B',_BPM);   // send heart rate with a 'B' prefix
        SendDataToSerial('Q',_IBI);   // send time between beats with a 'Q' prefix
    }
}



//  Sends Data to Pulse Sensor Processing App, Native Mac App, or Third-party Serial Readers.
void PulseSensor::SendDataToSerial(char symbol, int data )
{
    SerialUSB.print(symbol);
    SerialUSB.println(data);
}

//////////
/////////  All Serial Handling Code,
/////////  It's Changeable with the 'serialVisual' variable
/////////  Set it to 'true' or 'false' when it's declared at start of code.
/////////

void PulseSensor::SerialOutput()
{   // Decide How To Output Serial.
    if (serialVisual == true)
    {
        SerialMonitorVisual(_BPM);   // goes to function that makes Serial Monitor Visualizer
    }
    else
    {
        //SendDataToSerial('S', _BPM);     // goes to sendDataToSerial function
    }
}


//  Code to Make the Serial Monitor Visualizer Work
void PulseSensor::SerialMonitorVisual(int data )
{
    const int sensorMin = 0;      // sensor minimum, discovered through experiment
    const int sensorMax = 1024;    // sensor maximum, discovered through experiment

    int sensorReading = data;
    // map the sensor range to a range of 12 options:
    int range = map(sensorReading, sensorMin, sensorMax, 0, 11);

    // do something different depending on the
    // range value:
    switch (range)
    {
        case 0:
            SerialUSB.println("");     //ASCII Art Madness
            break;
        case 1:
            SerialUSB.println("---");
            break;
        case 2:
            SerialUSB.println("------");
            break;
        case 3:
            SerialUSB.println("---------");
            break;
        case 4:
            SerialUSB.println("------------");
            break;
        case 5:
            SerialUSB.println("--------------|-");
            break;
        case 6:
            SerialUSB.println("--------------|---");
            break;
        case 7:
            SerialUSB.println("--------------|-------");
            break;
        case 8:
            SerialUSB.println("--------------|----------");
            break;
        case 9:
            SerialUSB.println("--------------|----------------");
            break;
        case 10:
            SerialUSB.println("--------------|-------------------");
            break;
        case 11:
            SerialUSB.println("--------------|-----------------------");
            break;

    }
}


