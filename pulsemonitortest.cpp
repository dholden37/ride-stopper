/***
This file is part of PulseAudio.
PulseAudio is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation; either version 2.1 of the License,
or (at your option) any later version.
PulseAudio is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.
You should have received a copy of the GNU Lesser General Public License
along with PulseAudio; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
USA.
***/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <alsa/asoundlib.h>
#include "FingerprintExtractor.h" 
#define BUFSIZE 2097152
#define STEP 100000


// Our muting program (taken from http://stackoverflow.com/questions/3985014/linux-alsa-sound-api-questions-how-do-you-mute)

void SetAlsaMasterMute()
{
    long min, max;
    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    const char *card = "default";
    const char *selem_name = "Master";

    snd_mixer_open(&handle, 0);
    snd_mixer_attach(handle, card);
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);
    snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

    if (snd_mixer_selem_has_playback_switch(elem)) {
        snd_mixer_selem_set_playback_switch_all(elem, 0);
    }

    snd_mixer_close(handle);
}

int main(int argc, char*argv[]) {
/* The sample type to use */
static pa_sample_spec ss;
ss.format = PA_SAMPLE_S16LE,
ss.rate = 44100,
ss.channels = 2;

pa_simple *s = NULL;
fingerprint::FingerprintExtractor fextr;
fextr.initForQuery(ss.rate,ss.channels, 22);
int ret = 1;
int error;
/* Create the recording stream */
if (!(s = pa_simple_new(NULL, argv[0], PA_STREAM_RECORD, "alsa_output.pci-0000_00_14.2.analog-stereo.monitor", "record", &ss, NULL, NULL, &error))) {
fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
return ret;
}
short buf[BUFSIZE];
short* newbuf = buf;

for (;;) {

/* Record some data ... */
int result = pa_simple_read(s, newbuf, STEP, &error);
if (result < 0) {
fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(error));
goto finish;
}
// here we go!
newbuf += STEP / 2;
if( fextr.process( buf, newbuf - buf, false) ) {
	// We have liftoff!
	std::pair< const char*, size_t> data = fextr.getFingerprint();
	int i;
	for( i = 0; i < data.second; i++ ) {
	   std::cout << data.first[i];
	}
	std::cout << std::endl;
	break;
}
// TODO: make thing do stuff

// Does the mutey thing
// SetAlsaMasterMute();
}
ret = 0;
finish:
if (s)
pa_simple_free(s);
return ret;
}

