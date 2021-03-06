// Copyright (C) 2015  Davis E. King (davis@dlib.net)
// License: Boost Software License   See LICENSE.txt for the full license.
#ifndef DLIB_GPU_DaTA_CPP_
#define DLIB_GPU_DaTA_CPP_

// Only things that require CUDA are declared in this cpp file.  Everything else is in the
// gpu_data.h header so that it can operate as "header-only" code when using just the CPU.
#ifdef DLIB_USE_CUDA

#include "gpu_data.h"
#include <iostream>
#include "cuda_utils.h"
#include <cstring>


namespace dlib
{

// ----------------------------------------------------------------------------------------

    void memcpy (
        gpu_data& dest, 
        const gpu_data& src
    )
    {
        DLIB_CASSERT(dest.size() == src.size(), "");
        if (src.size() == 0)
            return;

        // copy the memory efficiently based on which copy is current in each object.
        if (dest.device_ready() && src.device_ready())
            CHECK_CUDA(cudaMemcpy(dest.device(), src.device(),          src.size()*sizeof(float), cudaMemcpyDeviceToDevice));
        else if (!dest.device_ready() && src.device_ready())
            CHECK_CUDA(cudaMemcpy(dest.host_write_only(), src.device(), src.size()*sizeof(float), cudaMemcpyDeviceToHost));
        else if (dest.device_ready() && !src.device_ready())
            CHECK_CUDA(cudaMemcpy(dest.device(), src.host(),            src.size()*sizeof(float), cudaMemcpyHostToDevice));
        else 
            CHECK_CUDA(cudaMemcpy(dest.host_write_only(), src.host(),   src.size()*sizeof(float), cudaMemcpyHostToHost));
    }

// ----------------------------------------------------------------------------------------

    void gpu_data::
    wait_for_transfer_to_finish() const
    {
        if (have_active_transfer)
        {
            CHECK_CUDA(cudaStreamSynchronize((cudaStream_t)cuda_stream.get()));
            have_active_transfer = false;
            // Check for errors.  These calls to cudaGetLastError() are what help us find
            // out if our kernel launches have been failing.
            CHECK_CUDA(cudaGetLastError());
        }
    }

    void gpu_data::
    copy_to_device() const
    {
        // We want transfers to the device to always be concurrent with any device
        // computation.  So we use our non-default stream to do the transfer.
        async_copy_to_device();
        wait_for_transfer_to_finish();
    }

    void gpu_data::
    copy_to_host() const
    {
        if (!host_current)
        {
            wait_for_transfer_to_finish();
            CHECK_CUDA(cudaMemcpy(data_host.get(), data_device.get(), data_size*sizeof(float), cudaMemcpyDeviceToHost));
            host_current = true;
            // At this point we know our RAM block isn't in use because cudaMemcpy()
            // implicitly syncs with the device. 
            device_in_use = false;
            // Check for errors.  These calls to cudaGetLastError() are what help us find
            // out if our kernel launches have been failing.
            CHECK_CUDA(cudaGetLastError());
        }
    }

    void gpu_data::
    async_copy_to_device() const
    {
        if (!device_current)
        {
            if (device_in_use)
            {
                // Wait for any possible CUDA kernels that might be using our memory block to
                // complete before we overwrite the memory.
                CHECK_CUDA(cudaStreamSynchronize(0));
                device_in_use = false;
            }
            CHECK_CUDA(cudaMemcpyAsync(data_device.get(), data_host.get(), data_size*sizeof(float), cudaMemcpyHostToDevice, (cudaStream_t)cuda_stream.get()));
            have_active_transfer = true;
            device_current = true;
        }
    }

    void gpu_data::
    set_size(
        size_t new_size
    )
    {
        if (new_size == 0)
        {
            if (device_in_use)
            {
                // Wait for any possible CUDA kernels that might be using our memory block to
                // complete before we free the memory.
                CHECK_CUDA(cudaStreamSynchronize(0));
                device_in_use = false;
            }
            wait_for_transfer_to_finish();
            data_size = 0;
            host_current = true;
            device_current = true;
            device_in_use = false;
            data_host.reset();
            data_device.reset();
        }
        else if (new_size != data_size)
        {
            if (device_in_use)
            {
                // Wait for any possible CUDA kernels that might be using our memory block to
                // complete before we free the memory.
                CHECK_CUDA(cudaStreamSynchronize(0));
                device_in_use = false;
            }
            wait_for_transfer_to_finish();
            data_size = new_size;
            host_current = true;
            device_current = true;
            device_in_use = false;

            try
            {
                CHECK_CUDA(cudaGetDevice(&the_device_id));

                void* data;
                CHECK_CUDA(cudaMallocHost(&data, new_size*sizeof(float)));
                // Note that we don't throw exceptions since the free calls are invariably
                // called in destructors.  They also shouldn't fail anyway unless someone
                // is resetting the GPU card in the middle of their program.
                data_host.reset((float*)data, [](float* ptr){
                    auto err = cudaFreeHost(ptr);
                    if(err!=cudaSuccess)
                        std::cerr << "cudaFreeHost() failed. Reason: " << cudaGetErrorString(err) << std::endl;
                });

                CHECK_CUDA(cudaMalloc(&data, new_size*sizeof(float)));
                data_device.reset((float*)data, [](float* ptr){
                    auto err = cudaFree(ptr);
                    if(err!=cudaSuccess)
                        std::cerr << "cudaFree() failed. Reason: " << cudaGetErrorString(err) << std::endl;
                });

                if (!cuda_stream)
                {
                    cudaStream_t cstream;
                    CHECK_CUDA(cudaStreamCreateWithFlags(&cstream, cudaStreamNonBlocking));
                    cuda_stream.reset(cstream, [](void* ptr){
                        auto err = cudaStreamDestroy((cudaStream_t)ptr);
                        if(err!=cudaSuccess)
                            std::cerr << "cudaStreamDestroy() failed. Reason: " << cudaGetErrorString(err) << std::endl;
                    });
                }

            }
            catch(...)
            {
                set_size(0);
                abort();
            }
        }
    }

// ----------------------------------------------------------------------------------------
}

#endif // DLIB_USE_CUDA

#endif // DLIB_GPU_DaTA_CPP_

