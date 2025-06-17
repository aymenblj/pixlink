#pragma once

#include "pipeline/strategy.hpp"
#include "pipeline/strategy_default.hpp"

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <functional>
#include <algorithm>
#include <stdexcept>

namespace pipeline {

namespace fs = std::filesystem;

// Helper to append suffix before file extension in a path string
inline std::string appendSuffix(const std::string& key, const std::string& suffix) {
    fs::path p(key);
    fs::path parent = p.parent_path();
    fs::path stem = p.stem();      // filename without extension
    fs::path ext = p.extension();
    // append the suffix to the original stem
    fs::path newName = stem.string() + suffix + ext.string();
    return (parent / newName).string();
}

template <typename ImageType>
class Pipeline {
public:
    using ImageMap = std::unordered_map<std::string, ImageType>;

    /**
     * @brief Construct a new Pipeline object.
     * 
     * @param inputFolder Folder path where input images are loaded from.
     * @param outputFolder Folder path where output images will be saved.
     * @param cache Optional custom cache manager. Defaults to DefaultCacheManager.
     * @param loader Optional custom image loader. Defaults to DefaultImageLoader.
     * @param saver Optional custom image saver. Defaults to DefaultImageSaver.
     */
    Pipeline(
        const std::string& inputFolder,
        const std::string& outputFolder,
        std::unique_ptr<CacheManager<ImageType>> cache = nullptr,
        std::unique_ptr<ImageLoader<ImageType>> loader = nullptr,
        std::unique_ptr<ImageSaver<ImageType>> saver = nullptr)
        : inputFolder(inputFolder)
        , outputFolder(outputFolder)
        , cacheManager(cache != nullptr ? std::move(cache) : std::make_unique<DefaultCacheManager<ImageType>>())
        , imageLoader(loader != nullptr ? std::move(loader) : std::make_unique<DefaultImageLoader<ImageType>>())
        , imageSaver(saver != nullptr ? std::move(saver) : std::make_unique<DefaultImageSaver<ImageType>>())
    {
        std::filesystem::create_directories(outputFolder);
    }

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    Pipeline(Pipeline&&) = default;
    Pipeline& operator=(Pipeline&&) = default;

    // --- Loading images ---

    /**
     * @brief Load an image from disk relative to the input folder.
     * If the image is cached, it will not be reloaded.
     * 
     * @param path Relative path to the image file from inputFolder.
     * @return Reference to *this for chaining.
     * @throws std::runtime_error if file does not exist.
     */
    Pipeline& load(const std::string& path) {
        std::filesystem::path fullPath = std::filesystem::path(inputFolder) / path;
        if (!std::filesystem::exists(fullPath))
            throw std::runtime_error("File does not exist: " + fullPath.string());
        std::string key = path;
        if (workingMap.count(key)) return *this;
        if (!cacheManager->isCached(key)) {
            imageLoader->loadIntoCache(*cacheManager, fullPath.string(), key);
        }
        workingMap[key] = cacheManager->getCached(key);
        return *this;
    }

    /**
     * @brief Load an image directly from memory, assigning it a given key.
     * The image is stored in cache and working set.
     * 
     * @param image Image object to load.
     * @param name Key/name to associate with the loaded image.
     * @return Reference to *this for chaining.
     */
    Pipeline& load(const ImageType& image, const std::string& name) {
        std::string key = name;
        imageLoader->loadIntoCache(*cacheManager, image, key);
        workingMap[key] = cacheManager->getCached(key);
        return *this;
    }

    /**
     * @brief Recursively load all images from a directory (relative to inputFolder),
     * optionally filtering by allowed file extensions.
     * 
     * @param directory Relative directory path to load images from.
     * @param extensions List of allowed file extensions (e.g., {".jpg", ".png"}). Defaults to {".jpg"}.
     * @return Reference to *this for chaining.
     */
    Pipeline& loadDirectory(const std::string& directory, const std::vector<std::string>& extensions = {".jpg"}) {
        std::filesystem::path base = std::filesystem::path(inputFolder) / directory;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(base)) {
            if (!entry.is_regular_file()) continue;
            std::string ext = entry.path().extension().string();
            if (!extensions.empty()) {
                std::string lower_ext = ext;
                std::transform(lower_ext.begin(), lower_ext.end(), lower_ext.begin(), ::tolower);
                bool found = false;
                for (const auto& allowed : extensions) {
                    std::string lower_allowed = allowed;
                    std::transform(lower_allowed.begin(), lower_allowed.end(), lower_allowed.begin(), ::tolower);
                    if (lower_ext == lower_allowed) { found = true; break; }
                }
                if (!found) continue;
            }
            std::string key = std::filesystem::relative(entry.path(), inputFolder).generic_string();
            if (!cacheManager->isCached(key)) {
                imageLoader->loadIntoCache(*cacheManager, entry.path().string(), key);
            }
            workingMap[key] = cacheManager->getCached(key);
        }
        return *this;
    }

    // --- Query / Access ---

    /**
     * @brief Get all keys of loaded images currently in working set.
     * Optionally filter keys by a relative directory prefix.
     * 
     * @param relativeDir Optional directory prefix filter (e.g., "subdir/").
     * @return Vector of image keys currently loaded.
     */
    std::vector<std::string> getAllImageKeys(const std::string& relativeDir = "") const {
        std::vector<std::string> keys;
        std::string prefix = relativeDir;
        if (!prefix.empty() && prefix.back() != '/') prefix += '/';
        for (const auto& [key, _] : workingMap)
            if (prefix.empty() || key.rfind(prefix, 0) == 0)
                keys.push_back(key);
        return keys;
    }

    /**
     * @brief Check if the working set of loaded images is empty.
     * 
     * @return true if no images are loaded in working set.
     * @return false otherwise.
     */
    bool isWorkingMapEmpty() const { return workingMap.empty(); }

    /**
     * @brief Check if the cache is empty (no images cached).
     * 
     * @return true if cache is empty.
     * @return false otherwise.
     */
    bool isCacheMapEmpty() const { return cacheManager->getKeys().empty(); }

    /**
     * @brief Get the path to the output folder where images are saved.
     * 
     * @return const reference to output folder path string.
     */
    const std::string& getOutputFolder() const { return outputFolder; }

    // --- Processing ---

    /**
     * @brief Apply a processing operation on a loaded image.
     * The operation is a function that takes a const reference to an image and returns a processed image.
     * 
     * @param key Key of the image to process.
     * @param op Operation function to apply on the image.
     * @return Reference to *this for chaining.
     * @throws std::runtime_error if image key not found in working set.
     */
    Pipeline& process(const std::string& key, std::function<ImageType(const ImageType&)> op) {
        auto it = assertInWorkingMap(key);
        it->second = op(it->second);
        return *this;
    }

    Pipeline& filter(std::function<bool(const std::string&, const ImageType&)> pred) {
    for (auto it = workingMap.begin(); it != workingMap.end(); ) {
        if (!pred(it->first, it->second))
            it = workingMap.erase(it);
        else
            ++it;
    }
    return *this;
    }

    // --- Saving images ---

    /**
     * @brief Save a loaded image to the output folder using its original key-based filename.
     * If the key has no extension, ".jpg" is appended.
     * 
     * @param key Key of the image to save.
     * @return Reference to *this for chaining.
     * @throws std::runtime_error if image key not found in working set.
     */
    Pipeline& save(const std::string& key) {
        auto it = assertInWorkingMap(key);
        std::filesystem::path p(key);
        std::string filename = p.extension().empty() ? (key + ".jpg") : key;
        std::filesystem::path outPath = std::filesystem::path(outputFolder) / filename;
        imageSaver->save(outPath.string(), it->second);
        return *this;
    }

    /**
     * @brief Save a loaded image to a custom output path relative to the output folder.
     * 
     * @param key Key of the image to save.
     * @param outputPath Relative output path (including filename).
     * @return Reference to *this for chaining.
     * @throws std::runtime_error if image key not found in working set.
     */
    Pipeline& save(const std::string& key, const std::string& outputPath) {
        auto it = assertInWorkingMap(key);
        std::filesystem::path fullPath = std::filesystem::path(outputFolder) / outputPath;
        imageSaver->save(fullPath.string(), it->second);
        return *this;
    }

    /**
     * @brief Save an image into a custom subdirectory inside the output folder, preserving original filename.
     * 
     * @param key Key of the image to save.
     * @param customSubdir Subdirectory inside output folder.
     * @return Reference to *this for chaining.
     * @throws std::runtime_error if image key not found in working set.
     */
    Pipeline& saveAs(const std::string& key, const std::string& customSubdir) {
        auto it = assertInWorkingMap(key);
        imageSaver->saveAs(it->second, outputFolder, customSubdir, key);
        return *this;
    }

    /**
     * @brief Save and rename an image into a custom subdirectory, appending a suffix to filename.
     * 
     * @param key Key of the image to save.
     * @param customSubdir Subdirectory inside output folder.
     * @param newFilename Suffix to append to filename before extension.
     * @return Reference to *this for chaining.
     * @throws std::runtime_error if image key not found in working set.
     */
    Pipeline& saveAs(const std::string& key, const std::string& customSubdir, const std::string& newFilename) {
        auto it = assertInWorkingMap(key);
        std::string saveKey = pipeline::appendSuffix(key, newFilename);
        imageSaver->saveAs(it->second, outputFolder, customSubdir, saveKey);
        return *this;
    }

    /**
     * @brief Save all loaded images currently in the working set to the output folder.
     * 
     * @return Reference to *this for chaining.
     */
    Pipeline& saveAll() {
        imageSaver->saveAll(workingMap, outputFolder);
        return *this;
    }

    // --- Unloading / Removing images ---

    /**
     * @brief Remove an image from both the working set and the cache.
     * 
     * @param key Key of the image to unload.
     * @return Reference to *this for chaining.
     * @throws std::runtime_error if image key not found in working set.
     */
    Pipeline& unload(const std::string& key) {
        auto it = assertInWorkingMap(key);
        cacheManager->remove(key);
        workingMap.erase(it);
        return *this;
    }

    /**
     * @brief Remove all images from working set and clear the entire cache.
     * 
     * @return Reference to *this for chaining.
     */
    Pipeline& unloadAll() {
        workingMap.clear();
        cacheManager->clear();
        return *this;
    }

    /**
     * @brief Remove an image from the working set but keep it cached.
     * 
     * @param key Key of the image to release.
     * @return Reference to *this for chaining.
     * @throws std::runtime_error if image key not found in working set.
     */
    Pipeline& release(const std::string& key) {
        auto it = assertInWorkingMap(key);
        workingMap.erase(it);
        return *this;
    }

    /**
     * @brief Reload an image by releasing it from working set and loading it again from cache or disk.
     * 
     * @param key Key of the image to reset.
     * @return Reference to *this for chaining.
     */
    Pipeline& reset(const std::string& key) {
        return this->release(key).load(key);
    }

    // --- Cache management ---

    /**
     * @brief Clear the entire cache manually.
     * 
     * @return Reference to *this for chaining.
     */
    Pipeline& clearCache() {
        cacheManager->clear();
        return *this;
    }

    // get workingMap
    ImageMap& getWorkingMap() { return workingMap; }

private:
    std::string inputFolder;
    std::string outputFolder;
    ImageMap workingMap;
    std::unique_ptr<CacheManager<ImageType>> cacheManager;
    std::unique_ptr<ImageLoader<ImageType>> imageLoader;
    std::unique_ptr<ImageSaver<ImageType>> imageSaver;

    // Check image exists in working map, throw if not found
    typename ImageMap::iterator assertInWorkingMap(const std::string& key) {
        auto it = workingMap.find(key);
        if (it == workingMap.end())
            throw std::runtime_error("Image not found in working set: " + key);
        return it;
    }

    typename ImageMap::const_iterator assertInWorkingMap(const std::string& key) const {
        auto it = workingMap.find(key);
        if (it == workingMap.end())
            throw std::runtime_error("Image not found in working set: " + key);
        return it;
    }

    // Create a relative key from absolute path (not currently used in public API)
    static inline std::string makeKey(const std::string& path, const std::string& inputFolder) {
        return fs::relative(fs::absolute(path), fs::absolute(inputFolder)).generic_string();
    }
};

} // namespace pipeline
