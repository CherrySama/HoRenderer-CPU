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
	ProgressTracker(size_t interval = 100)
		: totalWork(0), completedWork(0), updateInterval(interval), isCompleted(false) {}

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
	size_t updateInterval;  // Update interval
	std::mutex mutex;       // Thread-safe locks
	bool isCompleted;      
	double m_totalTime = 0.0f;
};