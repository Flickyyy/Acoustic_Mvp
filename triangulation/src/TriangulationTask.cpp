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

int time2int(const string& time) 
{
    Logger logger(".log");
    if (time.length() < 12)// Проверка длины
    {
        logger.addWriting("Time string too short", 'E');
        return 0;
    }
    else if (time[2] != ':' || time[5] != ':' || time[8] != ':')
    {   
        logger.addWriting("Invalid time format", 'E');
        return 0;
    }
    try 
    {
        int hours = (time[0]-'0')*10*60*60*1000 + (time[1]-'0')*60*60*1000;
        int min = (time[3]-'0')*10*60*1000 + (time[4]-'0')*60*1000;
        int sec = (time[6]-'0')*10*1000 + (time[7]-'0')*1000;
        int ms = (time[9]-'0')*100 + (time[10]-'0')*10 + (time[11]-'0');
        
        return hours + min + sec + ms;
    } 
    catch (...) 
    {
        logger.addWriting("Invalid characters in time", 'E');
        return 0;
    }
}

bool compareSensor(const Sensor& a, const Sensor& b)
{
    return a.mac < b.mac;
}

bool compareMessage(const SensorMessage& a, const SensorMessage& b)
{
    return a.mac < b.mac;
}

TriangulationTask::TriangulationTask(vector <Sensor> sensors, vector <SensorMessage> sensors_messages)
{ //переписать, добавить массив messages с которыми работаем, добавить массив датчиков с которыми работаем.
    int delta_time = 100; u_int64_t temp = 0; 
    vector <SensorMessage> messages;
    vector <Sensor> this_sensors;
    /*
    sort(sensors.begin(), sensors.end(), compareSensor);
    sort(sensors_messages.begin(), sensors_messages.end(), compareMessage); */
    temp = sensors_messages[sensors_messages.size()-1].timestump;
    for(auto e : sensors_messages)
    {
        if(abs( static_cast<int64_t>(e.timestump - temp)) <= delta_time)
        {
            messages.push_back(e);
        }
        if(messages.size() == 3) break;
    }
    for(auto e : messages)
    {
        for(auto k : sensors)
        {
            if(e.mac == k.mac) this_sensors.push_back(k);
        }
    }
    if(this_sensors.size() == 3 && messages.size() == 3) Triangulator triangulator(this_sensors, messages);
    else if(this_sensors.size() > 3 || messages.size() > 3) Triangulator triangulator({this_sensors[0], this_sensors[1], this_sensors[2]},{messages[0], messages[1], messages[2]});
    Logger logger(".log");
    logger.addWriting("make task", 'I');

}

void TriangulationTask::execute()
{ 
    string result = to_string(triangulator.PointDeterminate().first) + " " + to_string(triangulator.PointDeterminate().second);
    RedisPublisher publisher(host, port, publish_channel);
    publisher.publish(publish_channel, result);
    Logger logger(".log");
    logger.addWriting("Cord is: " + result, 'I');
    
}