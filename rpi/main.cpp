#include "digitalDecoder.h"
#include "analogDecoder.h"

#include <rtl-sdr.h>

#include <iostream>
#include <cmath>
#include <unistd.h>
#include <sys/time.h>

float magLut[0x10000];


int main()
{
    //
    // Open the device
    //
    if(rtlsdr_get_device_count() < 1)
    {
        std::cout << "Could not find any devices" << std::endl;
        return -1;
    }
        
    rtlsdr_dev_t *dev = nullptr;
        
    if(rtlsdr_open(&dev, 0) < 0)
    {
        std::cout << "Failed to open device" << std::endl;
        return -1;
    }
    
    //
    // Set the frequency
    //
    if(rtlsdr_set_center_freq(dev, 345000000) < 0)
    {
        std::cout << "Failed to set frequency" << std::endl;
        return -1;
    }
    
    std::cout << "Successfully set the frequency to " << rtlsdr_get_center_freq(dev) << std::endl;
    
    //
    // Set the gain
    //
    if(rtlsdr_set_tuner_gain_mode(dev, 1) < 0)
    {
        std::cout << "Failed to set gain mode" << std::endl;
        return -1;
    }
    
    if(rtlsdr_set_tuner_gain(dev, 350) < 0)
    {
        std::cout << "Failed to set gain" << std::endl;
        return -1;
    }
    
    std::cout << "Successfully set gain to " << rtlsdr_get_tuner_gain(dev) << std::endl;
    
    //
    // Set the sample rate
    //
    if(rtlsdr_set_sample_rate(dev, 1000000) < 0)
    {
        std::cout << "Failed to set sample rate" << std::endl;
        return -1;
    }
    
    std::cout << "Successfully set the sample rate to " << rtlsdr_get_sample_rate(dev) << std::endl;
    
    //
    // Prepare for streaming
    //
    rtlsdr_reset_buffer(dev);
    
    for(uint32_t ii = 0; ii < 0x10000; ++ii)
    {
        uint8_t real_i = ii & 0xFF;
        uint8_t imag_i = ii >> 8;
        
        float real = (((float)real_i) - 127.4) * (1.0f/128.0f);
        float imag = (((float)imag_i) - 127.4) * (1.0f/128.0f);
        
        float mag = std::sqrt(real*real + imag*imag);
        magLut[ii] = mag;
    }
    
    //
    // Common Receive
    //
    AnalogDecoder aDecoder;
    DigitalDecoder dDecoder;
    
    aDecoder.setCallback([&](char data){dDecoder.handleData(data);});
    
    //
    // Async Receive
    //
    
    typedef void(*rtlsdr_read_async_cb_t)(unsigned char *buf, uint32_t len, void *ctx);
    
    auto cb = [](unsigned char *buf, uint32_t len, void *ctx)
    {
        AnalogDecoder *adec = (AnalogDecoder *)ctx;
        
        int n_samples = len/2;
        for(int i = 0; i < n_samples; ++i)
        {
            float mag = magLut[*((uint16_t*)(buf + i*2))];
            adec->handleMagnitude(mag);
        }
    };
    
    const int err = rtlsdr_read_async(dev, cb, &aDecoder, 0, 0);
    std::cout << "Read Async returned " << err << std::endl;
    
/*    
    //
    // Synchronous Receive
    //
    static const size_t BUF_SIZE = 1024*256;
    uint8_t buffer[BUF_SIZE];
    
    while(true)
    {
        int n_read = 0;
        if(rtlsdr_read_sync(dev, buffer, BUF_SIZE, &n_read) < 0)
        {
            std::cout << "Failed to read from device" << std::endl;
            return -1;
        }
        
        int n_samples = n_read/2;
        for(int i = 0; i < n_samples; ++i)
        {
            float mag = magLut[*((uint16_t*)(buffer + i*2))];
            aDecoder.handleMagnitude(mag);
        }
    }
*/    
    //
    // Shut down
    //
    rtlsdr_close(dev);
    return 0;
}




