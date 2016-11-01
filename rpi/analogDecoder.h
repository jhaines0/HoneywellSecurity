#ifndef __ANALOG_DECODER_H__
#define __ANALOG_DECODER_H__

#include <functional>

class AnalogDecoder
{
  public:
    AnalogDecoder() = default;
    
    void handleMagnitude(float value);
    void setCallback(std::function<void(char)> cb) {m_cb = cb;};
    
  private:
    std::function<void(char)> m_cb;
    
    int m_discardedSamples = 0;
    float m_ookMax = 0.0;
    float m_val = 0.0;
};

#endif
