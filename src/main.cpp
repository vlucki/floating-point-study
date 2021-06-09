//#define LOG_LEVEL 2
#include "float.h"
#include "logger.h"
#include <cstring>
#include <stdexcept>
#include <string>
#include <cmath>

void TestWithBits(const unsigned char bitPattern, const float expectedResult)
{
    log_v("TestWithBits---------------");
    log_v_silent("");
    log_i("Creating MiniFloat from " << GetBitString(bitPattern));
    Math::MiniFloat test (bitPattern);

    log_v_silent("");
    const float result = test.ToFloat();

    if( result == expectedResult)
    {
        log_i("Result " << result << " matches expected value " << expectedResult << "\n");
    }
    else
    {
        log_e("Expected " << expectedResult << " but got " << result << "\n");
    }
}

void TestWithFloat(const float value)
{
    log_v_silent("TestWithFloat---------------\n");

    Math::MiniFloat test (value);

    log_v_silent("");
    const float result = test.ToFloat();

    if( result == value)
    {
        log_i("Result " << result << " matches expected value " << value << "\n");
    }
    else
    {
        log_e("Expected " << value << " but got " << result << "\n");
    }

}

void TestMixed(const unsigned char bitPattern, const float fValue)
{
    Math::MiniFloat bTest (bitPattern);
    Math::MiniFloat fTest (fValue);

    const auto printBasicResults = [bitPattern, fValue](const std::string& testType, const Math::MiniFloat& result)
    {
        log_v_silent("");
        const float fResult = result.ToFloat();
        unsigned char bits = result.GetBits();
        log_v_silent("");
        if (fResult == fValue)
        {
            log_i(testType << " based test returned " << fResult << " which matches provided float\n");
        }
        else
        {
            log_e(testType << " based test returned " << fResult << " which differs from provided float " << fValue << "\n");
        }

        if (bits == bitPattern)
        {
            log_i(testType << " based test resulted in bits " << GetBitString(bits) << " which matches provided pattern \n");
        }
        else
        {
            log_e(testType << " based test resulted in bits " << GetBitString(bits) << " which differs from provided pattern " << GetBitString(bitPattern) << "\n");
        }
    };

    printBasicResults("float", fTest);
    printBasicResults("bit pattern", bTest);
    return;

    const float bResult = bTest.ToFloat();
    const float fResult = fTest.ToFloat();
    unsigned char bBits = bTest.GetBits();
    unsigned char fBits = fTest.GetBits();

    if (bResult == fResult)
    {
        log_i("bit pattern constructed MiniFloat matches one based on float");
    }
    else
    {
        log_e("bit pattern constructed MiniFloat (" << GetBitString(bBits) << ") differs from one based on float (" << GetBitString(fBits) << ")");
    }

}
int main(int argc, char** argv)
{

    std::string choice;
    while(true)
    {
        print("Chose the testing method:\
                \n- Type a number to test;\
                \n- Type \"float\" to run pre-built tests converting floats to MiniFloat;\
                \n- Type \"bits\" to run pre-built tests converting bit sequences to MiniFloat;\
                \n- Type \"mixed\" to run pre-built tests converting bit sequences and floats to MiniFloat, then comparing them with eachother;\
                \n- Type \"exit\" to quit application.");
        std::cin >> choice;
        if(choice == "exit")
        {
            break;
        }
        if(choice == "float")
        {
            // -0.0625 to -8, then 0.0625 to 8
            for(int i = 0; i < 2; ++i)
            {
                for (int j = 0; j < 2; ++j)
                {
                    for (int k = -3; k <= 3; ++k)
                    {
                        float expected = std::pow(2, k);
                        if (j == 1)
                        {
                            if (k >= 0)
                            {
                                break;
                            }
                            expected += 1;
                        }
                        if (i == 0)
                        {
                            expected *= -1;
                        }
                        TestWithFloat(expected);
                    }
                }
            }
        }
        else if (choice == "bits")
        {
            TestWithBits(0b00111000, 1.5f);
            /*
            TestWithBits(0b00000000, 0.0f);
            TestWithBits(0b00100000, 0.5f);
            TestWithBits(0b00110000, 1.0f);
            TestWithBits(0b00110100, 1.25f);
            TestWithBits(0b01100000, 8.f);
            */
        }
        else if (choice == "mixed")
        {
            TestMixed(0b01100000, 8.f);
        }
        else
        {
            try
            {
                const float value = std::stof(choice);
                TestWithFloat(value);
            }
            catch (std::invalid_argument ia)
            {
                print("\"" << choice << "\" is not a valid float number");
            }
        }
    }
    return 0;
}
