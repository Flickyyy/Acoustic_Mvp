#pragma once
#ifndef REDIS_SUBSCRIBER_H
#define REDIS_SUBSCRIBER_H

#include <iostream>
#include <mutex>
#include <set>
#include <vector>
#include <hiredis/hiredis.h>

#include "Logger.h"
#include "Sensor.h"
#include "SensorMessage.h"

using namespace std;

class RedisSubscriber
{
    private:
        redisContext* context;
        vector <string> topics;
        mutex context_mutex;
    public:

        RedisSubscriber() {}
        RedisSubscriber(string host, int port);
        ~RedisSubscriber() {
            redisFree(context);
        }
        SensorMessage sensor_listen();
        void subscribe(string topic);
        vector <Sensor> updateTopics();
};

#endif //REDIS_SUBSCRIBER_H