#include "STEPImportOptimizer.h"
#include "STEPReader.h"
#include "logger/Logger.h"
#include <filesystem>
#include <algorithm>
#include <sstream>
#include <iomanip>

// Static member initialization
std::unordered_map<std::string, STEPImportOptimizer::ImportStats> STEPImportOptimizer::s_importStats;
std::vector<STEPImportOptimizer::OptimizationProfile> STEPImportOptimizer::s_profiles;
bool STEPImportOptimizer::s_profilesInitialized = false;

STEPReader::ReadResult STEPImportOptimizer::importWithOptimization(const std::string& filePath,
	const std::string& profileName)
{
	if (!s_profilesInitialized) {
		initializeProfiles();
	}

	// Auto-detect profile if "auto" is specified
	if (profileName == "auto") {
		size_t fileSize = getFileSize(filePath);
		std::string recommendedProfile = getRecommendedProfile(fileSize);
		LOG_INF_S("Auto-selected optimization profile: " + recommendedProfile + " for file size: " + std::to_string(fileSize) + " bytes");

		// Find the recommended profile
		auto it = std::find_if(s_profiles.begin(), s_profiles.end(),
			[&recommendedProfile](const OptimizationProfile& profile) {
				return profile.name == recommendedProfile;
			});

		if (it != s_profiles.end()) {
			auto result = STEPReader::readSTEPFile(filePath, it->options);
			recordStats(filePath, result, recommendedProfile);
			return result;
		}
	}

	// Use specified profile
	auto it = std::find_if(s_profiles.begin(), s_profiles.end(),
		[&profileName](const OptimizationProfile& profile) {
			return profile.name == profileName;
		});

	if (it != s_profiles.end()) {
		auto result = STEPReader::readSTEPFile(filePath, it->options);
		recordStats(filePath, result, profileName);
		return result;
	}

	// Fallback to default profile
	LOG_WRN_S("Profile not found: " + profileName + ", using default");
	auto result = STEPReader::readSTEPFile(filePath);
	recordStats(filePath, result, "default");
	return result;
}

std::vector<STEPImportOptimizer::OptimizationProfile> STEPImportOptimizer::getOptimizationProfiles()
{
	if (!s_profilesInitialized) {
		initializeProfiles();
	}
	return s_profiles;
}

STEPImportOptimizer::ImportStats STEPImportOptimizer::getImportStats(const std::string& filePath)
{
	auto it = s_importStats.find(filePath);
	if (it != s_importStats.end()) {
		return it->second;
	}
	return ImportStats();
}

std::string STEPImportOptimizer::getPerformanceSummary()
{
	if (s_importStats.empty()) {
		return "No import statistics available";
	}

	std::ostringstream oss;
	oss << "STEP Import Performance Summary\n";
	oss << "===============================\n\n";

	size_t totalImports = s_importStats.size();
	double totalTime = 0.0;
	size_t totalGeometries = 0;
	size_t totalFileSize = 0;
	size_t cacheHits = 0;

	for (const auto& stat : s_importStats) {
		totalTime += stat.second.importTimeMs;
		totalGeometries += stat.second.geometryCount;
		totalFileSize += stat.second.fileSizeBytes;
		if (stat.second.usedCache) {
			cacheHits++;
		}
	}

	double avgTime = totalTime / totalImports;
	double avgGeometries = static_cast<double>(totalGeometries) / totalImports;
	double avgFileSize = static_cast<double>(totalFileSize) / totalImports;
	double cacheHitRate = static_cast<double>(cacheHits) / totalImports * 100.0;

	oss << "Total imports: " << totalImports << "\n";
	oss << "Average import time: " << std::fixed << std::setprecision(1) << avgTime << " ms\n";
	oss << "Average geometries per import: " << std::fixed << std::setprecision(1) << avgGeometries << "\n";
	oss << "Average file size: " << std::fixed << std::setprecision(0) << avgFileSize / 1024.0 << " KB\n";
	oss << "Cache hit rate: " << std::fixed << std::setprecision(1) << cacheHitRate << "%\n";
	oss << "Overall performance: " << std::fixed << std::setprecision(1) << (totalGeometries / (totalTime / 1000.0)) << " geometries/second\n";

	return oss.str();
}

void STEPImportOptimizer::clearStats()
{
	s_importStats.clear();
	LOG_INF_S("STEP import statistics cleared");
}

STEPReader::OptimizationOptions STEPImportOptimizer::autoDetectOptimalSettings(const std::string& filePath)
{
	size_t fileSize = getFileSize(filePath);

	STEPReader::OptimizationOptions options;

	// Auto-detect based on file size
	if (fileSize < 1024 * 1024) { // < 1MB
		// Small files: use high precision and analysis
		options.enableParallelProcessing = false;
		options.enableShapeAnalysis = true;
		options.enableCaching = true;
		options.enableBatchOperations = false;
		options.maxThreads = 1;
		options.precision = 0.001;
	}
	else if (fileSize < 10 * 1024 * 1024) { // < 10MB
		// Medium files: balanced approach
		options.enableParallelProcessing = true;
		options.enableShapeAnalysis = false;
		options.enableCaching = true;
		options.enableBatchOperations = true;
		options.maxThreads = std::thread::hardware_concurrency();
		options.precision = 0.01;
	}
	else { // >= 10MB
		// Large files: prioritize speed
		options.enableParallelProcessing = true;
		options.enableShapeAnalysis = false;
		options.enableCaching = true;
		options.enableBatchOperations = true;
		options.maxThreads = std::thread::hardware_concurrency();
		options.precision = 0.1;
	}

	return options;
}

std::vector<std::pair<std::string, STEPImportOptimizer::ImportStats>> STEPImportOptimizer::benchmarkProfiles(const std::string& filePath)
{
	if (!s_profilesInitialized) {
		initializeProfiles();
	}

	std::vector<std::pair<std::string, ImportStats>> results;

	for (const auto& profile : s_profiles) {
		LOG_INF_S("Benchmarking profile: " + profile.name);

		// Clear cache before each benchmark
		STEPReader::clearCache();

		auto result = STEPReader::readSTEPFile(filePath, profile.options);

		ImportStats stats;
		stats.fileName = std::filesystem::path(filePath).filename().string();
		stats.geometryCount = result.geometries.size();
		stats.importTimeMs = result.importTime;
		stats.geometriesPerSecond = result.geometries.size() / (result.importTime / 1000.0);
		stats.fileSizeBytes = getFileSize(filePath);
		stats.usedCache = false; // Cache was cleared
		stats.optimizationLevel = profile.name;

		results.push_back({ profile.name, stats });
	}

	// Sort by performance (geometries per second)
	std::sort(results.begin(), results.end(),
		[](const auto& a, const auto& b) {
			return a.second.geometriesPerSecond > b.second.geometriesPerSecond;
		});

	return results;
}

std::string STEPImportOptimizer::getRecommendedProfile(size_t fileSizeBytes)
{
	if (fileSizeBytes < 1024 * 1024) { // < 1MB
		return "precision";
	}
	else if (fileSizeBytes < 10 * 1024 * 1024) { // < 10MB
		return "balanced";
	}
	else { // >= 10MB
		return "speed";
	}
}

void STEPImportOptimizer::initializeProfiles()
{
	s_profiles.clear();

	// Precision profile - for small files, high quality
	STEPReader::OptimizationOptions precisionOpts;
	precisionOpts.enableParallelProcessing = false;
	precisionOpts.enableShapeAnalysis = true;
	precisionOpts.enableCaching = true;
	precisionOpts.enableBatchOperations = false;
	precisionOpts.maxThreads = 1;
	precisionOpts.precision = 0.001;
	s_profiles.emplace_back("precision", precisionOpts, "High precision, detailed analysis");

	// Balanced profile - for medium files
	STEPReader::OptimizationOptions balancedOpts;
	balancedOpts.enableParallelProcessing = true;
	balancedOpts.enableShapeAnalysis = false;
	balancedOpts.enableCaching = true;
	balancedOpts.enableBatchOperations = true;
	balancedOpts.maxThreads = std::thread::hardware_concurrency();
	balancedOpts.precision = 0.01;
	s_profiles.emplace_back("balanced", balancedOpts, "Balanced speed and quality");

	// Speed profile - for large files
	STEPReader::OptimizationOptions speedOpts;
	speedOpts.enableParallelProcessing = true;
	speedOpts.enableShapeAnalysis = false;
	speedOpts.enableCaching = true;
	speedOpts.enableBatchOperations = true;
	speedOpts.maxThreads = std::thread::hardware_concurrency();
	speedOpts.precision = 0.1;
	s_profiles.emplace_back("speed", speedOpts, "Maximum speed, basic quality");

	// Ultra-fast profile - for very large files
	STEPReader::OptimizationOptions ultraFastOpts;
	ultraFastOpts.enableParallelProcessing = true;
	ultraFastOpts.enableShapeAnalysis = false;
	ultraFastOpts.enableCaching = false; // Disable cache for memory efficiency
	ultraFastOpts.enableBatchOperations = true;
	ultraFastOpts.maxThreads = std::thread::hardware_concurrency();
	ultraFastOpts.precision = 0.5;
	s_profiles.emplace_back("ultra-fast", ultraFastOpts, "Ultra-fast import, minimal memory usage");

	s_profilesInitialized = true;
	LOG_INF_S("STEP optimization profiles initialized");
}

size_t STEPImportOptimizer::getFileSize(const std::string& filePath)
{
	try {
		return std::filesystem::file_size(filePath);
	}
	catch (const std::exception& e) {
		LOG_WRN_S("Could not get file size for " + filePath + ": " + e.what());
		return 0;
	}
}

void STEPImportOptimizer::recordStats(const std::string& filePath,
	const STEPReader::ReadResult& result,
	const std::string& profileName)
{
	ImportStats stats;
	stats.fileName = std::filesystem::path(filePath).filename().string();
	stats.geometryCount = result.geometries.size();
	stats.importTimeMs = result.importTime;
	stats.geometriesPerSecond = result.geometries.size() / (result.importTime / 1000.0);
	stats.fileSizeBytes = getFileSize(filePath);
	stats.usedCache = false; // TODO: Implement cache hit detection
	stats.optimizationLevel = profileName;

	s_importStats[filePath] = stats;

	LOG_INF_S("Recorded import stats: " + std::to_string(stats.geometryCount) +
		" geometries in " + std::to_string(stats.importTimeMs) + "ms " +
		"(" + std::to_string(stats.geometriesPerSecond) + " geo/s)");
}