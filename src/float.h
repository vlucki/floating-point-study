#ifndef FLOAT_H
#define FLOAT_H

#include "logger.h"
#include <bits/stdint-uintn.h>
#include <cmath>
#include <cstring>

// RELEVANT LINKS
// https://bit-calculator.com/bit-shift-calculator
// https://float.exposed/0x387fc000
// https://www.cprogramming.com/tutorial/floating_point/understanding_floating_point_representation.html
// https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Source/Runtime/Core/Public/Math/Float16.h
namespace Math
{
    using uint = unsigned int;
    using uchar = unsigned char;

    /**
     * 8-bit float representation
     * Contains 1 sign bit, 3 exponent bits (with a bias of 3) and 4 mantissa bits (S EEE MMMM)
     */
    struct MiniFloat
    {
        public:

            // S EEE EEEE E MMM MMMM MMMM MMMM MMMM MMMM  -> float
            static constexpr uint floatSignMask     = 1 << 31;    // 1000 0000 0000 0000 0000 0000 0000 0000
            static constexpr uint floatExponentMask = 0xFF << 23; // 0111 1111 1000 0000 0000 0000 0000 0000
            static constexpr uint floatMantissaMask = 0x7FFFFF;   // 0000 0000 0111 1111 1111 1111 1111 1111
            static constexpr uint floatBias = 127;

            static constexpr uchar miniFloatMaxExponent = 7;            // 0110

            static constexpr uchar miniFloatSignBitmask = 1 << 7;       // 1000 0000
            static constexpr uchar miniFloatExponentBitmask = 7 << 4;   // 0111 0000
            static constexpr uchar miniFloatMantissaBitmask = 0xF;      // 0000 1111

            MiniFloat() : bits(0)
            {
            }

            MiniFloat(uchar bits) : bits(bits)
            {
                log_v("{MiniFloat(float)} Creating MiniFloat from bit pattern (" << GetBitString(bits) << ")");
            }

            MiniFloat(float value) : MiniFloat()
            {
                // TODO: Review implementation based on Unreal's FFloat16
                log_v("{MiniFloat(float)} Creating MiniFloat from float (" << value << ")");
                // Easy return
                if(value == 0.0f)
                {
                    log_v("{MiniFloat(float)} MiniFloat with value of 0 created");
                    return;
                }

                // Read bits from float
                uint floatBits;
                std::memcpy(&floatBits, &value, sizeof(float));

                log_v("{MiniFloat(float)} float (" << value << ") bits = " << GetBitString(floatBits));

                // Extract sign bit from float
                const uint floatSign = floatBits & floatSignMask;
                log_v("{MiniFloat(float)} float sign bit = " << (floatSign ? "1" : "0"));

                // Move float sign bit to correct position in mini float (8th) and copy it
                const uchar miniFloatSign = floatSign >> 24;
                bits |= miniFloatSign;
                log_v("{MiniFloat(float)} MiniFloat sign bit = " << (miniFloatSign ? "1" : "0"));

                const uint maskedFloatExponent = floatBits & floatExponentMask;
                const uint shiftedFloatExponent = maskedFloatExponent >> 23;
                const int unbiasedFloatExponent = shiftedFloatExponent - floatBias;

                log_v("{MiniFloat(float)} float masked exponent = " << GetBitString(maskedFloatExponent));
                log_v("{MiniFloat(float)} float exponent = " << shiftedFloatExponent << " (" << GetBitString(shiftedFloatExponent, true) << ")");
                log_v("{MiniFloat(float)} float unbiased exponent = " << unbiasedFloatExponent);

                const uint floatMantissa = floatBits & floatMantissaMask;

                // Check if exponent is too small
                if (shiftedFloatExponent <= 0 + floatBias - bias) // 0 + 127 - 3 = 124
                {
                    log_v("{MiniFloat(float)} Unbiased float exponent is too small for mini float");
                    const int NewExp = unbiasedFloatExponent + bias;
                    log_v("{MiniFloat(float)} expectedMiniFloatExponent = " << NewExp);

                    // Check if mini mantissa should be non-zero
                    if (bias - 1 - NewExp <= 24)
                    {
                        // still not 100% sure about this line - how is it accounting for the "hidden 1 bit"?
                        const uint hiddenOneFloatMantissa = floatMantissa | 0x800000;
                        const uint mantissaShift = 24 - 4 - NewExp; // float mantissa bits - mini float mantissa bits + 1
                        log_v("{MiniFloat(float)} hiddenOneFloatMantissa mantissa = " << hiddenOneFloatMantissa << " (" << GetBitString(hiddenOneFloatMantissa) << ")");
                        bits |= (uchar)(hiddenOneFloatMantissa >> mantissaShift);
                        const uint shiftedFloatMantissa = (hiddenOneFloatMantissa >> (23 - 4 - NewExp));
                        log_v("{MiniFloat(float)} shiftedFloatMantissa = " << GetBitString(shiftedFloatMantissa));
                        if (shiftedFloatMantissa & 1)
                        {
                            log_v("{MiniFloat(float)} Adding 1 to bits (round)");
                            ++bits;
                        }
                    }
                }
                else if (unbiasedFloatExponent + bias >= 7) // Check if exponent too large
                {
                    log_v("{MiniFloat(float)} Unbiased float exponent is too big for mini float");
                    // Infinity
                    bits |= miniFloatExponentBitmask;
                    // NaN float
                    if (unbiasedFloatExponent == 255 && floatMantissa)
                    {
                        log_v("{MiniFloat(float)} NaN float detected");
                        bits |= miniFloatMantissaBitmask;
                    }
                }
                else
                {
                    const uchar miniExponent = (unbiasedFloatExponent + bias) << 4; // move exponent into proper mini float position
                    const uchar miniMantissa = floatMantissa >> (23 - 4); // only get 4 most relevant mantissa bits
                    log_v("{MiniFloat(float)} MiniFloat exponent = " << (uint) miniExponent << " (" << GetBitString(miniExponent) << ")");
                    log_v("{MiniFloat(float)} MiniFloat mantissa = " << (uint) miniMantissa << " (" << GetBitString(miniMantissa) << ")");
                    bits |= miniExponent | miniMantissa;
                }

                log_v("{MiniFloat(float)} MiniFloat = " << (uint)bits << " (" << GetBitString(bits) << ")");
            }

            float ToFloat() const
            {
                log_v("{ToFloat()} Converting MiniFloat " << GetBitString(bits) << " to float");
                // Easy return
                if(bits == 0)
                {
                    log_v("{ToFloat()} MiniFloat is zero (0)");
                    return 0.0f;
                }
                // Create 32 bits mask
                uint floatBits = 0;

                const uchar miniSign = GetSign();
                const uchar miniExponent = GetExponent();
                const uchar miniMantissa = GetMantissa();
                log_v("{ToFloat()} MiniFloat masked sign bit = " << GetBitString(miniSign));
                log_v("{ToFloat()} MiniFloat masked exponent = " << GetBitString(miniExponent));
                log_v("{ToFloat()} MiniFloat masked mantissa = " << GetBitString(miniMantissa));

                const uint floatSign = miniSign << 24;
                log_v("{ToFloat()} float masked sign bit = " << GetBitString(floatSign));
                // Copy sign bit
                floatBits |= floatSign;

                if (miniExponent == 0) // Denormalized
                {
                    log_v("{ToFloat()} MiniFloat is denormalized");
                    if (miniMantissa != 0) // if false, then it's zero
                    {
                        float mantissaAsFloat = 0.f;
                        std::memcpy(&mantissaAsFloat, &miniMantissa, sizeof(float));
                        // Up to 4 bits for mantissa
                        const uint mantissaShift = 4 - (int)std::log2(mantissaAsFloat);
                        const uint floatExponent = floatBias - (bias - 1) - mantissaShift;
                        const uint floatMantissa = miniMantissa << (mantissaShift + 23 - 4); // 23 for float, 4 for MiniFloat
                        floatBits |= (floatExponent << 23) | floatMantissa;
                    }
                }
                else if (miniExponent == miniFloatExponentBitmask) // infinity or NaN
                {
                    // set float to largest MiniFloat -> 15
                    floatBits |= floatExponentMask;
                    floatBits |= miniMantissa;
                }
                else // Normalized
                {
                    // Shift bits right, so it's easier to calculate things
                    const uchar shiftedMiniExponent = miniExponent >> 4;
                    log_v("{ToFloat()} MiniFloat exponent = " << (uint)shiftedMiniExponent << " (" << GetBitString(shiftedMiniExponent, true) << ")");

                    // Subtract bias (may be negative, so result must be signed)
                    const char biasedMiniExponent = shiftedMiniExponent - bias;
                    log_v("{ToFloat()} MiniFloat biased exponent = " << (int)biasedMiniExponent);

                    // Since regular float bias > mini float bias, result will always be > 0 so it can be unsigned
                    const uint floatExponent = biasedMiniExponent + floatBias;
                    log_v("{ToFloat()} float exponent = " << floatExponent << " (" << GetBitString(floatExponent, true) << ")");

                    // move exponent to correct place in regular float
                    floatBits |= floatExponent << 23;
                    // mantissa can be copied directly, just shift it to the correct place
                    const uint floatMantissa = miniMantissa << 19;
                    log_v("{ToFloat()} float mantissa = " << floatMantissa << " (" << GetBitString(floatMantissa) << ")");
                    floatBits |= floatMantissa;
                }

                // Copy bits into actual float
                float result;
                std::memcpy(&result, &floatBits, sizeof(float));
                log_v("{ToFloat()} reconstructed float = " << result << " (" << GetBitString(floatBits) << ")");
                return result;
            }

            operator uchar&()
            {
                return bits;
            }

            operator uchar() const
            {
                return bits;
            }

            uchar GetBits() const { return bits; }
            uchar GetSign() const { return bits & miniFloatSignBitmask; }
            uchar GetExponent() const { return bits & miniFloatExponentBitmask; }
            uchar GetMantissa() const { return bits & miniFloatMantissaBitmask; }

        private:
            // S EEE MMMM
            uchar bits;
            static constexpr uchar bias = 3; // subtracted from exponent


    };
}

#endif /* FLOAT_H */
