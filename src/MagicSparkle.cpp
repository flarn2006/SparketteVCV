#include "plugin.hpp"
#include "Utility.hpp"
#include <cstring>

using namespace sparkette;

struct MagicSparkle : Module {
	enum ParamId {
		DECAY_PARAM,
		DENSITY_PARAM,
		DECAY_CV_PARAM,
		DENSITY_CV_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		DECAY_INPUT,
		DENSITY_INPUT,
		AUDIO_L_INPUT,
		AUDIO_R_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		POLYFREQ_OUTPUT,
		POLYENV_OUTPUT,
		AUDIO_L_OUTPUT,
		AUDIO_R_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		STAR1_LIGHT,
		STAR2_LIGHT,
		STAR3_LIGHT,
		STAR4_LIGHT,
		STAR5_LIGHT,
		LIGHTS_LEN
	};

	struct Tone {
		float freq;
		float amp = 0.f;
		float phase = 0.f;

		void start(float freq) {
			this->freq = freq;
			amp = 1.f;
		}

		float wave(float t) const { return std::sin(t * 2.f*M_PI); }
		
		float process(const ProcessArgs& args) {
			float y = amp * wave(phase);
			phase = std::fmod(phase + freq * args.sampleTime, 1.f);
			return y;
		}
	};

	template <std::size_t S, typename T = Tone>
	struct ToneArray {
		static constexpr std::size_t size = S;
		T tones[S];
		std::size_t next = 0;
		float decay = 0.5f;

		Tone& start(float freq) {
			Tone& t = tones[next];
			t.start(freq);
			next = (next + 1) % S;
			return t;
		}

		float process(const ProcessArgs& args) {
			float sum = 0.f;
			float d = std::pow(decay, args.sampleTime);
			for (std::size_t i=0; i<S; ++i) {
				sum += tones[i].process(args);
				tones[i].amp *= d;
			}
			return sum;
		}
	};

	static constexpr std::size_t BUFFER_SIZE = 1024;

	dsp::DoubleRingBuffer<float, BUFFER_SIZE> samples;
	float* fftbuf;
	dsp::RealFFT fft;
	ToneArray<PORT_MAX_CHANNELS> tones;

	MagicSparkle() : fft(BUFFER_SIZE) {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(DECAY_PARAM, 0.f, 1.f, 0.f, "Decay");
		configParam(DENSITY_PARAM, 0.f, 1.f, 0.f, "Density");
		configParam(DECAY_CV_PARAM, 0.f, 1.f, 0.f, "Decay CV");
		configParam(DENSITY_CV_PARAM, 0.f, 1.f, 0.f, "Density CV");
		configInput(DECAY_INPUT, "Decay");
		configInput(DENSITY_INPUT, "Density");
		configInput(AUDIO_L_INPUT, "Left/mono audio");
		configInput(AUDIO_R_INPUT, "Right audio");
		configOutput(POLYFREQ_OUTPUT, "Top frequencies");
		configOutput(POLYENV_OUTPUT, "Envelopes");
		configOutput(AUDIO_L_OUTPUT, "Left/mono audio");
		configOutput(AUDIO_R_OUTPUT, "Right audio");

		fftbuf = new float[BUFFER_SIZE*2];
	}

	~MagicSparkle() {
		delete[] fftbuf;
	}

	void process(const ProcessArgs& args) override {
		samples.push(inputs[AUDIO_L_INPUT].getVoltage());
		std::memcpy(fftbuf, samples.startData(), sizeof(float)*BUFFER_SIZE);
		std::memcpy(fftbuf+BUFFER_SIZE, fftbuf, sizeof(float)*BUFFER_SIZE);
		fft.rfft(fftbuf, fftbuf+BUFFER_SIZE);
		
		float r = static_cast<float>(rand() / RAND_MAX);
		if (r < applyScaleOffset(inputs[DENSITY_INPUT].getVoltage(), params[DENSITY_CV_PARAM], params[DENSITY_PARAM])) {
			std::size_t highestIndex = 0;
			float highest = 0.f;
			for (std::size_t i=0; i<BUFFER_SIZE; ++i) {
				float y = std::abs(fftbuf[BUFFER_SIZE+i]);
				if (y > highest) {
					highestIndex = BUFFER_SIZE+i;
					highest = y;
				}
			}
			tones.start(args.sampleRate / (highestIndex + 1));
		}

		tones.decay = applyScaleOffset(inputs[DECAY_INPUT].getVoltage(), params[DECAY_CV_PARAM], params[DECAY_PARAM]);
		outputs[AUDIO_L_OUTPUT].setVoltage(tones.process(args));

		for (std::size_t i=0; i<PORT_MAX_CHANNELS; ++i) {
			fftbuf[i] = tones.tones[i].freq;
			fftbuf[BUFFER_SIZE+i] = tones.tones[i].amp;
		}
		outputs[POLYFREQ_OUTPUT].setChannels(PORT_MAX_CHANNELS);
		outputs[POLYFREQ_OUTPUT].writeVoltages(fftbuf);
		outputs[POLYENV_OUTPUT].setChannels(PORT_MAX_CHANNELS);
		outputs[POLYENV_OUTPUT].writeVoltages(fftbuf + BUFFER_SIZE);
	}
};


struct MagicSparkleWidget : ModuleWidget {
	MagicSparkleWidget(MagicSparkle* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/MagicSparkle.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Davies1900hWhiteKnob>(mm2px(Vec(21.908, 46.47)), module, MagicSparkle::DECAY_PARAM));
		addParam(createParamCentered<Davies1900hBlackKnob>(mm2px(Vec(9.102, 56.63)), module, MagicSparkle::DENSITY_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(21.908, 60.122)), module, MagicSparkle::DECAY_CV_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(9.102, 70.282)), module, MagicSparkle::DENSITY_CV_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.908, 70.283)), module, MagicSparkle::DECAY_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.102, 80.442)), module, MagicSparkle::DENSITY_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.562, 94.73)), module, MagicSparkle::AUDIO_L_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.562, 106.478)), module, MagicSparkle::AUDIO_R_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.32, 8.37)), module, MagicSparkle::POLYFREQ_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(25.929, 18.53)), module, MagicSparkle::POLYENV_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(23.918, 94.73)), module, MagicSparkle::AUDIO_L_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(23.918, 106.478)), module, MagicSparkle::AUDIO_R_OUTPUT));

		addChild(createLightCentered<MediumLight<WhiteLight>>(mm2px(Vec(6.138, 8.264)), module, MagicSparkle::STAR5_LIGHT));
		addChild(createLightCentered<MediumLight<WhiteLight>>(mm2px(Vec(15.24, 21.07)), module, MagicSparkle::STAR3_LIGHT));
		addChild(createLightCentered<MediumLight<WhiteLight>>(mm2px(Vec(5.08, 23.61)), module, MagicSparkle::STAR4_LIGHT));
		addChild(createLightCentered<SmallLight<WhiteLight>>(mm2px(Vec(22.86, 31.124)), module, MagicSparkle::STAR1_LIGHT));
		addChild(createLightCentered<SmallLight<WhiteLight>>(mm2px(Vec(11.113, 33.558)), module, MagicSparkle::STAR2_LIGHT));
	}
};


Model* modelMagicSparkle = createModel<MagicSparkle, MagicSparkleWidget>("MagicSparkle");
