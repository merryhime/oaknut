# Oaknut

*A C++20 assembler for AArch64 (ARMv8.0)*

Oaknut is a header-only library that allows one to dynamically assemble code in-memory at runtime.

## Usage

Simple example:

```cpp
using EmittedFunction = int (*)();

EmittedFunction EmitExample(oaknut::CodeGenerator& code, int value)
{
    using namespace oaknut::util;

    EmittedFunction result = code.ptr<EmittedFunction>();

    code.MOVZ(W0, value);
    code.RET();

    return result;
}
```

## License

This project is [MIT licensed](LICENSE).
