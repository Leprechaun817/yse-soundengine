/*
  ==============================================================================

    soundLoader.cpp
    Created: 28 Jan 2014 11:49:12am
    Author:  yvan

  ==============================================================================
*/

#include "soundManager.h"
#include "soundFile.h"
#include "../headers/constants.hpp"
#include "global.h"
#include "../implementations/soundImplementation.h"
#include "../implementations/channelImplementation.h"
#include "../internal/deviceManager.h"
#include "../internal/channelManager.h"
#include "../implementations/logImplementation.h"

juce_ImplementSingleton(YSE::INTERNAL::soundManager)

YSE::INTERNAL::soundManager::soundManager() : Thread(juce::String("soundManager")) {
  formatManager.registerBasicFormats();
}

YSE::INTERNAL::soundManager::~soundManager() {
  clearSingletonInstance();
}

YSE::INTERNAL::soundFile * YSE::INTERNAL::soundManager::add(const File & file) {
  // find out if this file already exists
  for (std::forward_list<soundFile>::iterator i = soundFiles.begin(); i != soundFiles.end(); ++i) {
    if ( i->_file == file) {
      i->clients++;
      return &(*i);
    }
  }

  // if we got here, the file does not exist yet
  soundFiles.emplace_front();
  soundFile & sf = soundFiles.front();
  sf.clients++;
  if (sf.create(file)) {
    return &sf;
  }
  else {
    sf.release();
    return NULL;
  }
}

void YSE::INTERNAL::soundManager::addToQue(soundFile * elm) {
  const ScopedLock readQueLock(readQue);
  soundFilesQue.push_back(elm);
}


Bool YSE::INTERNAL::soundManager::empty() {
  if (soundObjects.empty()) return true;
  return false;
}

void YSE::INTERNAL::soundManager::run() {
  for (;;) {
    { // <- extra braces are needed to unlock readQueLock when no files are queued.
      // lock because we deque is not threadsafe
      const ScopedLock readQueLock(readQue);
      while (soundFilesQue.size()) {
        soundFile * s = (soundFile *)soundFilesQue.front();
        soundFilesQue.pop_front();

        // done with soundFilesQue for now. We can unlock.
        const ScopedUnlock readQueUnlock(readQue);

        // Now try to read the soundfile in a memory buffer
        currentAudioFileSource = nullptr;
        juce::ScopedPointer<AudioFormatReader> reader = formatManager.createReaderFor(s->_file);
        if (reader != nullptr) {
          s->_buffer.setSize(reader->numChannels, (Int)reader->lengthInSamples);
          reader->read(&s->_buffer, 0, (Int)reader->lengthInSamples, 0, true, true);

          // sample rate adjustment
          s->_sampleRateAdjustment = static_cast<Flt>(reader->sampleRate) / static_cast<Flt>(SAMPLERATE);
          s->_length = s->_buffer.getNumSamples();

          // file is ready for use now
          s->_state = YSE::INTERNAL::READY;
        }
        else {
          Global.getLog().emit(E_FILEREADER, "Unable to read " + s->_file.getFullPathName().toStdString());
        }

        // check for thread exit signal (in case multiple sounds are loaded)
        if (threadShouldExit()) {
          return;
        }
      }

      // check for thread exit signal (in case no sounds are loaded)
      if (threadShouldExit()) {
        return;
      }
    }
    // enter wait state
    wait(-1);
  }
}

void YSE::INTERNAL::soundManager::update() {
  // update soundFiles
  auto iMinus = soundFiles.before_begin();
  for (auto i = soundFiles.begin(); i != soundFiles.end(); ) {
    if (!i->inUse()) {
      const ScopedLock lock(Global.getDeviceManager().getLock());
      i = soundFiles.erase_after(iMinus);
    }
    else {
      iMinus = i;
      ++i;
    }
  }

  Int playingSounds = 0;
  // update sound objects & calculate virtual sounds
  {
    auto previous = soundObjects.before_begin();
    for (auto i = soundObjects.begin(); i != soundObjects.end(); ) {
      i->update();

      // delete sounds that are stopped and not in use any more
      if (i->intent_dsp == YSE::SS_STOPPED && i->_release) {
        const ScopedLock lock(Global.getDeviceManager().getLock());
        i = soundObjects.erase_after(previous);
        continue;
      }
      previous = i;

      // count playing sounds 
      if (i->loading_dsp
        || i->intent_dsp == YSE::SS_STOPPED
        || i->intent_dsp == YSE::SS_PAUSED) {
        ++i;
        continue;
      }
      playingSounds++;
      ++i;
    }
  }

  if (nonVirtualSize < playingSounds) {
    soundObjects.sort(soundImplementation::sortSoundObjects);
  }

  std::forward_list<soundImplementation>::iterator index = soundObjects.begin();
  for (int i = 0; i < nonVirtualSize && index != soundObjects.end(); index++, i++) {
    index->isVirtual_dsp = false;
  }
}

void YSE::INTERNAL::soundManager::maxSounds(Int value) {
  nonVirtualSize = value;
}

Int YSE::INTERNAL::soundManager::maxSounds() {
  return nonVirtualSize;
}

YSE::INTERNAL::soundImplementation * YSE::INTERNAL::soundManager::addImplementation() {
  soundObjects.emplace_front();
  return &soundObjects.front();
}

void YSE::INTERNAL::soundManager::removeImplementation(YSE::INTERNAL::soundImplementation * ptr) {
  ptr->_release = true;
  ptr->intent_dsp = SS_WANTSTOSTOP;
}

void YSE::INTERNAL::soundManager::adjustLastGainBuffer() {
  const ScopedLock lock(Global.getDeviceManager().getLock());

  for (std::forward_list<soundImplementation>::iterator i = soundObjects.begin(); i != soundObjects.end(); ++i) {
    // if a sound is still loading, it will be adjusted during initialize
    if (i->loading_dsp) continue;

    UInt j = i->lastGain_dsp.size(); // need to store previous size for deep resize
    i->lastGain_dsp.resize(Global.getChannelManager().getNumberOfOutputs());
    for (; j < i->lastGain_dsp.size(); j++) {
      i->lastGain_dsp[j].resize(i->buffer_dsp->size(), 0.0f);
    }
  }
}