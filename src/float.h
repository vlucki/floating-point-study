#ifndef FLOAT_H
#define FLOAT_H

#include "logger.h"
#include <bits/stdint-uintn.h>
#include <cmath>
#include <cstring>

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
                log_v("Creating MiniFloat from bit pattern (" << GetBitString(bits) << ")");
            }

            MiniFloat(float value) : MiniFloat()
            {

                log_v("Creating MiniFloat from float (" << value << ")");

                // Read bits from float
                uint floatBits;
                std::memcpy(&floatBits, &value, sizeof(float));

                log_v("float (" << value << ") bits = " << GetBitString(floatBits));

                // Extract sign bit from float
                const uint floatSign = floatBits & floatSignMask;
                log_v("float sign bit = " << (floatSign ? "1" : "0"));

                // Move float sign bit to correct position in mini float (8th) and copy it
                const uchar miniFloatSign = floatSign >> 24;
                bits |= miniFloatSign;
                log_v("MiniFloat sign bit = " << (miniFloatSign ? "1" : "0"));

                const uint maskedFloatExponent = floatBits & floatExponentMask;

                log_v("float masked exponent = " << GetBitString(maskedFloatExponent));

                const uint shiftedFloatExponent = maskedFloatExponent >> 23;
                const int unbiasedFloatExponent = (maskedFloatExponent ? shiftedFloatExponent : 1) - floatBias; // denormal float exponent = -126
                log_v("float exponent = " << shiftedFloatExponent << " (" << GetBitString(shiftedFloatExponent, true) << ")");
                log_v("float unbiased exponent = " << unbiasedFloatExponent);

                const uint floatMantissa = floatBits & floatMantissaMask;

                if (!maskedFloatExponent)
                {
                    log_v("Float denormalized. Too small for MiniFloat - assuming 0");
                }
                else if (maskedFloatExponent == floatExponentMask)
                {
                    log_v("Float is infinity or NaN");
                    bits |= miniFloatExponentBitmask;
                    if (floatMantissa)
                    {
                        // Make mini-float NaN
                        ++bits;
                    }
                }
                else
                {
                    log_v("Float normalized");

                    const char expectedMiniExponent = (unbiasedFloatExponent + bias);
                    log_v("Expected MiniFloat exponent = " << (int) expectedMiniExponent);
                    if (expectedMiniExponent >= 8)
                    {
                        log_v("Overflow detected: Float exponent is too big. Assuming largest value possible.");
                        bits |= (miniFloatExponentBitmask >> 5) << 5; // ignore lowest exp bit so mini float doesn't become infinity.
                        bits |= miniFloatMantissaBitmask;
                    }
                    else if (expectedMiniExponent <= 0)
                    {
                        log_v("Underflow detected: Float exponent is too small");
                        const uint mantissaShift = 20 - expectedMiniExponent;
                        if (mantissaShift <= 24)
                        {
                            log_v("Possible non-zero mantissa detected.");
                            const uint mantissaWithExplicitOne = floatMantissa | 0x800000;
                            const uchar miniMantissa = mantissaWithExplicitOne >> mantissaShift;
                            bits |= miniMantissa;
                            if ((mantissaWithExplicitOne >> (mantissaShift - 1)) & 1)
                            {
                                log_v("Rounding mini float up");
                                ++bits;
                            }

                        }
                    }
                    else
                    {
                        const uchar miniMantissa = floatMantissa >> (23 - 4); // only get 4 most relevant mantissa bits
                        const uchar miniExponent = expectedMiniExponent << 4;
                        log_v("MiniFloat mantissa = " << (uint) miniMantissa << " (" << GetBitString(miniMantissa) << ")");
                        log_v("MiniFloat exponent = " << (uint) miniExponent << " (" << GetBitString(miniExponent) << ")");
                        bits |= miniExponent | miniMantissa;
                        // 100 0000 0000 0000 0000
                        // Rounding check - seems unecessary
                        if (floatMantissa & 0x40000)
                        {
                            log_v("Rounding mini float up");
                            ++bits;
                        }
                    }
                }

                log_v("MiniFloat = " << (uint)bits << " (" << GetBitString(bits) << ")");
            }

            float ToFloat() const
            {
                log_v("Converting MiniFloat " << GetBitString(bits) << " to float");

                // Create 32 bits mask
                uint floatBits = 0;

                const uchar miniSign = GetSign();
                const uchar miniExponent = GetExponent();
                const uchar miniMantissa = GetMantissa();
                log_v("MiniFloat masked sign bit = " << GetBitString(miniSign));
                log_v("MiniFloat masked exponent = " << GetBitString(miniExponent));
                log_v("MiniFloat masked mantissa = " << GetBitString(miniMantissa));

                const uint floatSign = miniSign << 24;
                log_v("float masked sign bit = " << GetBitString(floatSign));
                // Copy sign bit
                floatBits |= floatSign;

                if (miniExponent == 0) // Denormal
                {
                    log_v("MiniFloat is denormal");
                    if (miniMantissa != 0)
                    {
                        // Try to extract float exponent from mini float mantissa
                        int e = -1;
                        uint m = miniMantissa;
                        do
                        {
                            ++e;
                            m <<= 1;
                        } while ((m & 0b10000) == 0); // while we haven't reached the first mini mantissa exponent bit

                        const uint floatMantissa = (m & miniFloatMantissaBitmask) << 19; // Extract actual mantissa and shift it in place
                        const uint floatExponent = (floatBias - bias - e) << 23;
                        log_v("Float mantissa = " << GetBitString(floatMantissa));
                        log_v("Float exponent = " << GetBitString(floatExponent));

                        floatBits |= floatMantissa;
                        floatBits |= floatExponent;         // Calculate exponent and shift it as well

                    } // else, mini float is zero
                }
                else
                {
                    // mantissa can be copied directly, just shift it to the correct place (works for NaN as well)
                    const uint floatMantissa = miniMantissa << 19;
                    log_v("float mantissa = " << floatMantissa << " (" << GetBitString(floatMantissa) << ")");
                    floatBits |= floatMantissa;

                    if (miniExponent == miniFloatExponentBitmask) // Infinity or NaN
                    {
                        // Set all float exponent bits
                        floatBits |= floatExponentMask;
                    }
                    else // Normalized
                    {
                        // Shift bits right, so it's easier to calculate things
                        const uchar shiftedMiniExponent = miniExponent >> 4;
                        log_v("MiniFloat exponent = " << (uint)shiftedMiniExponent << " (" << GetBitString(shiftedMiniExponent, true) << ")");

                        // Subtract bias (may be negative, so result must be signed)
                        const char unbiasedMiniExponent = shiftedMiniExponent - bias;
                        log_v("MiniFloat biased exponent = " << (int)unbiasedMiniExponent);

                        // Since regular float bias > mini float bias, result will always be > 0 so it can be unsigned
                        const uint floatExponent = unbiasedMiniExponent + floatBias;
                        log_v("float exponent = " << floatExponent << " (" << GetBitString(floatExponent, true) << ")");

                        // move exponent to correct place in regular float
                        floatBits |= floatExponent << 23;
                    }
                }

                // Copy bits into actual float
                float result;
                std::memcpy(&result, &floatBits, sizeof(float));
                log_v("reconstructed float = " << result << " (" << GetBitString(floatBits) << ")");
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
