# pixlink

**Fluent, header-only image processing pipeline for modern C++ & OpenCV (optional)**

---

## Overview

`pixlink` is a modern C++20 library for expressive, chainable, and generic image processing.  
It streamlines batch workflows with efficient memory management and pluggable caching.

---

## Features

- Header-only: just include and use
- Fluent, chainable API for image operations
- Load images from disk or memory, batch or single
- Directory structure awareness in outputs
- Memory utilities: `release`, `unload`, `clearCache`
- Tool-agnostic: Use any image processing library you prefer
- Pluggable cache: Swap in unlimited, LRU, or custom strategies
- Shallow copies: Images reference cached data to avoid heavy I/O
- Extensible: custom cache, loader, saver
- Planned: External/distributed cache support (TODO)
---

## Requirements

- C++20
- OpenCV 4.x+
- CMake 3.16+

---

## Build

## Build

Make sure to set `OpenCV_DIR` to your OpenCV build directory containing `OpenCVConfig.cmake`.  
Example on Windows:

```bash
cmake -S . -B build -DOpenCV_DIR="C:/opencv/build/x64/vc16/lib"
cmake --build build
cd build
```

## Run

```bash
pixlink
```

---

## Example

```cpp
for (const auto& key : pipeline.getAllImageKeys("animals")) {
    pipeline.process(key, gaussianBlur)
            .saveAs(key, "animals/gaussian")
            .reset(key)
            .process(key, medianBlur)
            .saveAs(key, "animals/median")
            .reset(key)
            .process(key, bilateralFilter)
            .saveAs(key, "animals/bilateral")
            .reset(key)
            .process(key, laplacianEdge)
            .process(key, medianBlur)
            .saveAs(key, "animals/median_laplacian", "_med_lap");
}

```

---

## Structure

```
project-root/
├── CMakeLists.txt
├── include/
│   └── pipeline/
│       ├── pipeline.hpp
│       ├── strategy.hpp
│       ├── strategy_default.hpp
│       └── strategy_lru.hpp
├── src/
│   └── main.cpp
```

---

## License

This project is licensed under the [MIT License](LICENSE).