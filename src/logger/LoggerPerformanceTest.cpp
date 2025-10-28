#include "logger/AsyncLogger.h"
#include "logger/Logger.h"
#include <chrono>
#include <thread>
#include <vector>
#include <iostream>

class LoggerPerformanceTest {
public:
    static void runPerformanceComparison() {
        std::cout << "=== Logger Performance Comparison ===" << std::endl;
        
        const int numLogs = 10000;
        const int numThreads = 4;
        
        // Test synchronous logger
        std::cout << "\nTesting Synchronous Logger..." << std::endl;
        auto syncTime = testSynchronousLogger(numLogs, numThreads);
        
        // Test asynchronous logger
        std::cout << "\nTesting Asynchronous Logger..." << std::endl;
        auto asyncTime = testAsynchronousLogger(numLogs, numThreads);
        
        // Results
        std::cout << "\n=== Results ===" << std::endl;
        std::cout << "Synchronous Logger: " << syncTime << " ms" << std::endl;
        std::cout << "Asynchronous Logger: " << asyncTime << " ms" << std::endl;
        std::cout << "Performance Improvement: " << (double)syncTime / asyncTime << "x faster" << std::endl;
        
        // Cleanup
        AsyncLogger::getLogger().Shutdown();
    }
    
private:
    static long long testSynchronousLogger(int numLogs, int numThreads) {
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        for (int t = 0; t < numThreads; ++t) {
            threads.emplace_back([numLogs, t]() {
                for (int i = 0; i < numLogs / numThreads; ++i) {
                    LOG_INF_S("Sync test message " + std::to_string(i) + " from thread " + std::to_string(t));
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }
    
    static long long testAsynchronousLogger(int numLogs, int numThreads) {
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        for (int t = 0; t < numThreads; ++t) {
            threads.emplace_back([numLogs, t]() {
                for (int i = 0; i < numLogs / numThreads; ++i) {
                    LOG_INF_S_ASYNC("Async test message " + std::to_string(i) + " from thread " + std::to_string(t));
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Wait for all logs to be processed
        while (AsyncLogger::getLogger().getQueueSize() > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }
};

// Test function that can be called from main
void testLoggerPerformance() {
    LoggerPerformanceTest::runPerformanceComparison();
}




