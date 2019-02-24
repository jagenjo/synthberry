#ifndef AUDIOIO_H
#define AUDIOIO_H

#include "soundio/soundio.h"
#include "RtMidi.h"

#include "includes.h"

void write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max);

int initSoundIO();
void listAudioDevices();
int openAudioDevice( int i, void (*myfunc)(struct SoundIoOutStream *outstream, int, int) );
int closeAudioDevice();

void initRTMIDI();
void listMIDIDevices();
int openMIDIPort( int i, void (*mycallback)(double, std::vector< unsigned char > *, void *) );
int closeMIDIPort();


#endif