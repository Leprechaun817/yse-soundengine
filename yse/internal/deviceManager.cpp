/*
  ==============================================================================

    deviceManager.cpp
    Created: 27 Jan 2014 8:04:27pm
    Author:  yvan

  ==============================================================================
*/

#include "deviceManager.h"
#include "global.h"
#include "JuceHeader.h"
#include "../headers/constants.hpp"
#include "../internal/soundManager.h"
#include "../implementations/channelImplementation.h"
#include "../implementations/logImplementation.h"

juce_ImplementSingleton(YSE::INTERNAL::deviceManager)

YSE::INTERNAL::deviceManager::deviceManager() : initialized(false), open(false),
                                      started(false), mainChannel(NULL), 
                                      bufferPos(STANDARD_BUFFERSIZE) {

}

YSE::INTERNAL::deviceManager::~deviceManager() {
  close();
  clearSingletonInstance();
}

Bool YSE::INTERNAL::deviceManager::init() {
  if (!initialized) {
    _lastError = audioDeviceManager.initialise(0, 2, NULL, true);
    if (_lastError.isNotEmpty()) {
      //Error.emit(E_AUDIODEVICE, _lastError.toStdString());
      jassertfalse
      return false;
    }
    initialized = true;

    if (!open) {
      audioDeviceManager.addAudioCallback(this);
      open = true;
    }  
  }
  return true;
}

void YSE::INTERNAL::deviceManager::close() {
  if (open) {
    audioDeviceManager.closeAudioDevice();
    open = false;
  }
}

Flt YSE::INTERNAL::deviceManager::cpuLoad() {
  return static_cast<Flt>(audioDeviceManager.getCpuUsage());
}

void YSE::INTERNAL::deviceManager::setChannel(channelImplementation * ptr) {
  mainChannel = ptr;
}

void YSE::INTERNAL::deviceManager::audioDeviceIOCallback(const float ** inputChannelData,
  int      numInputChannels,
  float ** outputChannelData,
  int      numOutputChannels,
  int      numSamples) {
  
  if (mainChannel == NULL) return;
  if (Global.getSoundManager().empty()) return;

  // global audio lock
  const ScopedLock lock(audioDeviceManager.getAudioCallbackLock());

  UInt pos = 0;

  while (pos < static_cast<UInt>(numSamples)) {
    if (bufferPos == STANDARD_BUFFERSIZE) {
      mainChannel->dsp();
      mainChannel->buffersToParent();
      bufferPos = 0;
    }

    UInt size = (numSamples - pos) > (STANDARD_BUFFERSIZE - bufferPos) ? (STANDARD_BUFFERSIZE - bufferPos) : ((UInt)numSamples - pos);
    for (UInt i = 0; i < mainChannel->out.size(); i++) {
      // this is not really a safe way to work with buffers, but it won't give any errors in here
      UInt l = size;
      Flt * ptr1 = ((Flt **)outputChannelData)[i] + pos;
      Flt * ptr2 = mainChannel->out[i].getBuffer() + bufferPos;
      for (; l > 7; l -= 8, ptr1 += 8, ptr2 += 8) {
        ptr1[0] = ptr2[0];
        ptr1[1] = ptr2[1];
        ptr1[2] = ptr2[2];
        ptr1[3] = ptr2[3];
        ptr1[4] = ptr2[4];
        ptr1[5] = ptr2[5];
        ptr1[6] = ptr2[6];
        ptr1[7] = ptr2[7];

      }
      while (l--) *ptr1++ = *ptr2++;
    }
    bufferPos += size;
    pos += size;
  }
}

void YSE::INTERNAL::deviceManager::audioDeviceAboutToStart(AudioIODevice * device) {

}

void YSE::INTERNAL::deviceManager::audioDeviceStopped() {

}

void YSE::INTERNAL::deviceManager::audioDeviceError(const juce::String & errorMessage) {
  Global.getLog().emit(E_AUDIODEVICE, errorMessage);
}

CriticalSection & YSE::INTERNAL::deviceManager::getLock() {
  return audioDeviceManager.getAudioCallbackLock();
}