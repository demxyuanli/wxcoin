#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <unordered_map>
#include "STEPReader.h"

/**
 * @brief Performance monitoring and optimization for STEP imports
 * 
 * Provides tools to monitor, analyze, and optimize STEP file import performance
 */
class STEPImportOptimizer {
public:
    /**
     * @brief Performance statistics for STEP import
     */
    struct ImportStats {
        std::string fileName;
        size_t geometryCount;
        double importTimeMs;
        double geometriesPerSecond;
        size_t fileSizeBytes;
        bool usedCache;
        std::string optimizationLevel;
        
        ImportStats() : geometryCount(0), importTimeMs(0.0), 
                       geometriesPerSecond(0.0), fileSizeBytes(0), usedCache(false) {}
    };
    
    /**
     * @brief Optimization profile for different import scenarios
     */
    struct OptimizationProfile {
        std::string name;
        STEPReader::OptimizationOptions options;
        std::string description;
        
        OptimizationProfile(const std::string& n, const STEPReader::OptimizationOptions& opts, 
                           const std::string& desc) 
            : name(n), options(opts), description(desc) {}
    };
    
    /**
     * @brief Import a STEP file with automatic optimization
     * @param filePath Path to the STEP file
     * @param profileName Name of optimization profile to use
     * @return Import result with performance statistics
     */
    static STEPReader::ReadResult importWithOptimization(const std::string& filePath, 
                                                        const std::string& profileName = "auto");
    
    /**
     * @brief Get available optimization profiles
     * @return Vector of optimization profiles
     */
    static std::vector<OptimizationProfile> getOptimizationProfiles();
    
    /**
     * @brief Get import statistics for a file
     * @param filePath Path to the file
     * @return Import statistics
     */
    static ImportStats getImportStats(const std::string& filePath);
    
    /**
     * @brief Get overall performance statistics
     * @return String with performance summary
     */
    static std::string getPerformanceSummary();
    
    /**
     * @brief Clear performance statistics
     */
    static void clearStats();
    
    /**
     * @brief Auto-detect optimal settings for a file
     * @param filePath Path to the STEP file
     * @return Optimized settings for the file
     */
    static STEPReader::OptimizationOptions autoDetectOptimalSettings(const std::string& filePath);
    
    /**
     * @brief Benchmark different optimization profiles
     * @param filePath Path to the STEP file
     * @return Vector of benchmark results
     */
    static std::vector<std::pair<std::string, ImportStats>> benchmarkProfiles(const std::string& filePath);
    
    /**
     * @brief Get recommended optimization profile for file size
     * @param fileSizeBytes File size in bytes
     * @return Recommended profile name
     */
    static std::string getRecommendedProfile(size_t fileSizeBytes);
    
private:
    STEPImportOptimizer() = delete; // Pure static class
    
    // Static members for performance tracking
    static std::unordered_map<std::string, ImportStats> s_importStats;
    static std::vector<OptimizationProfile> s_profiles;
    static bool s_profilesInitialized;
    
    /**
     * @brief Initialize optimization profiles
     */
    static void initializeProfiles();
    
    /**
     * @brief Calculate file size
     * @param filePath Path to the file
     * @return File size in bytes
     */
    static size_t getFileSize(const std::string& filePath);
    
    /**
     * @brief Record import statistics
     * @param filePath Path to the file
     * @param result Import result
     * @param profileName Profile used
     */
    static void recordStats(const std::string& filePath, 
                           const STEPReader::ReadResult& result,
                           const std::string& profileName);
}; 