#include "mathulate_core.h"

#include <cfenv>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

#pragma STDC FENV_ACCESS ON

namespace {

int gFailures = 0;

void fail(const char* testName, int equation, float a, float b, float value) {
    std::printf("FAIL %-32s eq=%d a=%g b=%g value=%g\n", testName, equation, a, b, value);
    ++gFailures;
}

void checkFinite(const char* testName, int equation, float a, float b, float value) {
    if (!std::isfinite(value))
        fail(testName, equation, a, b, value);
}

void checkFiniteBoundedCv(const char* testName, int equation, float a, float b, float value) {
    if (!std::isfinite(value) || value < -mathulate::kMaxVoltage || value > mathulate::kMaxVoltage)
        fail(testName, equation, a, b, value);
}

void checkNoFloatingExceptions(const char* testName, int equation, float a, float b) {
    int flags = std::fetestexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
    if (flags != 0) {
        std::printf("FAIL %-32s eq=%d a=%g b=%g fenv=0x%x\n", testName, equation, a, b, flags);
        ++gFailures;
    }
}

void checkStateFinite(const char* testName, const mathulate::State& state) {
    if (!std::isfinite(state.lorenzX) || !std::isfinite(state.lorenzY) || !std::isfinite(state.lorenzZ)) {
        std::printf("FAIL %-32s lorenz=(%g,%g,%g)\n", testName, state.lorenzX, state.lorenzY, state.lorenzZ);
        ++gFailures;
    }
}

void allEquationsProduceFiniteRawValues() {
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const float values[] = { -inf, -100.0f, -1.0f, -0.5f, -mathulate::kEpsilon, -0.0f,
                             0.0f, mathulate::kEpsilon, 0.5f, 1.0f, 100.0f, inf, nan };

    for (int equation = 0; equation < mathulate::kNumEquations; ++equation) {
        mathulate::State state;
        mathulate::resetState(&state);
        for (size_t ai = 0; ai < sizeof(values) / sizeof(values[0]); ++ai) {
            for (size_t bi = 0; bi < sizeof(values) / sizeof(values[0]); ++bi) {
                std::feclearexcept(FE_ALL_EXCEPT);
                float raw = mathulate::evaluateNormalized(&state, equation, values[ai], values[bi]);
                checkNoFloatingExceptions("raw fenv clean", equation, values[ai], values[bi]);
                checkFinite("raw finite", equation, values[ai], values[bi], raw);
                checkStateFinite("raw state finite", state);
            }
        }
    }
}

void allEquationsProduceFiniteBoundedCvOutput() {
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const float cvValues[] = { -inf, -10000.0f, -10.0f, -1.0f, -0.0001f, -0.0f,
                               0.0f, 0.0001f, 1.0f, 10.0f, 10000.0f, inf, nan };
    const float scaleOffset[][2] = {
        { 1.0f, 0.0f },
        { 0.0f, 0.0f },
        { 2.0f, -10.0f },
        { 2.0f, 10.0f },
        { -1000.0f, 10000.0f },
        { inf, nan },
    };

    for (int equation = 0; equation < mathulate::kNumEquations; ++equation) {
        mathulate::State state;
        mathulate::resetState(&state);
        for (size_t si = 0; si < sizeof(scaleOffset) / sizeof(scaleOffset[0]); ++si) {
            for (size_t ai = 0; ai < sizeof(cvValues) / sizeof(cvValues[0]); ++ai) {
                for (size_t bi = 0; bi < sizeof(cvValues) / sizeof(cvValues[0]); ++bi) {
                    std::feclearexcept(FE_ALL_EXCEPT);
                    float out = mathulate::processSample(&state, equation, cvValues[ai], cvValues[bi],
                                                         scaleOffset[si][0], scaleOffset[si][1]);
                    checkNoFloatingExceptions("output fenv clean", equation, cvValues[ai], cvValues[bi]);
                    checkFiniteBoundedCv("output finite bounded", equation, cvValues[ai], cvValues[bi], out);
                    checkStateFinite("output state finite", state);
                }
            }
        }
    }
}

void zeroAndNearZeroDenominatorsStayFinite() {
    const float numerators[] = { -1.0f, -0.5f, -mathulate::kEpsilon, 0.0f,
                                 mathulate::kEpsilon, 0.5f, 1.0f };
    const float denominators[] = { -mathulate::kEpsilon, -1.0e-12f, -0.0f,
                                   0.0f, 1.0e-12f, mathulate::kEpsilon };
    const int equations[] = { mathulate::kEquationDivide, mathulate::kEquationModulo };

    for (size_t ei = 0; ei < sizeof(equations) / sizeof(equations[0]); ++ei) {
        for (size_t ai = 0; ai < sizeof(numerators) / sizeof(numerators[0]); ++ai) {
            for (size_t bi = 0; bi < sizeof(denominators) / sizeof(denominators[0]); ++bi) {
                mathulate::State state;
                mathulate::resetState(&state);
                std::feclearexcept(FE_ALL_EXCEPT);
                float raw = mathulate::evaluateNormalized(&state, equations[ei], numerators[ai], denominators[bi]);
                checkNoFloatingExceptions("zero denominator raw fenv", equations[ei], numerators[ai], denominators[bi]);
                checkFinite("zero denominator raw", equations[ei], numerators[ai], denominators[bi], raw);
                std::feclearexcept(FE_ALL_EXCEPT);
                float out = mathulate::processSample(&state, equations[ei], numerators[ai] * 10.0f,
                                                     denominators[bi] * 10.0f, 1.0f, 0.0f);
                checkNoFloatingExceptions("zero denominator out fenv", equations[ei], numerators[ai], denominators[bi]);
                checkFiniteBoundedCv("zero denominator output", equations[ei], numerators[ai], denominators[bi], out);
            }
        }
    }
}

void singularityProneEquationsStayFinite() {
    const int equations[] = {
        mathulate::kEquationDivide,
        mathulate::kEquationModulo,
        mathulate::kEquationTangent,
        mathulate::kEquationLog,
        mathulate::kEquationSinc,
    };
    const float cvValues[] = { -10.0f, -9.9999f, -4.8001f, -4.8f, -4.7999f,
                               -0.0001f, 0.0f, 0.0001f,
                               4.7999f, 4.8f, 4.8001f, 9.9999f, 10.0f };

    for (size_t ei = 0; ei < sizeof(equations) / sizeof(equations[0]); ++ei) {
        mathulate::State state;
        mathulate::resetState(&state);
        for (size_t ai = 0; ai < sizeof(cvValues) / sizeof(cvValues[0]); ++ai) {
            for (size_t bi = 0; bi < sizeof(cvValues) / sizeof(cvValues[0]); ++bi) {
                std::feclearexcept(FE_ALL_EXCEPT);
                float out = mathulate::processSample(&state, equations[ei], cvValues[ai], cvValues[bi], 1.0f, 0.0f);
                checkNoFloatingExceptions("singularity-prone fenv", equations[ei], cvValues[ai], cvValues[bi]);
                checkFiniteBoundedCv("singularity-prone output", equations[ei], cvValues[ai], cvValues[bi], out);
            }
        }
    }
}

void lorenzLongRunStaysFiniteAndBounded() {
    mathulate::State state;
    mathulate::resetState(&state);

    for (int i = 0; i < 100000; ++i) {
        float cvA = 10.0f * std::sin(i * 0.013f);
        float cvB = 10.0f * std::cos(i * 0.017f);
        std::feclearexcept(FE_ALL_EXCEPT);
        float out = mathulate::processSample(&state, mathulate::kEquationLorenz, cvA, cvB, 2.0f, 0.0f);
        checkNoFloatingExceptions("lorenz long-run fenv", mathulate::kEquationLorenz, cvA, cvB);
        checkFiniteBoundedCv("lorenz long-run output", mathulate::kEquationLorenz, cvA, cvB, out);
        checkStateFinite("lorenz long-run state", state);
    }
}

void invalidEquationIsSafeSilence() {
    mathulate::State state;
    mathulate::resetState(&state);
    float out = mathulate::processSample(&state, 999, 10.0f, 0.0f, 1.0f, 0.0f);
    if (out != 0.0f)
        fail("invalid equation silence", 999, 10.0f, 0.0f, out);
}

} // namespace

int main() {
    if (mathulate::kNumEquations != 20) {
        std::printf("FAIL equation count expected 20 got %d\n", mathulate::kNumEquations);
        ++gFailures;
    }

    allEquationsProduceFiniteRawValues();
    allEquationsProduceFiniteBoundedCvOutput();
    zeroAndNearZeroDenominatorsStayFinite();
    singularityProneEquationsStayFinite();
    lorenzLongRunStaysFiniteAndBounded();
    invalidEquationIsSafeSilence();

    if (gFailures != 0) {
        std::printf("%d Mathulate safety check(s) failed.\n", gFailures);
        return EXIT_FAILURE;
    }

    std::printf("Mathulate safety tests passed.\n");
    return EXIT_SUCCESS;
}
