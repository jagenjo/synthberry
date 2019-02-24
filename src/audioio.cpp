#include "audioio.h"

RtMidiIn *midiin = 0;


struct SoundIo *soundio = NULL;
struct SoundIoDevice *device = NULL;
struct SoundIoOutStream *outstream = NULL;

int initSoundIO()
{
	int err;

	if (soundio)
		return 0;

	soundio = soundio_create();
	if (!soundio) {
		fprintf(stderr, "out of memory\n");
		return 1;
	}

	if ((err = soundio_connect(soundio))) {
		fprintf(stderr, "error connecting: %s\n", soundio_strerror(err));
		return 1;
	}
	soundio_flush_events(soundio);

	return 0;
}

void listAudioDevices()
{
	initSoundIO();

	int num = soundio_output_device_count(soundio);
	if (num < 1)
	{
		std::cout << "no devices found" << std::endl;
		return;
	}

	for (int i = 0; i < num; ++i)
	{
		SoundIoDevice* device = soundio_get_output_device(soundio, i);
		std::cout << "\t" << i << ".- " << device->name << " :: " << device->software_latency_current << std::endl;
	}
}


static void underflow_func( struct SoundIoOutStream *outstream ) {
	static int count = 0;
	fprintf(stderr, "underflow %d\n", ++count);
}


int openAudioDevice( int device_index, void (*myfunc)(struct SoundIoOutStream *,int,int) )
{
    int err;
	initSoundIO();

	if( device_index == -1 )
	{
		device_index = soundio_default_output_device_index(soundio);
		if (device_index < 0) {
			fprintf(stderr, "no output device found\n");
			return 1;
		}
	}    
	
	//std::cout << "Output device index: " << device_index << std::endl;

    device = soundio_get_output_device( soundio, device_index );
    if (!device) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    fprintf(stderr, "Output device: %s\n", device->name);

	outstream = soundio_outstream_create(device);
    if (!outstream) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }
    outstream->format = SoundIoFormatFloat32NE;
	std::cout << "Layout: " << device->layouts[0].name << std::endl;
	outstream->layout = device->layouts[0];

	outstream->write_callback = myfunc;
	outstream->underflow_callback = underflow_func;

    if ((err = soundio_outstream_open(outstream))) {
        fprintf(stderr, "unable to open device: %s", soundio_strerror(err));
        return 1;
    }

    if (outstream->layout_error)
        fprintf(stderr, "unable to set channel layout: %s\n", soundio_strerror(outstream->layout_error));

    if ((err = soundio_outstream_start(outstream))) {
        fprintf(stderr, "unable to start device: %s\n", soundio_strerror(err));
        return 1;
    }

	/*
    double latency;
    if ((err = soundio_outstream_get_latency(outstream, &latency))) {
 	 std::cout << "no latency info: " << soundio_strerror(err) << std::endl;
    }
    else
        std::cout << "Latency info: " << latency << std::endl;
	*/

    return 0;
}

int closeAudioDevice()
{
	
	printf("app end");
    soundio_outstream_destroy(outstream);
    soundio_device_unref(device);
    soundio_destroy(soundio);
	
    return 0;
}



void initRTMIDI()
{
	if (midiin)
		return;

	midiin = new RtMidiIn();
}


void listMIDIDevices()
{
	initRTMIDI();

	unsigned int i = 0, nPorts = midiin->getPortCount();
	if (nPorts == 0) {
		std::cout << "No MIDI input ports available" << std::endl;
		return;
	}

	std::string portName;
	for (i = 0; i<nPorts; i++) {
		portName = midiin->getPortName(i);
		std::cout << "\t" << i << ": " << portName << '\n';
	}
}

int openMIDIPort( int index, void (*mycallback)(double, std::vector< unsigned char > *, void *) )
{
    initRTMIDI();
    
	//rtmidi->openVirtualPort();

	std::string portName;
	unsigned int i = 0, nPorts = midiin->getPortCount();
	if ( nPorts == 0 ) {
		std::cout << "No input ports available!" << std::endl;
		return 1;
	}

	if (nPorts <= index) {
		std::cout << "Port not found!" << std::endl;
		return 1;
	}

	portName = midiin->getPortName(index);
	std::cout << "Input MIDI: " << portName << '\n';

	midiin->openPort( index );    

    // Set our callback function.  This should be done immediately after
    // opening the port to avoid having incoming messages written to the
    // queue instead of sent to the callback function.
    midiin->setCallback( mycallback );

    // Don't ignore sysex, timing, or active sensing messages.
    midiin->ignoreTypes( false, false, false );
	return 0;
}

int closeMIDIPort()
{
	return 1;
}




