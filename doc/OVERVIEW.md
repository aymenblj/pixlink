# Pipeline Class Overview

A **fluent, extensible image processing pipeline** in modern C++.

---

## Key Concepts

- **Working Set**: Images currently being processed (in memory).
- **Cache**: Fast reload buffer (unlimited or LRU); avoids redundant disk reads.

---

## Main Features

- **Header-only**: Just include and use.
- **Chainable methods**: Enables expressive, fluent code for multi-step image processing.
- **Flexible loading**: Load images from directories, files, or memory.
- **Customizable cache**: Plug in your own caching strategy (unlimited, LRU, etc).
- **Directory awareness**: Preserves and manages relative paths for organized batch processing.
- **Automatic output folder management**: Output directories created as needed.
- **Convenient memory management**:  
  - `release(key)`: Remove from working set (still cached)
  - `unload(key)`: Remove from both working set and cache
  - `clearCache()`: Remove all cached images
  - `reset(key)`: Reload image from cache
- **Flexible save**:  
  - `save(key)`: Save using key as filename  
  - `saveAs(key, subdir)`: Save to a custom subdirectory  
  - `saveAs(key, subdir, suffix)`: Save with a new filename (suffix appended before extension)

---

## Example Usage

```cpp
pipeline.loadDirectory("animals", {".jpg", ".png"});

for (const auto& key : pipeline.getAllImageKeys("animals")) {
    pipeline.process(key, gaussianBlur)
            .saveAs(key, "animals/gaussian")
            .release(key)
            .process(key, laplacianEdge)
            .saveAs(key, "animals/laplacian", "_lap")
            .unload(key);
}
```
- **Batch loads images** from `animals/` (recursive).
- **Processes and saves** with two different filters, in two output folders.
- **Efficiently manages memory** during batch operations.

---

## Typical Method Calls

- `load(path)` / `load(image, key)`: Load an image from disk or memory.
- `loadDirectory(directory, extensions)`: Recursively load all images with given extensions.
- `process(key, op)`: Apply a transformation to an image.
- `save(key)` / `saveAs(key, subdir, suffix)`: Save an image.
- `release(key)`: Remove from working set, keep in cache.
- `unload(key)`: Remove from both working set and cache.
- `clearCache()`: Remove all cached images.
- `getAllImageKeys(dir)`: Get all loaded images for a directory.
- `reset(key)`: Release and reload image.
- `isWorkingMapEmpty()`, `isCacheMapEmpty()`: Check state.

---

## Why Use Pipeline?

- **Performance**: Caching and memory management for large image sets.
- **Simplicity**: Chainable, readable code.
- **Flexibility**: Easily integrate custom loaders, savers, and cache logic.
- **Header-only**: No linking, just include.

---