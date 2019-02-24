#include "audioio.h"
#include <algorithm>
#include <string>

bool must_exit = false;
int frame = 0;
static float seconds_offset = 0.0f;
float pitch = 440.0f;
float phase = 0.0f;
float amplitude = 0.5f;
double t = 0.0;
float knobs[16];
float pulse_width = 0.5;
float fm = 0;

enum { SIN_WAVE, TRI_WAVE, SAW_WAVE, SQUARE_WAVE };
int wave_form = SIN_WAVE;


int AUDIO_WINDOW_SIZE = 256;

void midi_callback( double deltatime, std::vector< unsigned char > *message, void */*userData*/ )
{
	unsigned int nBytes = message->size();
	int type = (int)message->at(0);
	switch (type)
	{
		case 0x80: std::cout << "NOTEON "; break;
		case 0x90: std::cout << "NOTEOFF"; break;
		case 0xA0: std::cout << "AFTER  "; break;
		case 0xB0: std::cout << "CC     "; break;
		case 0xC0: std::cout << "PROGRAM CHANGE"; break;
	}
	std::cout << ":\t";

	
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
			case 1: wave_form = (v * 4); break;
			case 2: pulse_width = v; break;
			case 3: fm = v; break;
			case 4:	amplitude = v; break;
		}
	}

	//keys
	if (type == 0x80)
		pitch = 27.5 * pow(2, ((int)message->at(1) - 21) / 12.0);
    
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

	//start writing audio
	if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
		fprintf(stderr, "%s\n", soundio_strerror(err));
		exit(1);
	}

	double wave_length = pitch / float_sample_rate;

	//for every sample...
	for (int frame = 0; frame < frame_count; frame += 1) {

		//pitch = 500.0f + sin(knobs[1]*10*t*2.0*PI) * 100.0f;
		//double wave_length = pitch / float_sample_rate;
		float f = phase - (int)phase; //0 to 1
		float sample = 0;
		
		switch (wave_form)
		{
			case SAW_WAVE: sample = f*2.0-1.0; break;
			case TRI_WAVE: sample = (f < 0.5 ? f * 2.0 : 1.0 - f * 2.0); break;
			case SQUARE_WAVE: sample = f > pulse_width ? 1 : -1; break;
			case SIN_WAVE: sample = sin(f * (2.0 * PI)); break;
			default:
				sample = f;
		}

		if( fm )
			sample = lerp( sample, sample * sin(t*1000.0), fm);


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
}

void write_audio_callback2(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max)
{
    const struct SoundIoChannelLayout *layout = &outstream->layout;
    struct SoundIoChannelArea *areas;

    float float_sample_rate = outstream->sample_rate;
    float seconds_per_frame = 1.0f / float_sample_rate;
    int frames_left = frame_count_max;
    int err;
    
    while (frames_left > 0)
	{
        int frame_count = frames_left;

        if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
            fprintf(stderr, "%s\n", soundio_strerror(err));
            exit(1);
        }

        if (!frame_count)
            break;
            
        //pitch = 440 + sin(t) * 100;
        double wave_length = pitch / float_sample_rate;

        //for every sample...
        for (int frame = 0; frame < frame_count; frame += 1) {
            float sample = sin(phase * (2.0 * PI)) * amplitude; 
            phase += wave_length;
            
            for (int channel = 0; channel < layout->channel_count; channel += 1) {
                float *ptr = (float*)(areas[channel].ptr + areas[channel].step * frame);
                *ptr = sample;
            }
        }
                        
        /*

        float radians_per_second = pitch * 2.0f * PI;
        for (int frame = 0; frame < frame_count; frame += 1) {
            float sample = sin((seconds_offset + frame * seconds_per_frame) * radians_per_second);
            for (int channel = 0; channel < layout->channel_count; channel += 1) {
                float *ptr = (float*)(areas[channel].ptr + areas[channel].step * frame);
                *ptr = sample;
            }
        }
        seconds_offset = fmod(seconds_offset + seconds_per_frame * frame_count, 1.0);
        */

        if ((err = soundio_outstream_end_write(outstream))) {
            fprintf(stderr, "%s\n", soundio_strerror(err));
            exit(1);
        }

        frames_left -= frame_count;
    }
    
    t += seconds_per_frame * frame_count_max;    
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

	while(!must_exit)
	{
		step();
	}
	
	closeAudioDevice();
    return 0;
}
