This was a study in floating point number representation.

I decided to go for the simplest 8 bit *"mini float"* because it was the easiest to work through manually and I could only find implementations for half float (16 bit) online so it would be harder for me to cheat and just copy what I couldn't understand.

---------------------------------------
# Research Material

This is just a list of the more outstanding materials I found only. It's by no means exhaustive, just some things that stuck in my head and could be a nice starting point for someone else wanting to learn how floats work.

## 16 bit float implementations

### Unreal Engine FFloat16:
https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Source/Runtime/Core/Public/Math/Float16.h

### Fabian "ryg" Giesen
https://gist.github.com/rygorous/2156668

## Videos

### Binary Representation of Floating-Point Values - Mini-float 8-bit Floats(Kris Jordan channel)
https://www.youtube.com/watch?v=D-9SQMWo6kI

### IEEE 754 Standard for Floating Point Binary Arithmetic (Computer Science channel)
https://www.youtube.com/watch?v=RuKkePyo9zk

### Floating Point Numbers (with Tom Scott at Computerphile)
https://www.youtube.com/watch?v=PZRI1IfStY0

## Useful Tools:
### Bit shift calculator
https://bit-calculator.com/bit-shift-calculator

When converting between floats there are quite a couple bit-shifts going on. It was nice being able to plop a value in this tool and check what I'd get after shifting it left or right.

### Float Exposed
https://float.exposed/0x387fc000

This allows you to see the binary representation of different floats and see how one convert into the other. It's an excellent tool to help get a feel for how things work.

---------------------------------------

# Examples

## Float to Half (0.000030517578125)

```
S|    E    |          M
0 0111000 0 0000000 00000000 00000000 (float)
                  0 00000 10 00000000 (half)
                  S|  E  |     M

S = sign bit
E = exponent bits
M = mantissa bits

float bias (f_bias): 127
float exponent (f_exp): 112
unbiased f_exp (u_f_exp): f_exp - f_bias => 112 - 127 = -15
float mantissa (f_mant): last 23 bits from float (0 in this case)

half float bias (hf_bias): 15
half float exponent (hf_exp): u_f_exp + hf_bias => -15 + 15 = 0
```

Since `hf_exp <= 0`, this is an **underflow**, but before assuming the value is too small for a half, we must check if it's possible to represent it through the mantissa only:

```
14 - hf_exp <= 24
```

This will **always** result in something >= 14 because `hf_exp <= 0`.

The number 14 comes from the fact we want to move the implied 1 from the float mantissa (which is at the 24th position) into the first mantissa bit from the half float (10th position) when `hf_exp = 0`.

```
Hidden 1 bit for mantissa on float (would occupy first exponent bit)
0x800000 = 10000000 00000000 00000000
```

By subtracting `hf_exp` from 14, we find which half float mantissa bit should be set in order to represent the provided float value, starting from the 10th bit and going further to the right until out of range. If the condition is false, it follows that `hf_exp < -10` which represents too small a value for the half precision float.

Since we already know the value is within range, we can calculate the half float mantissa.

The first step is to make the implicit mantissa 1 **explicit**:

```
explicit float mantissa (e_f_mant): f_mant | 0x800000 => 0 | 0x800000 = 0x800000
```

Then, find out by how much the mantissa must be shifted right (the value we checked above):

```
mantissa_shift = 14 - hf_exp => 14 - 0 = 14
```

And use it to obtain the expected half float mantissa:

```
half float mantissa (hf_mant): e_f_mant >> mantissa_shift => 0x800000 >> 14 = 10 0000 0000
```

In this example, the float mantissa was 0 so the only thing shifted was the implied 1, which ended up as the first mantissa bit (the 10th bit).

The final step is checking for rounding, which is the same as before, but with a shift of 13 instead of 14 (account for a possible 1 that was shifted out of range):

```
rounding_check: e_f_mant >> (mantissa_shift - 1) => 0x800000 >> (14 - 1) => 0x800000 >> 13 = 100 0000 0000
rounding_check & 1 => 100 0000 0000 & 1 = 0
```

In this case, no rounding was required, but if it was, it would be only a matter of adding 1 to the raw bits and letting it overflow into the exponent bit if necessary

**RESULT**
```
sign = 0
exp = 0
mant = 10 0000 0000
```

======================
## Float to Mini Float (0.0625)

```
S|    E    |          M
0 0111101 1 0000000 00000000 00000000
                           0 000 0100
                           S| E | M

float bias (f_bias): 127
float exponent (f_exp): 123
unbiased f_exp (u_f_exp): f_exp - f_bias => 123 - 127 = -4
float mantissa (f_mant): last 23 bits from float (0 in this case)

mini float bias (mf_bias): 3
mini float exponent (mf_exp): u_f_exp + mf_bias => -4 + 3 = -1
```

Like the previous example, this is an underflow (`mf_exp <= 0`).

To move the hidden float 1 bit to the correct mini float position (4th), it must be right shifted 20

```
mantissa_shift: 20 - mf_exp => 20 - (-1) = 21 which is <= 24
```

The float mantissa with the explicit one is the same:

```
explicit float mantissa (e_f_mant): float mantissa | 0x800000 => 0 | 0x800000 = 0x800000
```

As are all next steps
 1000 0000 0000 0000 0000 0000
```
mini float mantissa (mf_mant): e_f_mant >> mantissa_shift => 0x800000 >> 21 = 0100

rounding_check: e_mf_mant >> (mantissa_shift - 1) => 0x800000 >> (21 - 1) => 0x800000 >> 20 = 1000

rounding_check & 1 => 1000 & 1 = 0
```

Again, no rounding was required.

**RESULT**
```
sign = 0
exp = 0
mant = 0100
```

_obs: When exponent bits are 0, we use (1 - bias) rather than -bias, so in this case we get 2^(1 - 3) = 2^(-2), which, when multiplied by the mantissa 2^(-2) yields 2^(-4) = 0.0625_


