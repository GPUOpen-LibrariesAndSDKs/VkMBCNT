#include "VulkanSample.h"

int main (int /*argc*/, char* /*argv*/[])
{
	auto sample = new AMD::VulkanComputeSample;
	
	sample->Run ();
	delete sample;

	return 0;
}