#include "audioio.h"
#include <algorithm>
#include <string>

bool must_exit = false;
int frame = 0;
static float seconds_offset = 0.0f;
float pitch = 440.0f;
double phase = 0.0f;
float amplitude = 0.5f;
double t = 0.0;
float knobs[16];
int program = 0;
float pulse_width = 0.5;
float fm = 0;

enum { SIN_WAVE, TRI_WAVE, SAW_WAVE, SQUARE_WAVE };
int wave_form = SIN_WAVE;


int AUDIO_WINDOW_SIZE = 512;

void midi_callback( double deltatime, std::vector< unsigned char > *message, void */*userData*/ )
{
	unsigned int nBytes = message->size();
	int type = (int)message->at(0) & 0xF0;
	int channel = (int)message->at(0) & 0x0F;
	switch (type)
	{
		case 0x80: std::cout << "NOTEOFF "; break;
		case 0x90: std::cout << "NOTEON  "; break;
		case 0xA0: std::cout << "POLY AFT"; break;
		case 0xB0: std::cout << "CONTROLC"; break;
		case 0xC0: std::cout << "PROGRAMC"; break;
		case 0xD0: std::cout << "CHAN AFT"; break;
		case 0xE0: std::cout << "PITCHB"; break;
		case 0xF0: std::cout << "SYSTEM"; break;
	}
	std::cout << ": ";
	
	for ( unsigned int i=0; i<nBytes; i++ )
	std::cout << "B" << i << " = " << (int)message->at(i) << ", ";
	if (nBytes > 0)
		std::cout << "stamp = " << deltatime;

	//knobs
	if ((int)message->at(0) == 0xB0) 
	{
		int num = (int)message->at(1);
		int knob_index = num % 16;
		float v = (int)message->at(2) / 127.0f;
		knobs[knob_index] = v;
		switch (knob_index)
		{
			case 1: wave_form = (v * 3); break;
			case 2: pulse_width = v; break;
			case 3: fm = v; break;
			case 4:	amplitude = v; break;
		}
	}

	
	if (type == 0x80) //keys
		pitch = 27.5 * pow(2, ((int)message->at(1) - 21) / 12.0);
	if (type == 0xC0) //program
		program = (int)message->at(1);

	std::cout << ", pitch = " << pitch << std::endl;
}


void write_audio_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max)
{
	const struct SoundIoChannelLayout *layout = &outstream->layout;
	struct SoundIoChannelArea *areas;

	double float_sample_rate = outstream->sample_rate;
	double seconds_per_frame = 1.0 / float_sample_rate;
	int err;

	int frame_count = clamp( AUDIO_WINDOW_SIZE, frame_count_min, frame_count_max);
	//std::cout << frame_count << " " << t << std::endl;


	//soundio_outstream_clear_buffer(outstream); //dont know

	//start writing audio
	if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
		fprintf(stderr, "%s\n", soundio_strerror(err));
		exit(1);
	}

	//for every sample...
	for (int frame = 0; frame < frame_count; frame += 1) {

		float f = phase - (int)phase; //0 to 1
		float sample = 0;
		double wave_length = pitch / float_sample_rate;

		if(program == 0)
		switch (wave_form)
		{
			case SAW_WAVE: sample = f*2.0-1.0; break;
			case TRI_WAVE: sample = (f < 0.5 ? f * 2.0 : 1.0 - f * 2.0); break;
			case SIN_WAVE: sample = sin(f * (2.0 * PI)); break;
			case SQUARE_WAVE: sample = f > pulse_width ? 1 : -1; break;
			default:
				sample = f;
		}

		if( fm )
			sample = lerp( sample, sample * sin(t*pitch), fm);


		sample *= amplitude;
		phase += wave_length;
		t += seconds_per_frame;

		for (int channel = 0; channel < layout->channel_count; channel += 1) {
			float *ptr = (float*)(areas[channel].ptr + areas[channel].step * frame);
			*ptr = sample;
		}
	}

	//send to device
	if ((err = soundio_outstream_end_write(outstream))) {
		fprintf(stderr, "%s\n", soundio_strerror(err));
		exit(1);
	}

	phase = phase - (int)phase;
}


void step()
{	
	frame++;
	
}


class InputParser {
public:
	InputParser(int &argc, char **argv) {
		for (int i = 1; i < argc; ++i)
			this->tokens.push_back(std::string(argv[i]));
	}
	/// @author iain
	const std::string& getCmdOption(const std::string &option) const {
		std::vector<std::string>::const_iterator itr;
		itr = std::find(this->tokens.begin(), this->tokens.end(), option);
		if (itr != this->tokens.end() && ++itr != this->tokens.end()) {
			return *itr;
		}
		static const std::string empty_string("");
		return empty_string;
	}
	/// @author iain
	bool cmdOptionExists(const std::string &option) const {
		return std::find(this->tokens.begin(), this->tokens.end(), option)
			!= this->tokens.end();
	}
private:
	std::vector <std::string> tokens;
};


int main(int argc, char **argv) {

	InputParser input(argc, argv);

	int audio_port = -1;
	int midi_port = -1;

	//audio_port = 0;
	//midi_port = 1;

	const std::string &audio_out = input.getCmdOption("-o");
	if (!audio_out.empty()) {
		audio_port = std::stoi(audio_out);
	}

	const std::string &midi_in = input.getCmdOption("-m");
	if (!midi_in.empty()) {
		midi_port = std::stoi(midi_in);
	}

	//show help
	if (argc == 1 && (audio_port == -1 || midi_port == -1) ) 
	{
		std::cout << "Synthberry by Javi Agenjo 2019 v0.1a" << std::endl;
		std::cout << "-o num   select output device" << std::endl;
		std::cout << "-m num   select midi input device" << std::endl;
		std::cout << "Audio devices found:" << std::endl;
		listAudioDevices();
		std::cout << "\nMIDI IN devices found:" << std::endl;
		listMIDIDevices();
		return 0;
	}

	//RTMIDI
	openMIDIPort(midi_port, &midi_callback );

	//AUDIO
	openAudioDevice(audio_port, write_audio_callback);

	std::cout << "Latency: " << (AUDIO_WINDOW_SIZE / 48000.0) << "ms" << std::endl;

	while(!must_exit)
	{
		step();
	}
	
	closeAudioDevice();
    return 0;
}
