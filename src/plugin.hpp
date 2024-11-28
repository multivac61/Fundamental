#include <rack.hpp>


using namespace rack;


extern Plugin* pluginInstance;

extern Model* modelVCO;
extern Model* modelVCO2;
extern Model* modelVCF;
extern Model* modelVCA_1;
extern Model* modelVCA;
extern Model* modelLFO;
extern Model* modelLFO2;
extern Model* modelDelay;
extern Model* modelADSR;
extern Model* modelMixer;
extern Model* modelVCMixer;
extern Model* model_8vert;
extern Model* modelUnity;
extern Model* modelMutes;
extern Model* modelPulses;
extern Model* modelScope;
extern Model* modelSEQ3;
extern Model* modelSequentialSwitch1;
extern Model* modelSequentialSwitch2;
extern Model* modelOctave;
extern Model* modelQuantizer;
extern Model* modelSplit;
extern Model* modelMerge;
extern Model* modelSum;
extern Model* modelViz;
extern Model* modelMidSide;
extern Model* modelNoise;
extern Model* modelRandom;
extern Model* modelCVMix;

extern Model* modelCompare;
extern Model* modelFade;
extern Model* modelGates;
extern Model* modelLogic;
extern Model* modelMult;
extern Model* modelProcess;
extern Model* modelPush;
extern Model* modelRandomValues;
extern Model* modelRescale;
extern Model* modelSHASR;


struct DigitalDisplay : Widget {
	std::string fontPath;
	std::string bgText;
	std::string text;
	float fontSize;
	NVGcolor bgColor = nvgRGB(0x46,0x46, 0x46);
	NVGcolor fgColor = SCHEME_YELLOW;
	Vec textPos;

	void prepareFont(const DrawArgs& args) {
		// Get font
		std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
		if (!font)
			return;
		nvgFontFaceId(args.vg, font->handle);
		nvgFontSize(args.vg, fontSize);
		nvgTextLetterSpacing(args.vg, 0.0);
		nvgTextAlign(args.vg, NVG_ALIGN_RIGHT);
	}

	void draw(const DrawArgs& args) override {
		// Background
		nvgBeginPath(args.vg);
		nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 2);
		nvgFillColor(args.vg, nvgRGB(0x19, 0x19, 0x19));
		nvgFill(args.vg);

		prepareFont(args);

		// Background text
		nvgFillColor(args.vg, bgColor);
		nvgText(args.vg, textPos.x, textPos.y, bgText.c_str(), NULL);
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer == 1) {
			prepareFont(args);

			// Foreground text
			nvgFillColor(args.vg, fgColor);
			nvgText(args.vg, textPos.x, textPos.y, text.c_str(), NULL);
		}
		Widget::drawLayer(args, layer);
	}
};


struct ChannelDisplay : DigitalDisplay {
	ChannelDisplay() {
		fontPath = asset::system("res/fonts/DSEG7ClassicMini-BoldItalic.ttf");
		textPos = Vec(22, 20);
		bgText = "18";
		fontSize = 16;
	}
};


template <typename TBase = GrayModuleLightWidget>
struct YellowRedLight : TBase {
	YellowRedLight() {
		this->addBaseColor(SCHEME_YELLOW);
		this->addBaseColor(SCHEME_RED);
	}
};

template <typename TBase = GrayModuleLightWidget>
struct YellowBlueLight : TBase {
	YellowBlueLight() {
		this->addBaseColor(SCHEME_YELLOW);
		this->addBaseColor(SCHEME_BLUE);
	}
};


struct VCVBezelBig : app::SvgSwitch {
	VCVBezelBig() {
		momentary = true;
		addFrame(Svg::load(asset::plugin(pluginInstance, "res/VCVBezelBig.svg")));
	}
};


template <typename TBase>
struct VCVBezelLightBig : TBase {
	VCVBezelLightBig() {
		this->borderColor = color::BLACK_TRANSPARENT;
		this->bgColor = color::BLACK_TRANSPARENT;
		this->box.size = mm2px(math::Vec(11.1936, 11.1936));
	}
};


/*MenuItem* createRangeItem(std::string label, float* gain, float* offset);*/

static inline MenuItem* createRangeItem(std::string label, float* gain, float* offset) {
	struct Range {
		float gain;
		float offset;

		bool operator==(const Range& other) const {
			return gain == other.gain && offset == other.offset;
		}
	};

	static const std::vector<Range> ranges = {
		{10.f, 0.f},
		{5.f, 0.f},
		{1.f, 0.f},
		{20.f, -10.f},
		{10.f, -5.f},
		{2.f, -1.f},
	};
	static std::vector<std::string> labels;
	if (labels.empty()) {
		for (const Range& range : ranges) {
			labels.push_back(string::f("%gV to %gV", range.offset, range.offset + range.gain));
		}
	}

	return createIndexSubmenuItem(label, labels,
		[=]() {
			auto it = std::find(ranges.begin(), ranges.end(), Range{*gain, *offset});
			return std::distance(ranges.begin(), it);
		},
		[=](int i) {
			*gain = ranges[i].gain;
			*offset = ranges[i].offset;
		}
	);
}

