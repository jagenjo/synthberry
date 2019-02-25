# synthberry
A commandline audio synthetizer with MIDI support for the Raspberry Pi.

The synth patches are loaded from a text file that specifies how the audio must be created:

```
patch1:
voices=4
portamento=0.1
//audio setup
ADSR( OSC("SQUARE", pitch,0.4) , 0.2, CC[3], 0.5, 0.3 );
```


