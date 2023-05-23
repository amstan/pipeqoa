#include <assert.h>
#include <jack/jack.h>
#include <stdio.h>
#include <unistd.h>

#define QOA_IMPLEMENTATION
#include <qoa.h>

jack_client_t *jack_client;
#define MAX_CHANNELS 8
jack_port_t *input_ports[MAX_CHANNELS];
jack_port_t *output_ports[MAX_CHANNELS];
qoa_desc qoa[MAX_CHANNELS];
const int CHANNELS = 2;

#define TRYA(x) do {\
	if (!(x)) { \
		fprintf(stderr, "Error assigning %s\n", #x); \
		return 1; \
	} \
} while (0);
#define TRYR(x) do {\
	int ret = (x); \
	if (ret) { \
		fprintf(stderr, "Error %d doing %s\n", ret, #x); \
		return ret; \
	} \
} while (0);

int jack_process(jack_nframes_t nframes, void *arg) {
	for (int ch = 0; ch < CHANNELS; ch++) {
		float* in_buf_f  = jack_port_get_buffer(input_ports[ch], nframes);

		short int in_buf[nframes];
		for (int i = 0; i < nframes; i++)
			in_buf[i] = in_buf_f[i] * 32768;

		unsigned char qoa_buf[nframes];
		unsigned int frame_size = qoa_encode_frame(in_buf, &qoa[ch], nframes, qoa_buf);

		unsigned int decoded_samples;
		short int out_buf[nframes];
		unsigned int decoded_bytes = qoa_decode_frame(qoa_buf, frame_size, &qoa[ch], out_buf, &decoded_samples);

		assert(frame_size == decoded_bytes);
		assert(nframes == decoded_samples);

		float* out_buf_f = jack_port_get_buffer(output_ports[ch], nframes);
		for (int i = 0; i < nframes; i++)
			out_buf_f[i] = out_buf[i] / 32768.;
	}
	return 0;
}

int jack_samplerate_changed(jack_nframes_t samplerate, void *arg) {
	printf("samplerate = %d\n", samplerate);
	for (int ch = 0; ch < CHANNELS; ch++)
		qoa[ch].samplerate = samplerate;
	return 0;
}

int main(int narg, char **args) {
	printf("QOA quality test running...\n", args[0]);

	TRYA(jack_client = jack_client_open("qoa_quality_test", JackNullOption, 0));

	TRYR(jack_set_process_callback(jack_client, jack_process, 0));

	for (int ch = 0; ch < CHANNELS; ch++) {
		char in_name[8], out_name[8];

		sprintf(in_name,  "in_%d",  ch);
		TRYA(input_ports[ch]  = jack_port_register(jack_client, in_name,  JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput,  0));
		sprintf(out_name, "out_%d", ch);
		TRYA(output_ports[ch] = jack_port_register(jack_client, out_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0));

		qoa[ch].channels = 1; // we do an independent encoder/decoder per channel to avoid interlacing frames
		// qoa[ch].samplerate = // set in jack_samplerate_changed()
		/* Set the initial LMS weights to {0, 0, -1, 2}. This helps with the
		prediction of the first few ms of a file. */
		qoa[ch].lms[0].weights[0] = 0;
		qoa[ch].lms[0].weights[1] = 0;
		qoa[ch].lms[0].weights[2] = -(1<<13);
		qoa[ch].lms[0].weights[3] =  (1<<14);

		/* Explicitly set the history samples to 0, as we might have some
		garbage in there. */
		for (int i = 0; i < QOA_LMS_LEN; i++) {
			qoa[ch].lms[0].history[i] = 0;
		}
	}

	TRYR(jack_set_sample_rate_callback(jack_client, jack_samplerate_changed, 0));
	TRYR(jack_activate(jack_client));
	printf("Jack connection started!\n");

	while(1) {
		sleep(1);
	}
	return 0;
}
