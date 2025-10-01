#pragma once
#ifndef TRIANGULATION_SERVICE_H
#define TRIANGULATION_SERVICE_H

#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <complex>
#include <cmath>
#include <fstream>
#include <utility>
#include <valarray>

#include <hiredis/hiredis.h>
#include <nlohmann/json.hpp>

#include "ThreadPool.h"
#include "RedisSubscriber.h"
#include "RedisPublisher.h"
#include "Triangulator.h"
#include "Sensor.h"
#include "SensorMessage.h"
#include "Logger.h"
#include "config.h"

using namespace std;
using namespace cfg;

class TriangulationService
{
private:
    RedisSubscriber updater, listener;
    ThreadPool task_pool;
    Logger logger;

    vector<SensorMessage> sensors_messages;
    vector<Sensor> sensor_list;

    atomic<bool> running{true};

public:
    TriangulationService();

    void start();
    void stop();
};

#endif //TRIANGULATION_SERVICE_H