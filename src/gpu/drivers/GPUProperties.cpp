#include "GPUProperties.hpp"

//#include <cgi/logger/logger.hpp>

namespace cvt {
namespace gpu {

GPUProperties::GPUProperties(int gpu_id)
{
	cudaGetDeviceProperties(&_props, gpu_id);
	
	std::stringstream comp_stream;

	comp_stream << _props.major << "." << _props.minor;
	
	compute = atof(comp_stream.str().c_str());
}

GPUProperties::~GPUProperties() {
	;
}

std::vector<int> GPUProperties::getMaxGridDims() const
{
	std::vector<int> dims;
	dims.assign(_props.maxGridSize, _props.maxGridSize + 3);
	return dims;
}

std::vector<int> GPUProperties::getMaxSurface1DLayeredDims() const
{
	std::vector<int> dims;
	dims.assign(_props.maxSurface1DLayered, _props.maxSurface1DLayered + 2);
	return dims;
}


std::vector<int> GPUProperties::getMaxSurface2DDims() const
{
	std::vector<int> dims;
	dims.assign(_props.maxSurface2D, _props.maxSurface2D + 2);
	return dims;
}

std::vector<int> GPUProperties::getMaxSurface2DLayeredDims() const
{
	std::vector<int> dims;
	dims.assign(_props.maxSurface2DLayered, _props.maxSurface2DLayered + 3);
	return dims;
}

std::vector<int> GPUProperties::getMaxSurface3DDims() const
{
	std::vector<int> dims;
	dims.assign(_props.maxSurface3D, _props.maxSurface3D + 3);
	return dims;
}

std::vector<int> GPUProperties::getMaxSurfaceCubemapLayered() const
{
	std::vector<int> dims;
	dims.assign(_props.maxSurfaceCubemapLayered, _props.maxSurfaceCubemapLayered + 2);
	return dims;
}

std::vector<int> GPUProperties::getMaxTexture1DLayered() const
{
	std::vector<int> dims;
	dims.assign(_props.maxTexture1DLayered, _props.maxTexture1DLayered + 2);
	return dims;
}

std::vector<int> GPUProperties::getMaxTexture2DDims() const
{
	std::vector<int> dims;
	dims.assign(_props.maxTexture2D, _props.maxTexture2D + 2);
	return dims;
}

std::vector<int> GPUProperties::getMaxTexture2DGatherDims() const
{
	std::vector<int> dims;
	dims.assign(_props.maxTexture2DGather, _props.maxTexture2DGather + 2);
	return dims;
}

std::vector<int> GPUProperties::getMaxTexture2DLayered() const
{
	std::vector<int> dims;
	dims.assign(_props.maxTexture2DLayered, _props.maxTexture2DLayered + 3);
	return dims;
}

std::vector<int> GPUProperties::getMaxTexture2DLinearDims() const
{
	std::vector<int> dims;
	dims.assign(_props.maxTexture2DLinear, _props.maxTexture2DLinear + 3);
	return dims;
}

std::vector<int> GPUProperties::getMaxTexture3DDims() const
{
	std::vector<int> dims;
	dims.assign(_props.maxTexture3D, _props.maxTexture3D + 3);
	return dims;
}

std::vector<int> GPUProperties::getMaxTextureCubemapLayered() const
{
	std::vector<int> dims;
	dims.assign(_props.maxTextureCubemapLayered, _props.maxTextureCubemapLayered + 2);
	return dims;
}


std::vector<int> GPUProperties::getMaxThreadsDims() const
{
	std::vector<int> dims;
	dims.assign(_props.maxThreadsDim, _props.maxThreadsDim + 3);
	return dims;
}

std::ostream& operator<<(std::ostream& os, const cvt::gpu::GPUProperties& p) {

	const size_t global_mb = p.getTotalGlobalMemoryBytes() / (1024 * 1024);
	const size_t constant_kb = p.getTotalConstantMemoryBytes() / 1024;
	const size_t l2_kb = p.getL2CacheSizeBytes() / 1024;
	const size_t shared = p.getSharedMemPerBlockBytes();

	os 	<< "Name:" << p.getDeviceName()
		<< "\nCompute Mode:" << p.getComputeMode()
		<< "\nCompute Capability:" << p.getMajorCompute() << "." << p.getMinorCompute()
		<< "\nCompute Capability (double): " << p.getCompute()
		<< "\nWarp Size:" << p.getWarpSize()
		<< "\nMaximum Threads Per Block:" << p.getMaxThreadsPerBlock()
		<< "\nMaximum Block Thread Dimensions:" << p.getMaxThreadsPerBlock_X() << "," << p.getMaxThreadsPerBlock_Y() << "," << p.getMaxThreadsPerBlock_Z()
		<< "\nMaximum Grid Size:" << p.getMaxGridDim_X() << "," << p.getMaxGridDim_Y() << "," << p.getMaxGridDim_Z()
		<< "\nMultiprocessor Count:" << p.getMultiProcessorCount()
		<< "\nMaximum Threads per MP:" << p.getMaxThreadsPerMultiProcessor()
		<< "\nGlobal Memory Size (MB):" << global_mb
		<< "\nConstant Memory Size (KB):" << constant_kb
		<< "\nShared Memory Per Block Size (B): " << shared
		<< "\nMax 2D Texture Dims: " << p.getMaxTexture2DDims()[0] << ", " << p.getMaxTexture2DDims()[1]
		<< "\nL2 Cache Size (KB):" << l2_kb	;
	return os;
}

/* Non-Class Functions */

///
/// This function looks for the most powerful GPUs
///
std::vector<int> getGpuDeviceIds(int min_major , int min_minor ) 
{
	std::vector<int> gpuDeviceIds;

	//Get the number of GPU's
	int raw_number_of_gpu_devices;
	cudaGetDeviceCount(&raw_number_of_gpu_devices);
	
	//Return false if there are no GPUs
	if (raw_number_of_gpu_devices == 0) 
	{
//		cgi::log::MetaLogStream::instance() << cgi::log::Priority::WARN << "cgi::gpu::getGpuDeviceIds"
//		<< "No raw GPU devices found" << cgi::log::flush;

		std::cout << "cvt::gpu::getGpuDeviceIds"
				<< "No raw GPU devices found" << std::endl;

		return gpuDeviceIds;
	}
	else
	{
//		cgi::log::MetaLogStream::instance() << cgi::log::Priority::DEBUG << "cgi::gpu::getGpuDeviceIds"
//		<< raw_number_of_gpu_devices << " raw GPU devices found" << cgi::log::flush;

		std::cout << "cvt::gpu::getGpuDeviceIds"
				<< raw_number_of_gpu_devices << " raw GPU devices found" << std::endl;
	}

	//Filter out non CUDA capabale GPU devices
	for (size_t i=0; i<raw_number_of_gpu_devices; i++) 
	{
		cudaDeviceProp deviceProp;
		cudaGetDeviceProperties(&deviceProp, i);

		//If the device is Cuda Capable, it will have a major >= 1
		if (deviceProp.major >= 1 && deviceProp.major >= min_major) 
		{
			if (deviceProp.major > min_major || deviceProp.minor >= min_minor)
			{
//				cgi::log::MetaLogStream::instance() << cgi::log::Priority::DEBUG << "cgi::gpu::getGpuDeviceIds"
//				<<  "Raw GPU device (<<" << ") found with compute : " << deviceProp.major << "." << deviceProp.minor << cgi::log::flush;

				std::cout << "cvt::gpu::getGpuDeviceIds"
						<<  "Raw GPU device (<<" << ") found with compute : " << deviceProp.major << "." << deviceProp.minor << std::endl;

				gpuDeviceIds.push_back(i);
			}
		}
	}

	return gpuDeviceIds;
}


}; // END cgi::gpu namespace
}; // END cgi namespace

