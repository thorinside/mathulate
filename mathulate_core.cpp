#include "mathulate_core.h"

#include <math.h>

namespace mathulate {
namespace {

static const float kPi = 3.14159265358979323846f;

static float clampf(float x, float lo, float hi) {
    return x < lo ? lo : (x > hi ? hi : x);
}

static float finiteOrZero(float x) {
    return isfinite(x) ? x : 0.0f;
}

static float safeDivide(float a, float b) {
    float denom = fabsf(b) < kEpsilon ? (b < 0.0f ? -kEpsilon : kEpsilon) : b;
    return a / denom;
}

static float safeModulo(float a, float b) {
    float denom = fabsf(b) < kEpsilon ? (b < 0.0f ? -kEpsilon : kEpsilon) : b;
    return fmodf(a, denom);
}

static float triangleFold(float x) {
    float phase = fmodf(x + 1.0f, 4.0f);
    if (phase < 0.0f)
        phase += 4.0f;
    return (phase < 2.0f) ? (phase - 1.0f) : (3.0f - phase);
}

static float softClip(float x) {
    return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}

static float processLorenz(State* state, float a, float b) {
    if (!state)
        return 0.0f;

    const float sigma = 10.0f;
    const float rho = 28.0f + 4.0f * a;
    const float beta = 8.0f / 3.0f;
    const float dt = 0.0005f * (1.0f + 4.0f * fabsf(b));

    float dx = sigma * (state->lorenzY - state->lorenzX);
    float dy = state->lorenzX * (rho - state->lorenzZ) - state->lorenzY;
    float dz = state->lorenzX * state->lorenzY - beta * state->lorenzZ;

    state->lorenzX = clampf(finiteOrZero(state->lorenzX + dx * dt), -40.0f, 40.0f);
    state->lorenzY = clampf(finiteOrZero(state->lorenzY + dy * dt), -40.0f, 40.0f);
    state->lorenzZ = clampf(finiteOrZero(state->lorenzZ + dz * dt), 0.0f, 60.0f);

    return clampf(state->lorenzX / 30.0f, -1.0f, 1.0f);
}

} // namespace

void resetState(State* state) {
    if (!state)
        return;
    state->lorenzX = 0.1f;
    state->lorenzY = 0.0f;
    state->lorenzZ = 0.0f;
}

float normalizeCv(float volts) {
    return clampf(finiteOrZero(volts) / kMaxVoltage, -1.0f, 1.0f);
}

float evaluateNormalized(State* state, int equation, float a, float b) {
    a = clampf(finiteOrZero(a), -1.0f, 1.0f);
    b = clampf(finiteOrZero(b), -1.0f, 1.0f);

    switch (equation) {
    case kEquationAdd:
        return a + b;
    case kEquationSubtract:
        return a - b;
    case kEquationMultiply:
        return a * b;
    case kEquationDivide:
        return safeDivide(a, b);
    case kEquationModulo:
        return safeModulo(a, b);
    case kEquationSine:
        return sinf(a * kPi);
    case kEquationCosine:
        return cosf(a * kPi);
    case kEquationTangent:
        return tanhf(tanf(clampf(a, -0.48f, 0.48f) * kPi));
    case kEquationAbsolute:
        return fabsf(a);
    case kEquationSquare:
        return a * a;
    case kEquationSigmoid:
        return 1.0f / (1.0f + expf(-6.0f * a));
    case kEquationTanh:
        return tanhf(2.5f * a);
    case kEquationExponential:
        return (expf(clampf(a, -1.0f, 1.0f)) - 1.0f) / 1.71828182845904523536f;
    case kEquationLog: {
        float unipolar = clampf((a + 1.0f) * 0.5f, kEpsilon, 1.0f);
        return logf(unipolar) / -logf(kEpsilon);
    }
    case kEquationSoftClip:
        return softClip(2.0f * a);
    case kEquationChebyshev:
        return (4.0f * a * a * a) - (3.0f * a);
    case kEquationLorenz:
        return processLorenz(state, a, b);
    case kEquationModuloFold:
        return triangleFold((a * 2.0f) + b);
    case kEquationSinc: {
        float x = a * kPi;
        return fabsf(x) < kEpsilon ? 1.0f : sinf(x) / x;
    }
    case kEquationGaussian:
        return expf(-4.0f * a * a);
    default:
        return 0.0f;
    }
}

float processSample(State* state, int equation, float cvA, float cvB,
                    float outputScale, float outputOffset) {
    float a = normalizeCv(cvA);
    float b = normalizeCv(cvB);
    float raw = finiteOrZero(evaluateNormalized(state, equation, a, b));
    float scale = finiteOrZero(outputScale);
    float offset = finiteOrZero(outputOffset);
    return clampf(clampf(raw, -1.0f, 1.0f) * kMaxVoltage * scale + offset,
                  -kMaxVoltage, kMaxVoltage);
}

} // namespace mathulate
