# Oaknut

*A C++20 assembler for AArch64 (ARMv8.0 to ARMv8.2)*

Oaknut is a header-only library that allows one to dynamically assemble code in-memory at runtime.

## Usage

Provide `oaknut::CodeGenerator` with a pointer to a block of memory. Call functions on it to emit code.

Simple example:

```cpp
#include <cstdio>
#include <oaknut/oaknut.hpp>

using EmittedFunction = int (*)();

EmittedFunction EmitExample(oaknut::CodeGenerator& code, int value)
{
    using namespace oaknut::util;

    EmittedFunction result = code.ptr<EmittedFunction>();

    code.MOV(W0, value);
    code.RET();

    return result;
}

int main()
{
    oaknut::CodeBlock mem{4096};
    oaknut::CodeGenerator code{mem.ptr()};

    mem.unprotect();

    EmittedFunction fn = EmitExample(code, 42);

    mem.protect();
    mem.invalidate_all();

    std::printf("%i\n", fn());  // Output: 42

    return 0;
}
```

### Instructions

Each AArch64 instruction corresponds to one emitter function. For a list of emitter functions see:
* ARMv8.0: [general instructions](include/oaknut/impl/mnemonics_generic_v8.0.inc.hpp), [FP & SIMD instructions](include/oaknut/impl/mnemonics_fpsimd_v8.0.inc.hpp)
* ARMv8.1: [general instructions](include/oaknut/impl/mnemonics_generic_v8.1.inc.hpp), [FP & SIMD instructions](include/oaknut/impl/mnemonics_fpsimd_v8.1.inc.hpp)
* ARMv8.2: [general instructions](include/oaknut/impl/mnemonics_generic_v8.2.inc.hpp), [FP & SIMD instructions](include/oaknut/impl/mnemonics_fpsimd_v8.2.inc.hpp)

### Operands

The `oaknut::util` namespace provides convenient names for operands for instructions. For example:

|Name|Class|  |
|----|----|----|
|W0, W1, ..., W30|`WReg`|32-bit general purpose registers|
|X0, X1, ..., X30|`XReg`|64-bit general purpose registers|
|WZR|`WzrReg` (convertable to `WReg`)|32-bit zero register|
|XZR|`ZrReg` (convertable to `XReg`)|64-bit zero register|
|WSP|`WspReg` (convertable to `WRegSp`)|32-bit stack pointer|
|SP|`SpReg` (convertable to `XRegSp`)|64-bit stack pointer|
|B0, B1, ..., B31|`BReg`|8-bit scalar SIMD register|
|H0, H1, ..., H31|`HReg`|16-bit scalar SIMD register|
|S0, S1, ..., S31|`SReg`|32-bit scalar SIMD register|
|D0, D1, ..., D31|`DReg`|64-bit scalar SIMD register|
|Q0, Q1, ..., Q31|`QReg`|128-bit scalar SIMD register|

For vector operations, you can specify registers like so:

|Name|Class|  |
|----|----|----|
|V0.B8(), ...|`VReg_8B`|8 elements each 8 bits in size|
|V0.B16(), ...|`VReg_16B`|16 elements each 8 bits in size|
|V0.H4(), ...|`VReg_4H`|4 elements each 16 bits in size|
|V0.H8(), ...|`VReg_8H`|8 elements each 16 bits in size|
|V0.S2(), ...|`VReg_2S`|2 elements each 32 bits in size|
|V0.S4(), ...|`VReg_4S`|4 elements each 32 bits in size|
|V0.D1(), ...|`VReg_1D`|1 elements each 64 bits in size|
|V0.D2(), ...|`VReg_2D`|2 elements each 64 bits in size|

And you can specify elements like so:

|Name|Class|  |
|----|----|----|
|V0.B()[0]|`BElem`|0th 8-bit element of V0 register|
|V0.H()[0]|`HElem`|0th 16-bit element of V0 register|
|V0.S()[0]|`SElem`|0th 32-bit element of V0 register|
|V0.D()[0]|`DElem`|0th 64-bit element of V0 register|

Register lists are specified using `List`:

```
List{V0.B16(), V1.B16(), V2.B16()}  // This expression has type List<VReg_16B, 3>
```

And lists of elements similarly (both forms are equivalent):

```
List{V0.B()[1], V1.B()[1], V2.B()[1]}  // This expression has type List<BElem, 3>
List{V0.B(), V1.B(), V2.B()}[1]        // This expression has type List<BElem, 3>
```

You can find examples of instruction use in [tests/general.cpp](tests/general.cpp) and [tests/fpsimd.cpp](tests/fpsimd.cpp).

## License

This project is [MIT licensed](LICENSE).
