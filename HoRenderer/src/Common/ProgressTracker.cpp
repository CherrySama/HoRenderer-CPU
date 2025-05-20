/*
    Created by Yinghao He on 2025-05-20
*/
#include "ProgressTracker.hpp"

void ProgressTracker::Initialize(size_t total)
{
	totalWork = total;
	completedWork = 0;
	isCompleted = false;
	startTime = std::chrono::high_resolution_clock::now();
}

void ProgressTracker::Update(size_t increment)
{
	//std::lock_guard<std::mutex> lock(mutex); // 确保线程安全
	size_t newCompleted = completedWork.fetch_add(increment) + increment;

	// Show progress by update interval or when task is completed
	if (newCompleted % updateInterval == 0 || newCompleted >= totalWork) {
		auto currentTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed = currentTime - startTime;
		size_t clampedCompleted = std::min(newCompleted, totalWork); 
		double progress = static_cast<double>(clampedCompleted) / totalWork;
		double estimatedTotalTime = (progress > 0) ? (elapsed.count() / progress) : 0.0;
		double remainingTime = (progress < 1.0) ? (estimatedTotalTime - elapsed.count()) : 0.0;

		// Use a mutex to protect updates to m_totalTime and isCompleted
		{
			std::lock_guard<std::mutex> lock(mutex);
			if (newCompleted >= totalWork && !isCompleted) {
				m_totalTime = elapsed.count();
				isCompleted = true;
			}
		}

		PrintProgressBar(clampedCompleted, totalWork, remainingTime);

		//if (newCompleted >= totalWork && !isCompleted) 
		//	isCompleted = true;

	}
}

void ProgressTracker::PrintProgressBar(size_t completed, size_t total, double remainingTime)
{
	std::lock_guard<std::mutex> lock(mutex); 
	const int barWidth = 50; // Progress bar width
	double progress = std::min(static_cast<double>(completed) / total, 1.0); // Make sure progress does not exceed 100%
	int pos = static_cast<int>(barWidth * progress);

	std::cout << "\r[";
	for (int i = 0; i < barWidth; ++i) {
		if (i < pos) std::cout << "=";      // Completed
		else if (i == pos) std::cout << ">"; // Current progress point
		else std::cout << " ";              // Unfinished part
	}
	std::cout << "] " << std::fixed << std::setprecision(2) << (progress * 100)
		<< "% | Remaining: " << remainingTime << "s";

	if (isCompleted) 
		std::cout << " | Total time: " << std::fixed << std::setprecision(2) << m_totalTime << "s" << std::flush;
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
