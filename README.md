# Verdigris Embedded Systems Take-home

Please read `INSTRUCTIONS.md`.

Quick start:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```
