#include "audioio.h"


bool must_exit = false;
int frame = 0;
static float seconds_offset = 0.0f;
float pitch = 440.0f;
float phase = 0.0f;
float amplitude = 1.0;
float t = 0.0f;


void midi_callback( double deltatime, std::vector< unsigned char > *message, void */*userData*/ )
{
  unsigned int nBytes = message->size();
  for ( unsigned int i=0; i<nBytes; i++ )
    std::cout << "Byte " << i << " = " << (int)message->at(i) << ", ";
  if ( nBytes > 0 )
    std::cout << "stamp = " << deltatime << std::endl;
    
    if( (int)message->at(1) == 3 )
	    amplitude = (int)message->at(2) / 127.0f;
    else
	    pitch = 440 + (int)message->at(2);
}


void write_audio_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max)
{
    const struct SoundIoChannelLayout *layout = &outstream->layout;
    struct SoundIoChannelArea *areas;

    float float_sample_rate = outstream->sample_rate;
    float seconds_per_frame = 1.0f / float_sample_rate;
    int frames_left = frame_count_max;
    int err;
    
    while (frames_left > 0) {
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


int main(int argc, char **argv) {

	//RTMIDI
	openMIDIPort(1, &midi_callback );

	//AUDIO
	openAudioDevice(1, write_audio_callback);

	while(!must_exit)
	{
		step();
	}
	
	closeAudioDevice();
	
    return 0;
}