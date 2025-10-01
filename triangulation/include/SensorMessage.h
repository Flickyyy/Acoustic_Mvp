#pragma once
#ifndef SENSORSMESSAGE_H
#define SENSORSMESSAGE_H

#include <iostream>
#include <vector>
#include <string>

using namespace std;

struct SensorMessage
{
    string mac;
    u_int64_t timestump;
    vector <int16_t> pcm_sound;

    SensorMessage () {};
};

#endif //SENSORSMESSAGE_H