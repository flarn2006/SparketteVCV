#include "plugin.hpp"
#include "Lights.hpp"
#include <cmath>

using namespace sparkette;

struct Busybox : Module {
	enum ParamId {
		LFREQ1_PARAM,
		LFREQ2_PARAM,
		LFREQ3_PARAM,
		LFREQ4_PARAM,
		LFOPW_PARAM,
		LFORESET_PARAM,
		ENV1A_PARAM,
		ENV1D_PARAM,
		ENV1S_PARAM,
		ENV1R_PARAM,
		ENV2A_PARAM,
		ENV2D_PARAM,
		ENV2S_PARAM,
		ENV2R_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		LFORESET_INPUT,
		ENV1GATE_INPUT,
		ENV1VCA_INPUT,
		ENV2GATE_INPUT,
		ENV2VCA_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		LFO1_OUTPUT,
		LFO1INV_OUTPUT,
		LFO2_OUTPUT,
		LFO2INV_OUTPUT,
		LFO3_OUTPUT,
		LFO3INV_OUTPUT,
		LFO4_OUTPUT,
		LFO4INV_OUTPUT,
		NOISE_OUTPUT,
		ENV1_OUTPUT,
		ENV1VCA_OUTPUT,
		ENV2_OUTPUT,
		ENV2VCA_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LFO1_LIGHT,
		LFO2_LIGHT,
		LFO3_LIGHT,
		LFO4_LIGHT,
		ENV1_LIGHT,
		ENV2_LIGHT,
		LIGHTS_LEN
	};

	struct LFO {
		Quantity* freq;
		Light* light;
		Output *out, *out2;
		float phase = 0.f;

		static constexpr float FREQ_DISPLAY_BASE = 2.f;

		// Assume 0<=t<=1. Return value must be in same range.
		virtual float wave(float t) = 0;

		void process(const ProcessArgs& args) {
			phase = std::fmod(phase + dsp::approxExp2_taylor5(freq->getValue()) * args.sampleTime, 1.f);
			float y = wave(phase);
			light->setBrightness(y);
			out->setVoltage(y * 10);
			out2->setVoltage(10.f - y * 10);
		}

		void reset() { phase = 0.f; }
	};

	struct SawLFO : LFO {
		float wave(float t) override { return t; }
	};
	struct TriangleLFO : LFO {
		float wave(float t) override { return 1.f - 2.f*std::abs(0.5f - t); }
	};
	struct SquareLFO : LFO {
		Quantity* pulseWidth;
		float wave(float t) override { return t < pulseWidth->getValue(); }
	};
	struct SineLFO : LFO {
		float wave(float t) override { return (std::sin(t * 2.f*M_PI) + 1) / 2; }
	};

	static constexpr std::size_t LFO_COUNT = 4;

	SawLFO sawLfo;
	TriangleLFO triangleLfo;
	SquareLFO squareLfo;
	SineLFO sineLfo;
	LFO* lfos[LFO_COUNT];
	
	dsp::SchmittTrigger lfo_reset_trig;

	struct ADSR_VCA {
		Quantity *attack, *decay, *sustain, *release;
		Input *gate, *vca_in;
		Output *env_out, *vca_out;
		Light *light;
		int segment[PORT_MAX_CHANNELS];
		float time[PORT_MAX_CHANNELS];
		float envelope[PORT_MAX_CHANNELS];
		float setpoint[PORT_MAX_CHANNELS];

		ADSR_VCA() {
			for (int i=0; i<PORT_MAX_CHANNELS; ++i) {
				segment[i] = 0;
				time[i] = 0.f;
				envelope[i] = 0.f;
				setpoint[i] = 0.f;
			}
		}

		void process(const ProcessArgs& args) {
			float gates[PORT_MAX_CHANNELS];
			float vca[PORT_MAX_CHANNELS];
			int nchan = gate->getChannels();
			gate->readVoltages(gates);
			vca_in->readVoltages(vca);

			for (int i=0; i<PORT_MAX_CHANNELS; ++i) {
				float a = attack->getValue();
				float d = decay->getValue();
				float s = sustain->getValue();
				float r = release->getValue();

				if (i < nchan && gates[i] > 0.5f) {
					switch (segment[i]) {
						case 0: //Inactive
							segment[i] = 1;
							time[i] = args.sampleTime;
							break;
						case 1: //Attack
							time[i] += args.sampleTime;
							if (time[i] >= a) {
								segment[i] = 2;
								time[i] -= a;
							}
							break;
						case 2: //Decay
							time[i] += args.sampleTime;
							if (time[i] >= d)
								segment[i] = 3;
							break;
						case 4: //Release
							segment[i] = 1;
							time[i] = args.sampleTime;
							setpoint[i] = envelope[i];
							break;
					}
				} else {
					switch (segment[i]) {
						case 0: //Inactive
							break;
						case 4: //Release
							time[i] += args.sampleTime;
							if (time[i] >= r) {
								segment[i] = 0;
								setpoint[i] = 0.f;
							}
							break;
						default:
							segment[i] = 4;
							time[i] = args.sampleTime;
							setpoint[i] = envelope[i];
							break;
					}
				}

				switch (segment[i]) {
					case 0: //Inactive
						envelope[i] = 0.f;
						break;
					case 1: //Attack
						envelope[i] = setpoint[i] + time[i] / a * (10.f - setpoint[i]);
						break;
					case 2: //Decay
						envelope[i] = s + (1.f - time[i] / d) * (10.f - s);
						break;
					case 3: //Sustain
						envelope[i] = s;
						break;
					case 4: //Release
						envelope[i] = setpoint[i] * (1.f - time[i] / r);
						break;
				}

				vca[i] *= envelope[i] / 10;
			}

			light->setBrightness(envelope[0] / 10);
			env_out->setChannels(nchan);
			env_out->writeVoltages(envelope);
			vca_out->setChannels(nchan);
			vca_out->writeVoltages(vca);
		}
	};

	static constexpr std::size_t ADSR_VCA_COUNT = 2;
	ADSR_VCA adsr[ADSR_VCA_COUNT];

	Busybox() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(LFREQ1_PARAM, -3.f, 8.f, 2.f, "LFO 1 Frequency", " Hz", LFO::FREQ_DISPLAY_BASE);
		configParam(LFREQ2_PARAM, -3.f, 8.f, 2.f, "LFO 2 Frequency", " Hz", LFO::FREQ_DISPLAY_BASE);
		configParam(LFREQ3_PARAM, -3.f, 8.f, 2.f, "LFO 3 Frequency", " Hz", LFO::FREQ_DISPLAY_BASE);
		configParam(LFREQ4_PARAM, -3.f, 8.f, 2.f, "LFO 4 Frequency", " Hz", LFO::FREQ_DISPLAY_BASE);
		configParam(LFOPW_PARAM, 0.f, 1.f, 0.5f, "LFO 3 Pulse Width", "%", 0.f, 100.f);
		configParam(LFORESET_PARAM, 0.f, 1.f, 0.f, "LFO Reset");
		configParam(ENV1A_PARAM, 0.f, 4.f, 0.f, "Envelope 1 Attack");
		configParam(ENV1D_PARAM, 0.f, 4.f, 0.5f, "Envelope 1 Decay");
		configParam(ENV1S_PARAM, 0.f, 10.f, 7.f, "Envelope 1 Sustain");
		configParam(ENV1R_PARAM, 0.f, 4.f, 0.5f, "Envelope 1 Release");
		configParam(ENV2A_PARAM, 0.f, 4.f, 0.f, "Envelope 2 Attack");
		configParam(ENV2D_PARAM, 0.f, 4.f, 0.5f, "Envelope 2 Decay");
		configParam(ENV2S_PARAM, 0.f, 10.f, 7.f, "Envelope 2 Sustain");
		configParam(ENV2R_PARAM, 0.f, 4.f, 0.5f, "Envelope 2 Release");
		configInput(LFORESET_INPUT, "LFO Reset Trigger");
		configInput(ENV1GATE_INPUT, "Envelope 1 Gate");
		configInput(ENV1VCA_INPUT, "Envelope 1 VCA");
		configInput(ENV2GATE_INPUT, "Envelope 2 Gate");
		configInput(ENV2VCA_INPUT, "Envelope 2 VCA");
		configOutput(LFO1_OUTPUT, "LFO 1");
		configOutput(LFO1INV_OUTPUT, "LFO 1 Inverted");
		configOutput(LFO2_OUTPUT, "LFO 2");
		configOutput(LFO2INV_OUTPUT, "LFO 2 Inverted");
		configOutput(LFO3_OUTPUT, "LFO 3");
		configOutput(LFO3INV_OUTPUT, "LFO 3 Inverted");
		configOutput(LFO4_OUTPUT, "LFO 4");
		configOutput(LFO4INV_OUTPUT, "LFO 4 Inverted");
		configOutput(NOISE_OUTPUT, "Noise");
		configOutput(ENV1_OUTPUT, "Envelope 1");
		configOutput(ENV1VCA_OUTPUT, "Envelope 1 VCA");
		configOutput(ENV2_OUTPUT, "Envelope 2");
		configOutput(ENV2VCA_OUTPUT, "Envelope 2 VCA");

		sawLfo.freq = paramQuantities[LFREQ1_PARAM];
		sawLfo.light = &lights[LFO1_LIGHT];
		sawLfo.out = &outputs[LFO1_OUTPUT];
		sawLfo.out2 = &outputs[LFO1INV_OUTPUT];
		lfos[0] = &sawLfo;

		triangleLfo.freq = paramQuantities[LFREQ2_PARAM];
		triangleLfo.light = &lights[LFO2_LIGHT];
		triangleLfo.out = &outputs[LFO2_OUTPUT];
		triangleLfo.out2 = &outputs[LFO2INV_OUTPUT];
		lfos[1] = &triangleLfo;

		squareLfo.freq = paramQuantities[LFREQ3_PARAM];
		squareLfo.light = &lights[LFO3_LIGHT];
		squareLfo.out = &outputs[LFO3_OUTPUT];
		squareLfo.out2 = &outputs[LFO3INV_OUTPUT];
		squareLfo.pulseWidth = paramQuantities[LFOPW_PARAM];
		lfos[2] = &squareLfo;

		sineLfo.freq = paramQuantities[LFREQ4_PARAM];
		sineLfo.light = &lights[LFO4_LIGHT];
		sineLfo.out = &outputs[LFO4_OUTPUT];
		sineLfo.out2 = &outputs[LFO4INV_OUTPUT];
		lfos[3] = &sineLfo;

		adsr[0].attack = paramQuantities[ENV1A_PARAM];
		adsr[0].decay = paramQuantities[ENV1D_PARAM];
		adsr[0].sustain = paramQuantities[ENV1S_PARAM];
		adsr[0].release = paramQuantities[ENV1R_PARAM];
		adsr[0].gate = &inputs[ENV1GATE_INPUT];
		adsr[0].vca_in = &inputs[ENV1VCA_INPUT];
		adsr[0].env_out = &outputs[ENV1_OUTPUT];
		adsr[0].vca_out = &outputs[ENV1VCA_OUTPUT];
		adsr[0].light = &lights[ENV1_LIGHT];

		adsr[1].attack = paramQuantities[ENV2A_PARAM];
		adsr[1].decay = paramQuantities[ENV2D_PARAM];
		adsr[1].sustain = paramQuantities[ENV2S_PARAM];
		adsr[1].release = paramQuantities[ENV2R_PARAM];
		adsr[1].gate = &inputs[ENV2GATE_INPUT];
		adsr[1].vca_in = &inputs[ENV2VCA_INPUT];
		adsr[1].env_out = &outputs[ENV2_OUTPUT];
		adsr[1].vca_out = &outputs[ENV2VCA_OUTPUT];
		adsr[1].light = &lights[ENV2_LIGHT];
	}

	void process(const ProcessArgs& args) override {
		bool reset = lfo_reset_trig.process(inputs[LFORESET_INPUT].getVoltage()) || params[LFORESET_PARAM].getValue();
		for (std::size_t i=0; i<LFO_COUNT; ++i) {
			if (reset)
				lfos[i]->reset();
			lfos[i]->process(args);
		}

		float r = (float)std::rand() / RAND_MAX;
		outputs[NOISE_OUTPUT].setVoltage(10.f * r - 5.f);

		for (std::size_t i=0; i<ADSR_VCA_COUNT; ++i)
			adsr[i].process(args);
	}
};


struct BusyboxWidget : ModuleWidget {
	BusyboxWidget(Busybox* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Busybox.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(5.08, 13.45)), module, Busybox::LFREQ1_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(5.08, 23.61)), module, Busybox::LFREQ2_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(5.08, 33.77)), module, Busybox::LFREQ3_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(5.08, 43.93)), module, Busybox::LFREQ4_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(19.0, 31.0)), module, Busybox::LFOPW_PARAM));
		addParam(createParamCentered<BefacoPush>(mm2px(Vec(15.24, 54.09)), module, Busybox::LFORESET_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(15.24, 74.41)), module, Busybox::ENV1A_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(25.4, 74.41)), module, Busybox::ENV1D_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(15.24, 84.57)), module, Busybox::ENV1S_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(25.4, 84.57)), module, Busybox::ENV1R_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(15.24, 97.27)), module, Busybox::ENV2A_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(25.4, 97.27)), module, Busybox::ENV2D_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(15.24, 107.43)), module, Busybox::ENV2S_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(25.4, 107.43)), module, Busybox::ENV2R_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.08, 54.09)), module, Busybox::LFORESET_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.08, 74.41)), module, Busybox::ENV1GATE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.08, 84.57)), module, Busybox::ENV1VCA_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.08, 97.27)), module, Busybox::ENV2GATE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.08, 107.43)), module, Busybox::ENV2VCA_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(25.4, 13.45)), module, Busybox::LFO1_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35.56, 13.45)), module, Busybox::LFO1INV_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(25.4, 23.61)), module, Busybox::LFO2_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35.56, 23.61)), module, Busybox::LFO2INV_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(25.4, 33.77)), module, Busybox::LFO3_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35.56, 33.77)), module, Busybox::LFO3INV_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(25.4, 43.93)), module, Busybox::LFO4_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35.56, 43.93)), module, Busybox::LFO4INV_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35.56, 56.63)), module, Busybox::NOISE_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35.56, 74.41)), module, Busybox::ENV1_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35.56, 84.57)), module, Busybox::ENV1VCA_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35.56, 97.27)), module, Busybox::ENV2_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35.56, 107.43)), module, Busybox::ENV2VCA_OUTPUT));

		addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(15.24, 13.45)), module, Busybox::LFO1_LIGHT));
		addChild(createLightCentered<SmallLight<YellowLight>>(mm2px(Vec(15.24, 23.61)), module, Busybox::LFO2_LIGHT));
		addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(15.24, 33.77)), module, Busybox::LFO3_LIGHT));
		addChild(createLightCentered<SmallLight<BlueLight>>(mm2px(Vec(15.24, 43.93)), module, Busybox::LFO4_LIGHT));
		addChild(createLightCentered<MediumLight<PurpleLight>>(mm2px(Vec(20.32, 79.49)), module, Busybox::ENV1_LIGHT));
		addChild(createLightCentered<MediumLight<PurpleLight>>(mm2px(Vec(20.32, 102.35)), module, Busybox::ENV2_LIGHT));
	}
};


Model* modelBusybox = createModel<Busybox, BusyboxWidget>("Busybox");
