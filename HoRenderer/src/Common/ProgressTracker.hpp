/*
    Created by Yinghao He on 2025-05-20
*/
#pragma once

#include <chrono>       
#include <mutex>        
#include <iostream>     
#include <iomanip>      
#include <algorithm>   
#include <atomic>       


class ProgressTracker {
public:
    ProgressTracker(double updateIntervalSeconds = 0.5)
        : totalWork(0), completedWork(0), 
          updateIntervalSeconds(updateIntervalSeconds), 
          isCompleted(false),
          lastUpdateTime(std::chrono::high_resolution_clock::now()) {}

	// Initialize the total workload and record the start time
	void Initialize(size_t total);

	// Update progress and display
	void Update(size_t increment = 1);

	// Print progress bar
	void PrintProgressBar(size_t completed, size_t total, double remainingTime);

	// Get the current progress percentage
	double GetProgress() const;

	// Get the estimated time remaining
	double GetRemainingTime() const;

private:
	size_t totalWork;       // Total workload
	std::atomic<size_t> completedWork;         // Amount of work done (thread-safe)
	std::chrono::high_resolution_clock::time_point startTime; 
	std::chrono::high_resolution_clock::time_point lastUpdateTime;
	double updateIntervalSeconds;  // Update interval
	std::mutex mutex;       // Thread-safe locks
	bool isCompleted;      
	double m_totalTime = 0.0f;

	// Variables for batch processing
	std::atomic<size_t> pendingUpdates{0};
    
	// Time estimation related variables
    double lastProgressRate{0.0};    // The progress change rate of the last calculation
    double lastRemainingTime{0.0};   // Last estimated time remaining
};