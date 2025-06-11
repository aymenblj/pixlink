#pragma once

#include "pipeline/strategy.hpp"
#include <list>
#include <unordered_map>
#include <string>
#include <vector>
#include <stdexcept>

namespace pipeline {

/**
 * @brief LRU (Least Recently Used) cache implementation for images.
 * 
 * This cache stores a fixed number of images identified by string keys.
 * When the capacity is exceeded, the least recently used image is evicted.
 * 
 * @tparam ImageType The image data type stored in the cache.
 */
template <typename ImageType>
class LRUCacheManager : public CacheManager<ImageType> {
public:
    /**
     * @brief Construct a new LRUCacheManager with given capacity.
     * 
     * @param capacity Maximum number of images the cache can hold.
     */
    explicit LRUCacheManager(size_t capacity) : capacity_(capacity) {}

    /**
     * @brief Cache an image with the associated key.
     * 
     * If the key already exists, the image is updated and marked as recently used.
     * If the cache is full, the least recently used image is evicted.
     * 
     * @param key The unique string key identifying the image.
     * @param image The image data to cache.
     */
    void cacheImage(const std::string& key, const ImageType& image) override {
        auto it = map_.find(key);
        if (it != map_.end()) {
            it->second.first = image;
            usage_.splice(usage_.begin(), usage_, it->second.second);
        } else {
            if (map_.size() == capacity_) {
                const std::string& lruKey = usage_.back();
                map_.erase(lruKey);
                usage_.pop_back();
            }
            usage_.push_front(key);
            map_[key] = {image, usage_.begin()};
        }
    }

    /**
     * @brief Check if an image identified by key is cached.
     * 
     * @param key The key to query.
     * @return true if the image is cached, false otherwise.
     */
    bool isCached(const std::string& key) const override {
        return map_.find(key) != map_.end();
    }

    /**
     * @brief Retrieve a cached image by key.
     * 
     * Marks the image as recently used.
     * 
     * @param key The key identifying the cached image.
     * @return ImageType The cached image.
     * @throws std::runtime_error if the key is not found.
     */
    ImageType getCached(const std::string& key) const override {
        auto it = map_.find(key);
        if (it == map_.end()) throw std::runtime_error("Key not found in cache: " + key);
        usage_.splice(usage_.begin(), usage_, it->second.second);
        return it->second.first;
    }

    /**
     * @brief Retrieve a cached image by key without copying.
     * 
     * Marks the image as recently used.
     * 
     * @param key The key identifying the cached image.
     * @return ImageType The cached image (shallow copy or reference).
     * @throws std::runtime_error if the key is not found.
     */
    ImageType getCachedShallow(const std::string& key) const override {
        auto it = map_.find(key);
        if (it == map_.end()) throw std::runtime_error("Key not found in cache: " + key);
        usage_.splice(usage_.begin(), usage_, it->second.second);
        return it->second.first;
    }

    /**
     * @brief Remove an image from the cache by key.
     * 
     * If the key does not exist, this is a no-op.
     * 
     * @param key The key of the image to remove.
     */
    void remove(const std::string& key) override {
        auto it = map_.find(key);
        if (it != map_.end()) {
            usage_.erase(it->second.second);
            map_.erase(it);
        }
    }

    /**
     * @brief Clear all cached images.
     */
    void clear() override {
        map_.clear();
        usage_.clear();
    }

    /**
     * @brief Get a list of all keys currently cached, ordered from most recently used to least recently used.
     * 
     * @return std::vector<std::string> Vector of cached keys.
     */
    std::vector<std::string> getKeys() const override {
        std::vector<std::string> keys;
        for (const auto& k : usage_) keys.push_back(k);
        return keys;
    }

private:
    size_t capacity_; ///< Maximum cache size
    mutable std::list<std::string> usage_; ///< Tracks usage order: front = most recently used
    mutable std::unordered_map<std::string, std::pair<ImageType, typename std::list<std::string>::iterator>> map_; ///< Key to image and usage iterator
};

} // namespace pipeline
