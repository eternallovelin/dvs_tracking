#include "frequencyaccumulator.h"

const float FrequencyAccumulator::PI = 3.14159265f;

#define N_GUESSES 16

FrequencyAccumulator::FrequencyAccumulator(
        int frequency, float sigma, int filterSize,
        float filterSigma, float minDist, int numMaxima, int w, int h)
{
    // Weights
    weightMap = new Map<int>(w,h);
    for(int i = 0; i < weightMap->size();i++)
        weightMap->set(i,0);

    maxima = new Maxima(numMaxima,minDist);

    targetFrequency = frequency;
    sd = sigma;
    lastReset = 0;
    lastUpdate = 0;

    //Maxima search
    minDistance = minDist;
    nMaxima = numMaxima;

    // Smoohing filter
    filter = new Filter(filterSize,filterSigma, w, h);
}

FrequencyAccumulator::~FrequencyAccumulator(){    
    delete weightMap;
    delete maxima;
    delete filter;
}

void FrequencyAccumulator::update(Interval interval){
    lastUpdate = interval.timeStamp;
    int x = interval.x;
    int y = interval.y;
    int prevWeight = weightMap->get(x,y);
    int weight = getWeight(interval.deltaT,targetFrequency,sd) + prevWeight;

//    printf("dt: %f, w: %f\n",interval.deltaT,weight);

    weightMap->insert(x,y,weight);
}

int FrequencyAccumulator::getWeight(double interval, int frequency, float sd){
    double targetInterval = 1.0/frequency;
    double tDiff = targetInterval - interval;
    int weight = int(1.0/(sd*sqrt(2*PI)) * exp(-pow(tDiff/sd,2.0))/2.0);
    return weight;
}

bool FrequencyAccumulator::hasExpired(){
    if( (lastUpdate - lastReset) > (1.0/targetFrequency) )
        return true;
    else
        return false;
}

void FrequencyAccumulator::reset(){
    // reset values
    for(int i = 0; i < weightMap->size();i++)
        weightMap->set(i,0);
    maxima->reset();
    lastReset = lastUpdate;
}

Maxima* FrequencyAccumulator::findMaxima(){
    // search for maxima in the map, use minDistance!
    for(int h = 0; h < weightMap->height; h++){
        for(int w = 0; w < weightMap->width; w++){
            int weight = weightMap->get(w,h);
            maxima->update(w,h,weight);
        }
    }
    return maxima;
}