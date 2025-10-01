#include <iostream>
#include <cmath>
#include <tuple>

#include "../include/TriangulationTask.h"
#include "../include/RedisSubscriber.h"
#include "../include/RedisPublisher.h"
#include "../include/SensorMessage.h"
#include "../include/Sensor.h"
#include "../include/config.h"

using namespace std;
using namespace cfg;

bool compareSensor(const Sensor& a, const Sensor& b)
{
    return a.mac < b.mac;
}

bool compareMessage(const SensorMessage& a, const SensorMessage& b)
{
    return a.mac < b.mac;
}

TriangulationTask::TriangulationTask(vector <Sensor> sensors, vector <SensorMessage> sensors_messages)
    : triangulator(sensors, sensors_messages)
{
    Logger logger(".log");
    logger.addWriting("Успешно создана задача", 'I');
}

void TriangulationTask::execute()
{ 
    pair<double, double> coordinates = triangulator.PointDeterminate();
    string result = to_string(coordinates.first) + " " + to_string(coordinates.second);
    RedisPublisher publisher(host, port, publish_channel);
    publisher.publish(publish_channel, result);
    Logger logger(".log");
    logger.addWriting("Cord is: " + result, 'I');
}