/*
  ==============================================================================

    global.h
    Created: 27 Jan 2014 10:18:27pm
    Author:  yvan

  ==============================================================================
*/

#ifndef GLOBAL_H_INCLUDED
#define GLOBAL_H_INCLUDED
#include "../classes.hpp"
#include "JuceHeader.h"
#include "../headers/types.hpp"

namespace YSE {
  namespace INTERNAL {

    class global {
    public:
      deviceManager  & getDeviceManager();
      soundManager   & getSoundManager();
      channelManager & getChannelManager();
      reverbManager  & getReverbManager();

      logImplementation & getLog();
      time              & getTime();
      settings          & getSettings();

      listenerImplementation & getListener();

      void addSlowJob(ThreadPoolJob * job);
      void addFastJob(ThreadPoolJob * job);
      void waitForSlowJob(ThreadPoolJob * job);
      void waitForFastJob(ThreadPoolJob * job);
      bool containsSlowJob(ThreadPoolJob * job);
      
      void flagForUpdate() { update++;  }
      bool needsUpdate() { return update > 0;  }
      void updateDone() { update--; }

      global();

    private:
      void init();
      void close();

      deviceManager * dm;
      soundManager  * sm;
      logImplementation * log;
      time * ysetime;
      settings * set;
      channelManager * cm;
      listenerImplementation * li;
      reverbManager * rm;

      ThreadPool slowThreads;
      ThreadPool fastThreads;

      aInt update;

      friend class system; // system needs access to the init and close method
    };

    extern global Global;
  }
}



#endif  // GLOBAL_H_INCLUDED