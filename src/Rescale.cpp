#include "plugin.hpp"


struct Rescale : Module {
	enum ParamId {
		GAIN_PARAM,
		OFFSET_PARAM,
		MAX_PARAM,
		MIN_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		IN_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		ENUMS(MAX_LIGHT, 2),
		ENUMS(MIN_LIGHT, 2),
		LIGHTS_LEN
	};

	float multiplier = 1.f;
	bool reflectMin = false;
	bool reflectMax = false;
	dsp::ClockDivider lightDivider;

	Rescale() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		struct GainQuantity : ParamQuantity {
			float getDisplayValue() override {
				Rescale* module = reinterpret_cast<Rescale*>(this->module);
				if (module->multiplier == 1.f) {
					unit = "%";
					displayMultiplier = 100.f;
				}
				else {
					unit = "x";
					displayMultiplier = module->multiplier;
				}
				return ParamQuantity::getDisplayValue();
			}
		};
		configParam<GainQuantity>(GAIN_PARAM, -1.f, 1.f, 0.f, "Gain", "%", 0, 100);
		configParam(OFFSET_PARAM, -10.f, 10.f, 0.f, "Offset", " V");
		configParam(MAX_PARAM, -10.f, 10.f, 10.f, "Maximum", " V");
		configParam(MIN_PARAM, -10.f, 10.f, -10.f, "Minimum", " V");
		configInput(IN_INPUT, "Signal");
		configOutput(OUT_OUTPUT, "Signal");
		configBypass(IN_INPUT, OUT_OUTPUT);

		lightDivider.setDivision(16);
	}

	void onReset(const ResetEvent& e) override {
		Module::onReset(e);
		multiplier = 1.f;
		reflectMin = false;
		reflectMax = false;
	}

	void process(const ProcessArgs& args) override {
		using simd::float_4;

		int channels = std::max(1, inputs[IN_INPUT].getChannels());

		float gain = params[GAIN_PARAM].getValue() * multiplier;
		float offset = params[OFFSET_PARAM].getValue();
		float min = params[MIN_PARAM].getValue();
		float max = params[MAX_PARAM].getValue();

		bool maxLight = false;
		bool minLight = false;
		bool lightProcess = lightDivider.process();

		for (int c = 0; c < channels; c += 4) {
			float_4 x = inputs[IN_INPUT].getVoltageSimd<float_4>(c);
			x *= gain;
			x += offset;

			// Check lights
			if (lightProcess) {
				// Mask result for non factor of 4 channels.
				int mask = 0xffff >> (16 - channels + c);
				if (simd::movemask(x <= min) & mask)
					minLight = true;
				if (simd::movemask(x >= max) & mask)
					maxLight = true;
			}

			if (max <= min) {
				x = min;
			}
			else if (reflectMin && reflectMax) {
				// Cyclically reflect value between min and max
				float range = max - min;
				x = (x - min) / range;
				x = simd::fmod(x + 1.f, 2.f) - 1.f;
				x = simd::fabs(x);
				x = x * range + min;
			}
			else if (reflectMin) {
				x = simd::fabs(x - min) + min;
				x = simd::fmin(x, max);
			}
			else if (reflectMax) {
				x = max - simd::fabs(max - x);
				x = simd::fmax(x, min);
			}
			else {
				x = simd::fmin(x, max);
				x = simd::fmax(x, min);
			}

			outputs[OUT_OUTPUT].setVoltageSimd(x, c);
		}

		outputs[OUT_OUTPUT].setChannels(channels);

		// Lights
		if (lightProcess) {
			float lightTime = args.sampleTime * lightDivider.getDivision();
			lights[MAX_LIGHT + 0].setBrightnessSmooth(maxLight && (channels <= 1), lightTime);
			lights[MAX_LIGHT + 1].setBrightnessSmooth(maxLight && (channels > 1), lightTime);
			lights[MIN_LIGHT + 0].setBrightnessSmooth(minLight && (channels <= 1), lightTime);
			lights[MIN_LIGHT + 1].setBrightnessSmooth(minLight && (channels > 1), lightTime);
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "multiplier", json_real(multiplier));
		json_object_set_new(rootJ, "reflectMin", json_boolean(reflectMin));
		json_object_set_new(rootJ, "reflectMax", json_boolean(reflectMax));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* multiplierJ = json_object_get(rootJ, "multiplier");
		if (multiplierJ)
			multiplier = json_number_value(multiplierJ);

		json_t* reflectMinJ = json_object_get(rootJ, "reflectMin");
		if (reflectMinJ)
			reflectMin = json_boolean_value(reflectMinJ);

		json_t* reflectMaxJ = json_object_get(rootJ, "reflectMax");
		if (reflectMaxJ)
			reflectMax = json_boolean_value(reflectMaxJ);
	}
};


struct RescaleWidget : ModuleWidget {
	RescaleWidget(Rescale* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Rescale.svg"), asset::plugin(pluginInstance, "res/Rescale-dark.svg")));

		addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.62, 24.723)), module, Rescale::GAIN_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.617, 43.031)), module, Rescale::OFFSET_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(7.612, 64.344)), module, Rescale::MAX_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(7.612, 80.597)), module, Rescale::MIN_PARAM));

		addInput(createInputCentered<ThemedPJ301MPort>(mm2px(Vec(7.62, 96.859)), module, Rescale::IN_INPUT));

		addOutput(createOutputCentered<ThemedPJ301MPort>(mm2px(Vec(7.62, 113.115)), module, Rescale::OUT_OUTPUT));

		addChild(createLightCentered<TinyLight<YellowBlueLight<>>>(mm2px(Vec(12.327, 57.3)), module, Rescale::MAX_LIGHT));
		addChild(createLightCentered<TinyLight<YellowBlueLight<>>>(mm2px(Vec(12.327, 73.559)), module, Rescale::MIN_LIGHT));
	}

	void appendContextMenu(Menu* menu) override {
		Rescale* module = getModule<Rescale>();

		menu->addChild(new MenuSeparator);

		menu->addChild(createIndexSubmenuItem("Gain multiplier", {"1x", "10x", "100x", "1000x"},
			[=]() {
				return (int) std::log10(module->multiplier);
			},
			[=](int mode) {
				module->multiplier = std::pow(10.f, (float) mode);
			}
		));

		menu->addChild(createBoolPtrMenuItem("Reflect at maximum", "", &module->reflectMax));
		menu->addChild(createBoolPtrMenuItem("Reflect at minimum", "", &module->reflectMin));
	}
};


Model* modelRescale = createModel<Rescale, RescaleWidget>("Rescale");