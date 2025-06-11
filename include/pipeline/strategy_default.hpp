#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <memory>
#include <stdexcept>

namespace pipeline {

/**
 * @brief A simple cache manager that stores images in an unordered_map.
 * 
 * This cache manager has no size limit and stores copies of images.
 * 
 * @tparam ImageType The image type to cache.
 */
template <typename ImageType>
class DefaultCacheManager : public CacheManager<ImageType> {
public:
    using ImageMap = std::unordered_map<std::string, ImageType>;

    /**
     * @brief Store an image in the cache with a specified key.
     * @param key Unique string key to associate with the image.
     * @param image The image to cache.
     */
    void cacheImage(const std::string& key, const ImageType& image) override { map[key] = image; }

    /**
     * @brief Check if an image with the given key is cached.
     * @param key Key to check in the cache.
     * @return true if cached, false otherwise.
     */
    bool isCached(const std::string& key) const override { return map.count(key) > 0; }

    /**
     * @brief Get a cached image by key (shallow copy).
     * @param key Key of the cached image.
     * @return The cached image.
     * @throws std::runtime_error if key is not found.
     */
    ImageType getCachedShallow(const std::string& key) const override {
        auto it = map.find(key);
        if (it == map.end()) throw std::runtime_error("Cache miss: " + key);
        return it->second;
    }

    /**
     * @brief Get a cached image by key (deep copy).
     * @param key Key of the cached image.
     * @return The cached image.
     * @throws std::runtime_error if key is not found.
     */
    ImageType getCached(const std::string& key) const override {
        auto it = map.find(key);
        if (it == map.end()) throw std::runtime_error("Cache miss: " + key);
        return it->second;
    }

    /**
     * @brief Remove an image from the cache.
     * @param key Key of the image to remove.
     */
    void remove(const std::string& key) override { map.erase(key); }

    /**
     * @brief Clear the entire cache.
     */
    void clear() override { map.clear(); }

    /**
     * @brief Get all keys currently in the cache.
     * @return Vector of string keys.
     */
    std::vector<std::string> getKeys() const override {
        std::vector<std::string> keys;
        for (const auto& [k, _] : map) keys.push_back(k);
        return keys;
    }

private:
    ImageMap map; ///< Internal map storing cached images.
};

/**
 * @brief Default image loader that loads images from files and directories.
 * 
 * User must specialize the loadFromFile method for their ImageType.
 * 
 * @tparam ImageType The image type to load.
 */
template <typename ImageType>
class DefaultImageLoader : public ImageLoader<ImageType> {
public:
    /**
     * @brief Load an image from a file.
     * 
     * Must be specialized for the ImageType.
     * 
     * @param path Path to the image file.
     * @return Loaded image.
     */
    ImageType loadFromFile(const std::string& path) override;

    /**
     * @brief Load an image from a file and cache it.
     * @param cache Cache manager to store the loaded image.
     * @param path Path to the image file.
     * @param key Key to store the image under in the cache.
     */
    void loadIntoCache(CacheManager<ImageType>& cache, const std::string& path, const std::string& key) override {
        cache.cacheImage(key, loadFromFile(path));
    }

    /**
     * @brief Cache an existing image under a specified key.
     * @param cache Cache manager to store the image.
     * @param image The image to cache.
     * @param key Key to store the image under.
     */
    void loadIntoCache(CacheManager<ImageType>& cache, const ImageType& image, const std::string& key) override {
        cache.cacheImage(key, image);
    }

    /**
     * @brief Recursively load all images from a directory into cache.
     * 
     * Only files with extensions in `extensions` are loaded.
     * Keys are relative to inputRoot.
     * 
     * @param cache Cache manager to store images.
     * @param dir Directory path to scan.
     * @param inputRoot Root directory to calculate relative keys.
     * @param extensions Allowed file extensions (e.g. ".png", ".jpg").
     * @return Vector of keys for the loaded images.
     */
    std::vector<std::string> loadDirectory(CacheManager<ImageType>& cache, const std::string& dir, const std::string& inputRoot, const std::vector<std::string>& extensions) override {
        namespace fs = std::filesystem;
        std::vector<std::string> keys;
        fs::path inputRootPath = fs::absolute(inputRoot);
        for (const auto& entry : fs::recursive_directory_iterator(dir)) {
            if (!entry.is_regular_file()) continue;
            fs::path path = entry.path();
            std::string ext = path.extension().string();
            if (!isSupported(ext, extensions)) continue;
            std::string key = fs::relative(fs::absolute(path), inputRootPath).generic_string();
            try {
                loadIntoCache(cache, path.string(), key);
                keys.push_back(key);
            } catch (...) {}
        }
        return keys;
    }

    /**
     * @brief Check if a file extension is supported.
     * @param ext File extension (case-insensitive).
     * @param allowed List of allowed extensions.
     * @return true if supported, false otherwise.
     */
    static bool isSupported(const std::string& ext, const std::vector<std::string>& allowed) {
        std::string lower = ext;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return std::find(allowed.begin(), allowed.end(), lower) != allowed.end();
    }
};

/**
 * @brief Default image saver that saves images to disk.
 * 
 * User must specialize the save method for their ImageType.
 * 
 * @tparam ImageType The image type to save.
 */
template <typename ImageType>
class DefaultImageSaver : public ImageSaver<ImageType> {
public:
    using ImageMap = std::unordered_map<std::string, ImageType>;

    /**
     * @brief Save a single image to a file.
     * 
     * Must be specialized for the ImageType.
     * 
     * @param outputPath Output file path.
     * @param image Image to save.
     */
    void save(const std::string& outputPath, const ImageType& image) override;

    /**
     * @brief Save multiple images into an output directory.
     * 
     * If image keys do not have extensions, ".jpg" is appended.
     * 
     * @param images Map of image keys to images.
     * @param outputDir Directory to save images into.
     */
    void saveAll(const ImageMap& images, const std::string& outputDir) override {
        namespace fs = std::filesystem;
        for (const auto& [key, img] : images) {
            fs::path p(key);
            std::string filename = p.extension().empty() ? (key + ".jpg") : key;
            fs::path outPath = fs::path(outputDir) / filename;
            save(outPath.string(), img);
        }
    }

    /**
     * @brief Save an image into a custom subdirectory inside the output directory.
     * 
     * If key does not have an extension, ".jpg" is appended.
     * 
     * @param image Image to save.
     * @param outputDir Root output directory.
     * @param customSubdir Custom subdirectory name.
     * @param key Key representing the image filename.
     */
    void saveAs(const ImageType& image, const std::string& outputDir, const std::string& customSubdir, const std::string& key) override {
        namespace fs = std::filesystem;
        fs::path keyPath(key);
        std::string filename = keyPath.filename().string();
        if (keyPath.extension().empty()) filename += ".jpg";
        fs::path outPath = fs::path(outputDir) / customSubdir / filename;
        save(outPath.string(), image);
    }
};

} // namespace pipeline
