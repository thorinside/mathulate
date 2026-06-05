/*
 * Mathulate - disting NT CV math processor
 *
 * Generated from Substrate artifact "Disting NT Math Function Plugin Spec".
 * Takes two CV inputs, normalizes them to -1..+1, applies one of twenty
 * safe mathematical functions, then emits bounded -10V..+10V CV.
 */

#include <distingnt/api.h>
#include <cstddef>
#include <stdint.h>
#include <new>

#include "mathulate_core.h"

namespace {

static const int kDefaultSpeed = 4;
static const float kClockThreshold = 1.0f;
static const float kFallbackSampleRate = 48000.0f;
#ifndef MATHULATE_VERSION
#define MATHULATE_VERSION "v0.1.0-dev"
#endif
static const char* const kVersion = MATHULATE_VERSION;

struct MathulateDTC {
    mathulate::State core;
    float outputScale;
    float outputOffset;
    float phaseOffset;
    float speedMultiplier;
    float phase;
    float displayPhase;
    float displayOutput;
    float displayB;
    float clockPeriodSamples;
    uint32_t samplesSinceClock;
    bool previousClockHigh;
    bool clockSeen;
};

struct MathulateAlgorithm : public _NT_algorithm {
    explicit MathulateAlgorithm(MathulateDTC* d) : dtc(d) {}
    MathulateDTC* dtc;
};

enum {
    kParamInputA,
    kParamInputB,
    kParamOutput,
    kParamOutputMode,
    kParamEquation,
    kParamScale,
    kParamOffset,
    kParamClockIn,
    kParamSpeed,
    kParamPhaseOffset,
};

static const char* const equationStrings[] = {
    "Add",
    "Subtract",
    "Multiply",
    "Divide safe",
    "Modulo",
    "Sine",
    "Cosine",
    "Tangent safe",
    "Absolute",
    "Square",
    "Sigmoid",
    "Tanh",
    "Exponential",
    "Log safe",
    "Soft clip",
    "Chebyshev T3",
    "Lorenz",
    "Modulo fold",
    "Sinc",
    "Gaussian",
    NULL,
};

static const char* const speedStrings[] = {
    "/16",
    "/8",
    "/4",
    "/2",
    "x1",
    "x2",
    "x3",
    "x4",
    "x8",
    NULL,
};

static const float speedMultipliers[] = {
    1.0f / 16.0f,
    1.0f / 8.0f,
    1.0f / 4.0f,
    1.0f / 2.0f,
    1.0f,
    2.0f,
    3.0f,
    4.0f,
    8.0f,
};

static const _NT_parameter parameters[] = {
    NT_PARAMETER_CV_INPUT("CV A", 1, 1)
    NT_PARAMETER_CV_INPUT("CV B", 1, 2)
    NT_PARAMETER_CV_OUTPUT_WITH_MODE("CV Out", 1, 13)
    { .name = "Equation", .min = 0, .max = mathulate::kNumEquations - 1, .def = mathulate::kEquationAdd, .unit = kNT_unitEnum, .scaling = kNT_scalingNone, .enumStrings = equationStrings },
    { .name = "Scale", .min = 0, .max = 200, .def = 100, .unit = kNT_unitPercent, .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "Offset", .min = -10000, .max = 10000, .def = 0, .unit = kNT_unitVolts, .scaling = kNT_scaling1000, .enumStrings = NULL },
    NT_PARAMETER_CV_INPUT("Clock In", 0, 0)
    { .name = "Speed", .min = 0, .max = ARRAY_SIZE(speedMultipliers) - 1, .def = kDefaultSpeed, .unit = kNT_unitEnum, .scaling = kNT_scalingNone, .enumStrings = speedStrings },
    { .name = "Phase", .min = 0, .max = 100, .def = 0, .unit = kNT_unitPercent, .scaling = kNT_scalingNone, .enumStrings = NULL },
};

static const uint8_t pageMain[] = { kParamEquation, kParamSpeed, kParamScale, kParamOffset, kParamPhaseOffset };
static const uint8_t pageRouting[] = { kParamInputA, kParamInputB, kParamClockIn, kParamOutput, kParamOutputMode };

static const _NT_parameterPage pages[] = {
    { .name = "Main", .numParams = ARRAY_SIZE(pageMain), .group = 0, .unused = { 0, 0 }, .params = pageMain },
    { .name = "Routing", .numParams = ARRAY_SIZE(pageRouting), .group = 0, .unused = { 0, 0 }, .params = pageRouting },
};

static const _NT_parameterPages parameterPages = {
    .numPages = ARRAY_SIZE(pages),
    .pages = pages,
};

static float clampf(float x, float lo, float hi) {
    return x < lo ? lo : (x > hi ? hi : x);
}

static float speedMultiplierForIndex(int index) {
    if (index < 0)
        index = 0;
    if (index >= (int)ARRAY_SIZE(speedMultipliers))
        index = ARRAY_SIZE(speedMultipliers) - 1;
    return speedMultipliers[index];
}

static float sampleRate() {
    return NT_globals.sampleRate > 0 ? (float)NT_globals.sampleRate : kFallbackSampleRate;
}

static void resetClock(MathulateDTC* dtc) {
    dtc->clockPeriodSamples = sampleRate();
    dtc->samplesSinceClock = 0;
    dtc->previousClockHigh = false;
    dtc->clockSeen = false;
}

static void initialiseDtc(MathulateDTC* dtc) {
    mathulate::resetState(&dtc->core);
    dtc->outputScale = 1.0f;
    dtc->outputOffset = 0.0f;
    dtc->phaseOffset = 0.0f;
    dtc->speedMultiplier = speedMultiplierForIndex(kDefaultSpeed);
    dtc->phase = 0.0f;
    dtc->displayPhase = 0.0f;
    dtc->displayOutput = 0.0f;
    dtc->displayB = 0.0f;
    resetClock(dtc);
}

static void calculateRequirements(_NT_algorithmRequirements& req, const int32_t*) {
    req.numParameters = ARRAY_SIZE(parameters);
    req.sram = sizeof(MathulateAlgorithm);
    req.dram = 0;
    req.dtc = sizeof(MathulateDTC);
    req.itc = 0;
}

static _NT_algorithm* construct(const _NT_algorithmMemoryPtrs& ptrs,
                                const _NT_algorithmRequirements&,
                                const int32_t*) {
    MathulateAlgorithm* alg = new (ptrs.sram) MathulateAlgorithm((MathulateDTC*)ptrs.dtc);
    alg->parameters = parameters;
    alg->parameterPages = &parameterPages;
    initialiseDtc(alg->dtc);
    return alg;
}

static void parameterChanged(_NT_algorithm* self, int p) {
    MathulateAlgorithm* pThis = (MathulateAlgorithm*)self;

    if (p == kParamScale)
        pThis->dtc->outputScale = pThis->v[kParamScale] * 0.01f;
    else if (p == kParamOffset)
        pThis->dtc->outputOffset = pThis->v[kParamOffset] * 0.001f;
    else if (p == kParamPhaseOffset)
        pThis->dtc->phaseOffset = pThis->v[kParamPhaseOffset] * 0.01f;
    else if (p == kParamSpeed)
        pThis->dtc->speedMultiplier = speedMultiplierForIndex(pThis->v[kParamSpeed]);
    else if (p == kParamClockIn)
        resetClock(pThis->dtc);
    else if (p == kParamEquation && pThis->v[kParamEquation] == mathulate::kEquationLorenz)
        mathulate::resetState(&pThis->dtc->core);
}

static int16_t clampParamValue(int p, int value) {
    if (value < parameters[p].min)
        value = parameters[p].min;
    if (value > parameters[p].max)
        value = parameters[p].max;
    return (int16_t)value;
}

static int valueFromPot(int p, float pot) {
    pot = clampf(pot, 0.0f, 1.0f);
    return parameters[p].min + (int)(pot * (parameters[p].max - parameters[p].min) + 0.5f);
}

static float potFromValue(int p, int value) {
    int range = parameters[p].max - parameters[p].min;
    if (range <= 0)
        return 0.0f;
    return clampf((value - parameters[p].min) / (float)range, 0.0f, 1.0f);
}

static void setParamFromUi(_NT_algorithm* self, int p, int value) {
    int32_t algIndex = NT_algorithmIndex(self);
    if (algIndex < 0)
        return;
    NT_setParameterFromUi((uint32_t)algIndex, p + NT_parameterOffset(), clampParamValue(p, value));
}

static void step(_NT_algorithm* self, float* busFrames, int numFramesBy4) {
    MathulateAlgorithm* pThis = (MathulateAlgorithm*)self;
    MathulateDTC* dtc = pThis->dtc;
    int numFrames = numFramesBy4 * 4;

    int inputAIndex = pThis->v[kParamInputA] - 1;
    int inputBIndex = pThis->v[kParamInputB] - 1;
    int outputIndex = pThis->v[kParamOutput] - 1;
    int clockIndex = pThis->v[kParamClockIn] - 1;
    if (inputAIndex < 0 || inputBIndex < 0 || outputIndex < 0)
        return;

    const float* inputA = busFrames + inputAIndex * numFrames;
    const float* inputB = busFrames + inputBIndex * numFrames;
    const float* clockIn = clockIndex >= 0 ? busFrames + clockIndex * numFrames : NULL;
    float* output = busFrames + outputIndex * numFrames;
    bool replace = pThis->v[kParamOutputMode];
    int equation = pThis->v[kParamEquation];
    float scale = dtc->outputScale;
    float offset = dtc->outputOffset;
    float sr = sampleRate();
    float internalPeriod = sr / clampf(dtc->speedMultiplier, 0.001f, 64.0f);

    for (int i = 0; i < numFrames; ++i) {
        float period = internalPeriod;

        if (clockIn) {
            bool clockHigh = clockIn[i] > kClockThreshold;
            bool rising = clockHigh && !dtc->previousClockHigh;
            if (rising) {
                if (dtc->clockSeen && dtc->samplesSinceClock > 0)
                    dtc->clockPeriodSamples = (float)dtc->samplesSinceClock;
                dtc->samplesSinceClock = 0;
                dtc->clockSeen = true;
                dtc->phase = 0.0f;
                mathulate::resetState(&dtc->core);
            }
            dtc->previousClockHigh = clockHigh;
            if (dtc->clockSeen)
                period = dtc->clockPeriodSamples / clampf(dtc->speedMultiplier, 0.001f, 64.0f);
            ++dtc->samplesSinceClock;
        }

        period = clampf(period, 1.0f, sr * 3600.0f);
        float effectivePhase = mathulate::wrapPhase(dtc->phase + dtc->phaseOffset + mathulate::normalizeCv(inputA[i]) * 0.5f);
        float volts = mathulate::processPhaseSample(&dtc->core, equation, dtc->phase, dtc->phaseOffset, inputA[i], inputB[i], scale, offset);
        output[i] = replace ? volts : (output[i] + volts);
        dtc->displayPhase = effectivePhase;
        dtc->displayOutput = volts;
        dtc->displayB = mathulate::normalizeCv(inputB[i]);
        dtc->phase = mathulate::wrapPhase(dtc->phase + 1.0f / period);
    }
}

static int graphY(float value) {
    value = clampf(value, -1.0f, 1.0f);
    const int top = 11;
    const int bottom = 45;
    return bottom - (int)((value + 1.0f) * 0.5f * (bottom - top) + 0.5f);
}

static float graphOutputValue(float raw, const MathulateDTC* dtc) {
    float normalizedOutput = clampf(raw, -1.0f, 1.0f) * dtc->outputScale +
                            dtc->outputOffset / mathulate::kMaxVoltage;
    return clampf(normalizedOutput, -1.0f, 1.0f);
}

static void drawGraph(MathulateAlgorithm* pThis) {
    const int left = 2;
    const int right = 253;
    const int top = 10;
    const int bottom = 46;
    const int midY = (top + bottom) / 2;
    const int equation = pThis->v[kParamEquation];
    MathulateDTC* dtc = pThis->dtc;

    NT_drawShapeI(kNT_box, left - 1, top - 1, right + 1, bottom + 1, 5);
    NT_drawShapeI(kNT_line, left, midY, right, midY, 3);
    NT_drawShapeI(kNT_line, (left + right) / 2, top, (left + right) / 2, bottom, 3);

    mathulate::State preview;
    mathulate::resetState(&preview);
    int prevX = left;
    int prevY = midY;
    for (int x = left; x <= right; x += 2) {
        float t = (x - left) / (float)(right - left);
        float a = mathulate::phaseToBipolar(t + dtc->phaseOffset);
        float raw = mathulate::evaluateNormalized(&preview, equation, a, dtc->displayB);
        int y = graphY(graphOutputValue(raw, dtc));
        if (x != left)
            NT_drawShapeI(kNT_line, prevX, prevY, x, y, 11);
        prevX = x;
        prevY = y;
    }

    int dotX = left + (int)(mathulate::wrapPhase(dtc->displayPhase - dtc->phaseOffset) * (right - left) + 0.5f);
    int dotY = graphY(dtc->displayOutput / mathulate::kMaxVoltage);
    NT_drawShapeI(kNT_rectangle, dotX - 1, dotY - 1, dotX + 1, dotY + 1, 15);
}

static bool draw(_NT_algorithm* self) {
    MathulateAlgorithm* pThis = (MathulateAlgorithm*)self;
    MathulateDTC* dtc = pThis->dtc;

    NT_drawText(0, 5, "MATHULATE", 15, kNT_textLeft, kNT_textTiny);
    NT_drawText(44, 5, kVersion, 8, kNT_textLeft, kNT_textTiny);
    bool clocked = pThis->v[kParamClockIn] > 0;
    NT_drawText(207, 5, clocked ? "CLK" : "INT", clocked && dtc->clockSeen ? 15 : 8, kNT_textLeft, kNT_textTiny);
    NT_drawText(229, 5, speedStrings[pThis->v[kParamSpeed]], 15, kNT_textLeft, kNT_textTiny);
    NT_drawShapeI(kNT_line, 0, 7, 255, 7, 5);

    drawGraph(pThis);

    char buf[16];
    NT_drawShapeI(kNT_line, 0, 49, 255, 49, 5);
    NT_drawText(0, 56, equationStrings[pThis->v[kParamEquation]], 15, kNT_textLeft, kNT_textTiny);

    NT_drawText(96, 56, "S", 8, kNT_textLeft, kNT_textTiny);
    NT_intToString(buf, pThis->v[kParamScale]);
    NT_drawText(104, 56, buf, 15, kNT_textLeft, kNT_textTiny);

    NT_drawText(136, 56, "O", 8, kNT_textLeft, kNT_textTiny);
    NT_floatToString(buf, pThis->v[kParamOffset] * 0.001f, 2);
    NT_drawText(144, 56, buf, 15, kNT_textLeft, kNT_textTiny);

    NT_drawText(196, 56, "P", 8, kNT_textLeft, kNT_textTiny);
    NT_intToString(buf, pThis->v[kParamPhaseOffset]);
    NT_drawText(204, 56, buf, 15, kNT_textLeft, kNT_textTiny);

    return true;
}

static uint32_t hasCustomUi(_NT_algorithm*) {
    return kNT_potL | kNT_potC | kNT_potR |
           kNT_potButtonL | kNT_potButtonC | kNT_potButtonR |
           kNT_encoderL | kNT_encoderR |
           kNT_encoderButtonL | kNT_encoderButtonR;
}

static void customUi(_NT_algorithm* self, const _NT_uiData& data) {
    MathulateAlgorithm* pThis = (MathulateAlgorithm*)self;

    if (data.controls & kNT_potL)
        setParamFromUi(self, kParamScale, valueFromPot(kParamScale, data.pots[0]));
    if (data.controls & kNT_potC)
        setParamFromUi(self, kParamOffset, valueFromPot(kParamOffset, data.pots[1]));
    if (data.controls & kNT_potR)
        setParamFromUi(self, kParamPhaseOffset, valueFromPot(kParamPhaseOffset, data.pots[2]));

    if (data.encoders[0] != 0)
        setParamFromUi(self, kParamEquation, pThis->v[kParamEquation] + data.encoders[0]);
    if (data.encoders[1] != 0)
        setParamFromUi(self, kParamSpeed, pThis->v[kParamSpeed] + data.encoders[1]);

    uint16_t pressed = data.controls & ~data.lastButtons;
    if (pressed & kNT_potButtonL)
        setParamFromUi(self, kParamScale, parameters[kParamScale].def);
    if (pressed & kNT_potButtonC)
        setParamFromUi(self, kParamOffset, parameters[kParamOffset].def);
    if (pressed & kNT_potButtonR)
        setParamFromUi(self, kParamPhaseOffset, parameters[kParamPhaseOffset].def);
    if (pressed & kNT_encoderButtonL) {
        int nextCategory = ((pThis->v[kParamEquation] / 5) + 1) % 4;
        setParamFromUi(self, kParamEquation, nextCategory * 5);
    }
    if (pressed & kNT_encoderButtonR) {
        pThis->dtc->phase = 0.0f;
        pThis->dtc->displayPhase = 0.0f;
        mathulate::resetState(&pThis->dtc->core);
    }
}

static void setupUi(_NT_algorithm* self, _NT_float3& pots) {
    pots[0] = potFromValue(kParamScale, self->v[kParamScale]);
    pots[1] = potFromValue(kParamOffset, self->v[kParamOffset]);
    pots[2] = potFromValue(kParamPhaseOffset, self->v[kParamPhaseOffset]);
}

static const _NT_factory factory = {
    .guid = NT_MULTICHAR('N', 's', 'M', 't'),
    .name = "Mathulate",
    .description = "Two-input CV math modulation source",
    .numSpecifications = 0,
    .specifications = NULL,
    .calculateStaticRequirements = NULL,
    .initialise = NULL,
    .calculateRequirements = calculateRequirements,
    .construct = construct,
    .parameterChanged = parameterChanged,
    .step = step,
    .draw = draw,
    .midiRealtime = NULL,
    .midiMessage = NULL,
    .tags = kNT_tagUtility,
    .hasCustomUi = hasCustomUi,
    .customUi = customUi,
    .setupUi = setupUi,
    .serialise = NULL,
    .deserialise = NULL,
    .midiSysEx = NULL,
    .parameterUiPrefix = NULL,
    .parameterString = NULL,
};

} // namespace

extern "C" uintptr_t pluginEntry(_NT_selector selector, uint32_t data) {
    switch (selector) {
    case kNT_selector_version:
        return kNT_apiVersionCurrent;
    case kNT_selector_numFactories:
        return 1;
    case kNT_selector_factoryInfo:
        return (uintptr_t)((data == 0) ? &factory : NULL);
    }
    return 0;
}
