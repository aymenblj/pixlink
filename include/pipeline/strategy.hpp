#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace pipeline {

/**
 * @brief Type alias for a map storing images keyed by string identifiers.
 *
 * @tparam ImageType Type representing an image (e.g., cv::Mat).
 */
template <typename ImageType>
using ImageMap = std::unordered_map<std::string, ImageType>;

/**
 * @brief Abstract interface for managing image caching.
 *
 * @tparam ImageType Type representing an image.
 */
template <typename ImageType>
class CacheManager {
public:
    virtual ~CacheManager() = default;

    /**
     * @brief Cache the given image under the specified key.
     *
     * @param key Unique string identifier for the image.
     * @param image The image data to cache.
     */
    virtual void cacheImage(const std::string& key, const ImageType& image) = 0;

    /**
     * @brief Check if the cache contains an image with the given key.
     *
     * @param key Key to check in the cache.
     * @return true if image is cached, false otherwise.
     */
    virtual bool isCached(const std::string& key) const = 0;

    /**
     * @brief Retrieve a cached image by key. Returns a deep copy.
     *
     * @param key Key of the cached image.
     * @return ImageType A copy of the cached image.
     *
     * @throws std::runtime_error if the key does not exist.
     */
    virtual ImageType getCached(const std::string& key) const = 0;

    /**
     * @brief Retrieve a cached image by key. Returns a shallow copy or reference.
     *
     * @param key Key of the cached image.
     * @return ImageType A shallow copy or reference to the cached image.
     *
     * @throws std::runtime_error if the key does not exist.
     */
    virtual ImageType getCachedShallow(const std::string& key) const = 0;

    /**
     * @brief Remove the cached image identified by the key.
     *
     * @param key Key of the image to remove.
     */
    virtual void remove(const std::string& key) = 0;

    /**
     * @brief Clear the entire cache.
     */
    virtual void clear() = 0;

    /**
     * @brief Get a list of all keys currently cached.
     *
     * @return std::vector<std::string> Vector containing all cached keys.
     */
    virtual std::vector<std::string> getKeys() const = 0;
};

/**
 * @brief Abstract interface for loading images from disk or memory.
 *
 * @tparam ImageType Type representing an image.
 */
template <typename ImageType>
class ImageLoader {
public:
    virtual ~ImageLoader() = default;

    /**
     * @brief Load an image from a file path.
     *
     * @param path Filesystem path to the image file.
     * @return ImageType Loaded image data.
     *
     * @throws std::runtime_error on load failure.
     */
    virtual ImageType loadFromFile(const std::string& path) = 0;

    /**
     * @brief Load an image from a file and store it in the cache with a key.
     *
     * @param cache Reference to the cache manager.
     * @param path Filesystem path to the image file.
     * @param key Key under which to store the loaded image.
     */
    virtual void loadIntoCache(CacheManager<ImageType>& cache, const std::string& path, const std::string& key) = 0;

    /**
     * @brief Load an image from memory and store it in the cache with a key.
     *
     * @param cache Reference to the cache manager.
     * @param image Image data in memory.
     * @param key Key under which to store the image.
     */
    virtual void loadIntoCache(CacheManager<ImageType>& cache, const ImageType& image, const std::string& key) = 0;

    /**
     * @brief Load all images from a directory recursively into the cache.
     *
     * @param cache Reference to the cache manager.
     * @param dir Directory path to load images from.
     * @param inputRoot Base input directory to create relative keys.
     * @param extensions Vector of allowed file extensions (e.g. {".jpg"}).
     * @return std::vector<std::string> List of keys of loaded images.
     */
    virtual std::vector<std::string> loadDirectory(CacheManager<ImageType>& cache, const std::string& dir, const std::string& inputRoot, const std::vector<std::string>& extensions) = 0;
};

/**
 * @brief Abstract interface for saving images to disk.
 *
 * @tparam ImageType Type representing an image.
 */
template <typename ImageType>
class ImageSaver {
public:
    virtual ~ImageSaver() = default;

    /**
     * @brief Save a single image to the specified output path.
     *
     * @param outputPath Full filesystem path to save the image.
     * @param image Image data to save.
     *
     * @throws std::runtime_error on save failure.
     */
    virtual void save(const std::string& outputPath, const ImageType& image) = 0;

    /**
     * @brief Save all images from the provided map to the output directory.
     *
     * @param images Map of key to image data.
     * @param outputDir Directory path where images should be saved.
     */
    virtual void saveAll(const ImageMap<ImageType>& images, const std::string& outputDir) = 0;

    /**
     * @brief Save an image into a custom subdirectory under output directory using the key as filename.
     *
     * @param image Image data to save.
     * @param outputDir Base output directory.
     * @param customSubdir Subdirectory under outputDir where to save.
     * @param key Original key/filename for the image.
     */
    virtual void saveAs(const ImageType& image, const std::string& outputDir, const std::string& customSubdir, const std::string& key) = 0;
};

} // namespace pipeline
