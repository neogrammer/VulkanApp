#pragma once
#include <unordered_map>

enum class QFam
{
	Gfx,
	None
};

// Indices of Queue Families
struct QFamilies
{
	int gfxIdx = -1;  // location of graphics queue family
	QFam gfxName = QFam::None;
	// Check if Queue Families are valid
    bool isValid()
	{
		return (gfxIdx >= 0);
	}
};

typedef std::unordered_map<QFam,int> QueueFamilyIndices;
