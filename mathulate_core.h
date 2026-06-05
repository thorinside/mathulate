#ifndef MATHULATE_CORE_H
#define MATHULATE_CORE_H

namespace mathulate {

static const float kMaxVoltage = 10.0f;
static const float kEpsilon = 1.0e-5f;

enum Equation {
    kEquationAdd,
    kEquationSubtract,
    kEquationMultiply,
    kEquationDivide,
    kEquationModulo,
    kEquationSine,
    kEquationCosine,
    kEquationTangent,
    kEquationAbsolute,
    kEquationSquare,
    kEquationSigmoid,
    kEquationTanh,
    kEquationExponential,
    kEquationLog,
    kEquationSoftClip,
    kEquationChebyshev,
    kEquationLorenz,
    kEquationModuloFold,
    kEquationSinc,
    kEquationGaussian,
    kNumEquations,
};

struct State {
    float lorenzX;
    float lorenzY;
    float lorenzZ;
};

void resetState(State* state);
float normalizeCv(float volts);
float evaluateNormalized(State* state, int equation, float a, float b);
float processSample(State* state, int equation, float cvA, float cvB,
                    float outputScale, float outputOffset);

} // namespace mathulate

#endif // MATHULATE_CORE_H
