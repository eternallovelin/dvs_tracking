#ifndef PARTICLE_H
#define PARTICLE_H

class Particle
{
public:
    Particle(float posX = 0, float posY = 0, float uncert = 10000.0f, int w = 0, double ts = 0);
    void set(float posX, float posY, float uncert, int w, double ts);
    void reset();

    float x,y;
    float uncertainty;
    int weight;
    double timeStamp;
};

#endif // PARTICLE_H
