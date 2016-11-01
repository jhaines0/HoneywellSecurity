#include "analogDecoder.h"

#include <cmath>
#include <algorithm>
#include <iostream>

#define HW_RATIO 17

#define MIN_OOK_THRESHOLD 0.25f
#define OOK_THRESHOLD_RATIO 0.75f
#define OOK_DECAY_PER_SAMPLE 0.0001f

#define FILTER_ALPHA 0.7


void AnalogDecoder::handleMagnitude(float val)
{
    //
    // Smooth
    //
    m_val = (FILTER_ALPHA)*m_val + (1.0 - FILTER_ALPHA)*val;
    val = m_val;
    
    //
    // 1 of N
    //
    if(m_discardedSamples < (HW_RATIO-1))
    {
        m_discardedSamples++;
        return;
    }
    
    m_discardedSamples = 0;
    
    //
    // Saturate
    //
    val = std::min(val, 1.0f);

    //
    // Threshold
    //
    m_ookMax -= OOK_DECAY_PER_SAMPLE;
    m_ookMax = std::max(m_ookMax, val);
    m_ookMax = std::max(m_ookMax, MIN_OOK_THRESHOLD/OOK_THRESHOLD_RATIO);

    //
    // Send to digital stage
    //
    int digital;
    if(m_cb)
    {
        if(val > m_ookMax*OOK_THRESHOLD_RATIO)
        {
            digital = 1;
            m_cb(1);
        }
        else
        {
            digital = 0;
            m_cb(0);
        }
    }
}
