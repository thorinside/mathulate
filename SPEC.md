# Design Specification: disting NT Math Function Plugin

Source: Substrate artifact `46b3ac7e-f350-44a6-8c6e-fc5af8f71be8`, “disting NT Math Function Plugin Spec”.

## Overview
A versatile CV processor for the disting NT that takes two normalized CV inputs and applies a selected mathematical equation to produce a single CV output. The goal is to provide a Swiss-Army-knife of mathematical transformations for sound design, ranging from basic arithmetic to chaotic and musically surprising functions.

## Signal Path
- **Inputs**:
  - CV Input 1 (Normalized)
  - CV Input 2 (Normalized)
- **Processing**:
  - The selected equation from the library is applied to the two inputs.
  - Results are bounded and stabilized using safe-math implementations.
- **Output Stage**:
  - The raw result is processed through Scale and Offset, then clamped to -10V to +10V.
- **Output**: Single CV Output.

## Parameters
- **Equation Select**: 20 mathematical functions.
- **Output Scale**: 0–200%.
- **Output Offset**: -10V to +10V.

## Mathematical Library
1. Addition
2. Subtraction
3. Multiplication
4. Division (safe)
5. Modulo
6. Sine
7. Cosine
8. Tangent (safe)
9. Absolute Value
10. Square
11. Sigmoid
12. Hyperbolic Tangent
13. Exponential (safe)
14. Logarithmic (safe)
15. Soft-Clip
16. Chebyshev Polynomial
17. Lorenz System (simplified)
18. Modulo Folding
19. Sinc Function
20. Gaussian Bell
