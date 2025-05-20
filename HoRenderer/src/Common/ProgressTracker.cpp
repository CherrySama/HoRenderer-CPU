/*
    Created by Yinghao He on 2025-05-20
*/
#include "ProgressTracker.hpp"

void ProgressTracker::Initialize(size_t total)
{
    totalWork = total;
    completedWork = 0;
    pendingUpdates = 0;
    isCompleted = false;
    lastProgressRate = 0.0;
    lastRemainingTime = 0.0;
    startTime = std::chrono::high_resolution_clock::now();
    lastUpdateTime = startTime;
}

void ProgressTracker::Update(size_t increment)
{
    // Accumulate work updates without immediate display
    size_t newCompleted = completedWork.fetch_add(increment) + increment;
    pendingUpdates.fetch_add(increment);
    
    // Get current time
    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> timeSinceLastUpdate = currentTime - lastUpdateTime;
    
    // Determine if display should be updated: based on time interval or task completion
    bool shouldUpdate = (timeSinceLastUpdate.count() >= updateIntervalSeconds) || 
                        (newCompleted >= totalWork && pendingUpdates > 0);
    
    if (shouldUpdate) {
        // Attempt to acquire update lock; skip if another thread is already updating
        if (mutex.try_lock()) {
            // Clear pending updates
            pendingUpdates.store(0);
            lastUpdateTime = currentTime;
            
            // Calculate progress and time
            std::chrono::duration<double> elapsed = currentTime - startTime;
            size_t clampedCompleted = std::min(newCompleted, totalWork);
            double progress = static_cast<double>(clampedCompleted) / totalWork;
            
            // Calculate current progress rate (per second)
            double currentProgressRate = progress / elapsed.count();
            
            // Smooth the rate using exponential moving average
            if (lastProgressRate > 0) {
                // 75% old data, 25% new data to reduce fluctuations
                lastProgressRate = 0.75 * lastProgressRate + 0.25 * currentProgressRate;
            } else {
                lastProgressRate = currentProgressRate;
            }
            
            // Calculate remaining time
            double remainingProgress = 1.0 - progress;
            double remainingTime = (lastProgressRate > 0) ? (remainingProgress / lastProgressRate) : 0.0;
            lastRemainingTime = remainingTime;
            
            // Update completion status
            if (newCompleted >= totalWork && !isCompleted) {
                m_totalTime = elapsed.count();
                isCompleted = true;
            }
            
            // Print progress bar
            PrintProgressBar(clampedCompleted, totalWork, remainingTime);
            
            mutex.unlock();
        }
    }
}

void ProgressTracker::PrintProgressBar(size_t completed, size_t total, double remainingTime)
{
	// std::lock_guard<std::mutex> lock(mutex); 
	const int barWidth = 50; // Progress bar width
	double progress = std::min(static_cast<double>(completed) / total, 1.0); // Make sure progress does not exceed 100%
	int pos = static_cast<int>(barWidth * progress);

	std::stringstream ss;
	ss << "\r[";
	for (int i = 0; i < barWidth; ++i) {
		if (i < pos) ss << "=";      // Completed
		else if (i == pos) ss << ">"; // Current progress point
		else ss << " ";              // Unfinished part
	}

	ss << "] " << std::fixed << std::setprecision(2) << (progress * 100)
	<< "% | Progress per second: " << std::fixed << std::setprecision(4) << (lastProgressRate * 100)
	<< "% | Remaining time: " << std::fixed << std::setprecision(1) << remainingTime << " seconds";

	if (isCompleted) 
		ss << " | Total time: " << std::fixed << std::setprecision(2) << m_totalTime << "s" << std::flush;
	
	std::cout << ss.str() << std::flush;
}

double ProgressTracker::GetProgress() const
{
	return totalWork > 0 ? static_cast<double>(completedWork) / totalWork : 0.0;
}

double ProgressTracker::GetRemainingTime() const
{
	if (completedWork == 0) 
		return 0.0;
	auto currentTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = currentTime - startTime;
	double progress = static_cast<double>(completedWork) / totalWork;
	double estimatedTotalTime = elapsed.count() / progress;
	return estimatedTotalTime - elapsed.count();
}
