#include "digitalDecoder.h"

#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>


#define SYNC_MASK    0xFFFF000000000000ul
#define SYNC_PATTERN 0xFFFE000000000000ul


void DigitalDecoder::writeDeviceState()
{
    std::ofstream file;
    file.open("/var/www/html/deviceState.json");
    
    if(!file.is_open())
    {
        printf("Could not open JSON file\n");
        return;
    }
    
    bool isFirst = true;
    
    file << "[" << std::endl;
    for(const auto &dd : deviceStateMap)
    {
        if(!isFirst) file << "," << std::endl;
        isFirst = false;
        
        file << "{" << std::endl;
        file << "    \"serial\": " << dd.first << "," << std::endl;
        file << "    \"isMotion\": " << (dd.second.isMotionDetector ? "true," : "false,") << std::endl;
        file << "    \"tamper\": " << (dd.second.tamper ? "true," : "false,") << std::endl;
        file << "    \"alarm\": " << (dd.second.alarm ? "true," : "false,") << std::endl;
        file << "    \"batteryLow\": " << (dd.second.batteryLow ? "true," : "false,") << std::endl;
        file << "    \"lastUpdateTime\": " << dd.second.lastUpdateTime << "," << std::endl;
        file << "    \"lastAlarmTime\": " << dd.second.lastAlarmTime << std::endl;
        file << "}";
    }
    
    file << std::endl << "]" << std::endl;
    
    file.close();
}

#warning "Update the SmartThings endpoint here"
void DigitalDecoder::sendDeviceState()
{
    printf("Sending Device State\n");
    system("curl -H 'Authorization: Bearer 12345678-1234-1234-1234-1234567890ab' -H 'Content-Type: application/json' -X PUT -d '@/var/www/html/deviceState.json' https://graph.api.smartthings.com:443/api/smartapps/installations/12345678-1234-1234-1234-1234567890ab/event&");
}

void DigitalDecoder::updateDeviceState(uint32_t serial, uint8_t state)
{
    deviceState_t ds;
    
    // Extract prior info
    if(deviceStateMap.count(serial))
    {
        ds = deviceStateMap[serial];
    }
    else
    {
        ds.isMotionDetector = false;
    }
    
    // Watch for anything that indicates this sensor is a motion detector
    if(state < 0x80) ds.isMotionDetector = true;
    
    // Decode motion/open bits
    if(ds.isMotionDetector)
    {
        ds.alarm = (state & 0x80);
    }
    else
    {
        ds.alarm = (state & 0x20);
    }
    
    // Decode tamper bit
    ds.tamper = (state & 0x40);
    
    // Decode battery low bit    
    ds.batteryLow = (state & 0x08);
    
    // Timestamp
    timeval now;
    gettimeofday(&now, nullptr);
    ds.lastUpdateTime = now.tv_sec;
    
    if(ds.alarm) ds.lastAlarmTime = now.tv_sec;

    // Put the answer back in the map
    deviceStateMap[serial] = ds;
    
    // Record the current state
    writeDeviceState();
    
    // Send the notification if something changed
    if(state != ds.lastRawState)
    {
        sendDeviceState();
    }
    deviceStateMap[serial].lastRawState = state;
    
    for(const auto &dd : deviceStateMap)
    {
        printf("%sDevice %7u: %s\n",dd.first==serial ? "*" : " ", dd.first, dd.second.alarm ? "ALARM" : "OK");
    }
    printf("\n");
}

void DigitalDecoder::handlePayload(uint64_t payload)
{
    uint64_t sof = (payload & 0xF00000000000) >> 44;
    uint64_t ser = (payload & 0x0FFFFF000000) >> 24;
    uint64_t typ = (payload & 0x000000FF0000) >> 16;
    uint64_t crc = (payload & 0x00000000FFFF) >>  0;
    
    //
    // Check CRC
    //
    const uint64_t polynomial = 0x18005;
    uint64_t sum = payload & (~SYNC_MASK);
    uint64_t current_divisor = polynomial << 31;
    
    while(current_divisor >= polynomial)
    {
#ifdef __arm__
        if(__builtin_clzll(sum) == __builtin_clzll(current_divisor))
#else        
        if(__builtin_clzl(sum) == __builtin_clzl(current_divisor))
#endif
        {
            sum ^= current_divisor;
        }        
        current_divisor >>= 1;
    }
    
    const bool valid = (sum == 0);
    
    //
    // Tell the world
    //
    if(valid)
    {
        updateDeviceState(ser, typ);
    }
    
    
    //
    // Print Packet
    //
//    if(valid)    
//        printf("Payload: %lX (Serial %lu, Status %lX, %s)\n", payload, ser, typ, valid ? "VALID":"ERROR");
//    else
//        printf("Payload: %lX %s\n", payload, valid ? "VALID":"ERROR");

    static uint32_t packetCount = 0;
    static uint32_t errorCount = 0;
    
    packetCount++;
    if(!valid)
    {
        errorCount++;
        printf("%u/%u packets failed CRC\n", errorCount, packetCount);
    }
}



void DigitalDecoder::handleBit(bool value)
{
    static uint64_t payload = 0;
    
    payload <<= 1;
    payload |= (value ? 1 : 0);
    
//#ifdef __arm__
//    printf("Got bit: %d, payload is now %llX\n", value?1:0, payload);
//#else
//    printf("Got bit: %d, payload is now %lX\n", value?1:0, payload);
//#endif     
    
    if((payload & SYNC_MASK) == SYNC_PATTERN)
    {
        handlePayload(payload);
        payload = 0;
    }
}

void DigitalDecoder::decodeBit(bool value)
{
    enum ManchesterState
    {
        LOW_PHASE_A,
        LOW_PHASE_B,
        HIGH_PHASE_A,
        HIGH_PHASE_B
    };
    
    static ManchesterState state = LOW_PHASE_A;
    
    switch(state)
    {
        case LOW_PHASE_A:
        {
            state = value ? HIGH_PHASE_B : LOW_PHASE_A;
            break;
        }
        case LOW_PHASE_B:
        {
            handleBit(false);
            state = value ? HIGH_PHASE_A : LOW_PHASE_A;
            break;
        }
        case HIGH_PHASE_A:
        {
            state = value ? HIGH_PHASE_A : LOW_PHASE_B;
            break;
        }
        case HIGH_PHASE_B:
        {
            handleBit(true);
            state = value ? HIGH_PHASE_A : LOW_PHASE_A;
            break;
        }
    }
}

void DigitalDecoder::handleData(char data)
{
    static const int samplesPerBit = 8;
    
        
    if(data != 0 && data != 1) return;
        
    const bool thisSample = (data == 1);
    
    if(thisSample == lastSample)
    {
        samplesSinceEdge++;
        
        //if(samplesSinceEdge < 100)
        //{
        //    printf("At %d for %u\n", thisSample?1:0, samplesSinceEdge);
        //}
        
        if((samplesSinceEdge % samplesPerBit) == (samplesPerBit/2))
        {
            // This Sample is a new bit
            decodeBit(thisSample);
        }
    }
    else
    {
        samplesSinceEdge = 1;
    }
    lastSample = thisSample;
}
