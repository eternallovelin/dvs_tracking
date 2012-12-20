#include "tracker.h"
#include "stdio.h"
#include "maxima.h"
#include "localmaximum.h"
#include <math.h>

#define DVS_RES 128

//parameteres
#define SIGMA_W 0.0002f
#define FILTER_SIZE 3
#define SIGMA_FILTER 0.75f
#define MIN_DIST 16.0f
#define NUM_MAXIMA 3

Tracker::Tracker(RingBuffer<Event> *buffer, std::vector<int> frequencies, QObject *parent) : QThread(parent){
    //Members
    eventBuffer = buffer;
    targetFrequencies = frequencies;

    widget = 0;

    exit = false;

    // init maps
    latestEvents = new Map<Event>(DVS_RES,DVS_RES);
    npTransitions = new Map<Transition>(DVS_RES,DVS_RES);
    pnTransitions = new Map<Transition>(DVS_RES,DVS_RES);

    weightBuffers = new FrequencyAccumulator*[targetFrequencies.size()];

    for(unsigned int i = 0; i < targetFrequencies.size();i++){
        weightBuffers[i] = new FrequencyAccumulator(
                    targetFrequencies[i],SIGMA_W,FILTER_SIZE,SIGMA_FILTER,MIN_DIST,NUM_MAXIMA,DVS_RES,DVS_RES);
    }

    //debugging
    logger = new HypothesisLogger("C:/Users/giselher/Documents/uzh/hypo_log.txt");
    lastEventTs = 0;
    eventCount = 0;
}

Tracker::~Tracker(){
    delete latestEvents;
    delete npTransitions;
    delete pnTransitions;

    for(unsigned int i = 0; i < targetFrequencies.size();i++){
        delete weightBuffers[i];
    }
    delete [] weightBuffers;

    delete logger;
}

void Tracker::processEvent(Event e){
    // Record, if there is a transition
    Transition t = getTransition(e);
    if(t.timeStamp == 0){
        return;
    }

    // Get interval to last transition
    Interval dt = getInterval(t);
    if(dt.timeStamp == 0 || dt.deltaT < 0){
        return;
    }

    //stop logger
    if(lastEventTs > e.timeStamp && !logger->done()){
        printf("#Events(tra): %d\n",eventCount);
        eventCount = 0;
//        logger->stop();
    }
    else
        eventCount++;

    //Calculate importance of interval for each frequency
    for(unsigned int i = 0; i < targetFrequencies.size(); i++){
        FrequencyAccumulator *buf = weightBuffers[i];
        buf->update(dt);

        Maxima *maxima = 0;
        if(buf->hasExpired()){
            maxima = buf->findMaxima();
            //process maxima HERE
//            if(!logger->done()){
//                for(int j = 0; j < maxima->size();j++){
//                    if(maxima->get(j)->weight != 0)
//                        logger->log(e.timeStamp,targetFrequencies[i],maxima->size(),
//                                    j,maxima->get(j)->x,maxima->get(j)->y,maxima->get(j)->weight);
//                }
//            }
            updateWeightWidget(i,buf,maxima);
            buf->reset();
        }
    }
    lastEventTs = e.timeStamp;
}

Transition Tracker::getTransition(Event e){
    //Get last event at same position and overwrite with new event.
    Event last = latestEvents->get(e.x,e.y);
    latestEvents->insert(e.x,e.y,e);
    if(last.timeStamp == 0)
        return Transition(0);
    //If consecutive events differ in type, create and return a transition
    if(last.type != e.type)
        return Transition(e.timeStamp,e.x,e.y,e.type);
    else
        return Transition(0);
}

Interval Tracker::getInterval(Transition t){
    Map<Transition> *transitions = (t.type == 1) ? npTransitions : pnTransitions;
    Transition last = transitions->get(t.x,t.y);
    transitions->insert(t.x,t.y,t);

    if(last.timeStamp == 0)
        return Interval(0);
    else{
        double deltaT = t.timeStamp - last.timeStamp;
        return Interval(t.timeStamp,t.x,t.y,deltaT);
    }
}

void Tracker::stop(){
    exit = true;
}

void Tracker::setWidget(CamWidget *camWidget){
    widget = camWidget;
}

void Tracker::updateCamWidget(Event *e){
    if(widget == 0)
        return;
    widget->updateImage(e);
}

void Tracker::updateWeightWidget(int bufID, FrequencyAccumulator *buf, Maxima *m){
    for(int y = 0; y < DVS_RES;y++){
        for(int x = 0; x < DVS_RES;x++){
            float value = buf->weightMap->get(x,y);
            if(value > 0){
                int grey = int(value/4.0);
                if(grey > 255)
                    grey = 255;
                if(grey > 0){
                    widget->updateImage(x,y,grey);
                }
            }
        }
    }
    //    for(int i = 0; i < m->size();i++){
    //        if(m->get(i)->weight == 0)
    //            continue;
    //        int x = m->get(i)->x;
    //        int y = m->get(i)->y;
    //        int w = m->get(i)->weight;
    //        widget->updateImage(x,y,w,bufID);
    //    }
}

void Tracker::run(){
    while(!exit){
        if((eventBuffer->available()) > 0){

            //updateCamWidget(eventBuffer->latestIndex(),eventBuffer->available());

            Event *e;
            while((e = eventBuffer->getNext()) != 0){
                // do not process if special event
                if(e->isSpecial())
                    return;

                //process events here
                //updateCamWidget(e);
                processEvent(*e);
            }
        }
        else
            msleep(1);
    }
}