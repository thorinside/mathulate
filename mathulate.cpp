/*
 * Mathulate - disting NT CV math processor
 *
 * Generated from Substrate artifact "Disting NT Math Function Plugin Spec".
 * Takes two CV inputs, normalizes them to -1..+1, applies one of twenty
 * safe mathematical functions, then emits bounded -10V..+10V CV.
 */

#include <distingnt/api.h>
#include <cstddef>
#include <new>

#include "mathulate_core.h"

namespace {

struct MathulateDTC {
    mathulate::State core;
    float outputScale;
    float outputOffset;
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

static const _NT_parameter parameters[] = {
    NT_PARAMETER_CV_INPUT("CV A", 1, 1)
    NT_PARAMETER_CV_INPUT("CV B", 1, 2)
    NT_PARAMETER_CV_OUTPUT_WITH_MODE("CV Out", 1, 13)
    { .name = "Equation", .min = 0, .max = mathulate::kNumEquations - 1, .def = mathulate::kEquationAdd, .unit = kNT_unitEnum, .scaling = kNT_scalingNone, .enumStrings = equationStrings },
    { .name = "Scale", .min = 0, .max = 200, .def = 100, .unit = kNT_unitPercent, .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "Offset", .min = -10000, .max = 10000, .def = 0, .unit = kNT_unitVolts, .scaling = kNT_scaling1000, .enumStrings = NULL },
};

static const uint8_t pageMain[] = { kParamEquation, kParamScale, kParamOffset };
static const uint8_t pageRouting[] = { kParamInputA, kParamInputB, kParamOutput, kParamOutputMode };

static const _NT_parameterPage pages[] = {
    { .name = "Main", .numParams = ARRAY_SIZE(pageMain), .group = 0, .unused = { 0, 0 }, .params = pageMain },
    { .name = "Routing", .numParams = ARRAY_SIZE(pageRouting), .group = 0, .unused = { 0, 0 }, .params = pageRouting },
};

static const _NT_parameterPages parameterPages = {
    .numPages = ARRAY_SIZE(pages),
    .pages = pages,
};

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

    mathulate::resetState(&alg->dtc->core);
    alg->dtc->outputScale = 1.0f;
    alg->dtc->outputOffset = 0.0f;

    return alg;
}

static void parameterChanged(_NT_algorithm* self, int p) {
    MathulateAlgorithm* pThis = (MathulateAlgorithm*)self;

    if (p == kParamScale)
        pThis->dtc->outputScale = pThis->v[kParamScale] * 0.01f;
    else if (p == kParamOffset)
        pThis->dtc->outputOffset = pThis->v[kParamOffset] * 0.001f;
    else if (p == kParamEquation && pThis->v[kParamEquation] == mathulate::kEquationLorenz)
        mathulate::resetState(&pThis->dtc->core);
}

static void step(_NT_algorithm* self, float* busFrames, int numFramesBy4) {
    MathulateAlgorithm* pThis = (MathulateAlgorithm*)self;
    MathulateDTC* dtc = pThis->dtc;
    int numFrames = numFramesBy4 * 4;

    int inputAIndex = pThis->v[kParamInputA] - 1;
    int inputBIndex = pThis->v[kParamInputB] - 1;
    int outputIndex = pThis->v[kParamOutput] - 1;
    if (inputAIndex < 0 || inputBIndex < 0 || outputIndex < 0)
        return;

    const float* inputA = busFrames + inputAIndex * numFrames;
    const float* inputB = busFrames + inputBIndex * numFrames;
    float* output = busFrames + outputIndex * numFrames;
    bool replace = pThis->v[kParamOutputMode];
    int equation = pThis->v[kParamEquation];
    float scale = dtc->outputScale;
    float offset = dtc->outputOffset;

    for (int i = 0; i < numFrames; ++i) {
        float volts = mathulate::processSample(&dtc->core, equation, inputA[i], inputB[i], scale, offset);
        output[i] = replace ? volts : (output[i] + volts);
    }
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
    .draw = NULL,
    .midiRealtime = NULL,
    .midiMessage = NULL,
    .tags = kNT_tagUtility,
    .hasCustomUi = NULL,
    .customUi = NULL,
    .setupUi = NULL,
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
