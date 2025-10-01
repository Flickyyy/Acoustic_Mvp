#include <iostream>
#include <string>
#include <set>
#include <nlohmann/json.hpp>
#include <hiredis/hiredis.h>

#include "../include/RedisSubscriber.h"
#include "../include/SensorMessage.h"

using namespace std;
using json = nlohmann::json;
using js_err = json::parse_error;

template <typename T>
void output(vector <T> a)
{
    for (int i=0; i<a.size(); i++) cout << a[i] << " ";
    cout << "\n";
}

RedisSubscriber::RedisSubscriber(string host, int port) 
{
    lock_guard<mutex> lock(context_mutex);
    Logger logger(".log");
    context = redisConnect(host.c_str(), port);
    if(context == nullptr || (*context).err ) logger.addWriting("Ошибка подключения", 'E');               
    else logger.addWriting("Вы подключились к каналу ", 'I');
}

void RedisSubscriber::subscribe(string topic)
{
    lock_guard<mutex> lock(context_mutex);
    Logger logger(".log");
    string channel_name = "SUBSCRIBE " + topic;
    redisReply* channel_reply = (redisReply*) redisCommand(context, channel_name.c_str());
    if(channel_reply == nullptr) logger.addWriting("Ошибка подписки", 'E');
    else logger.addWriting("Вы подписались на topic " + topic, 'I');
    freeReplyObject(channel_reply);
}

vector <Sensor> RedisSubscriber::updateTopics()
{
    Logger logger(".log");
    string message_str = "";
    redisReply* message_repl = nullptr;
    
    if(redisGetReply(context, (void**)&message_repl) == REDIS_OK) 
    {
        if (message_repl && 
            (*message_repl).type == REDIS_REPLY_ARRAY && 
            (*message_repl).elements == 3)
        {
            string topic_name = "";
            if ((*message_repl).element[0] && (*(*message_repl).element[0]).str)
            {
                string action = (*(*message_repl).element[0]).str;
            }
            if ((*message_repl).element[1] && (*(*message_repl).element[1]).str) 
            {
                topic_name = (*(*message_repl).element[1]).str;
            }
            if ((*message_repl).element[2] && (*(*message_repl).element[2]).str) 
            {
                message_str = (*(*message_repl).element[2]).str;
            }
            if (topic_name != "update_sensors") 
            {
                freeReplyObject(message_repl);
                return vector<Sensor>(); 
            }
        }
        else logger.addWriting("error redis remote", 'E');

        freeReplyObject(message_repl);
    }
    try
    {
        json data = json::parse(message_str);
        vector <Sensor> sensors;
        topics = vector <string> ();
        if(data.contains("sensors"))
        {
            for(auto e : data["sensors"]) 
            {
                topics.push_back(e["mac"]);
                Sensor temp;
                temp.mac = e["mac"];
                temp.x = e["x"]; temp.y = e["y"];
                sensors.push_back(temp);
            }
        }
        else
        {
            topics.push_back(data["mac"]);
            Sensor temp;
            temp.mac = data["mac"];
            temp.x = data["x"]; temp.y = data["y"];
            sensors.push_back(temp);
        }
        for(auto e : topics)
        {
            (*this).subscribe(e);
        } 
        return sensors;
    }
    catch(js_err& err)
    {
        return vector <Sensor>();
    }
    
}

SensorMessage RedisSubscriber::sensor_listen()
{
    
    Logger logger1(".log");
    string message_str = "", topic_name = "";
    redisReply* message_repl = nullptr;
    
    if(redisGetReply(context, (void**)&message_repl) == REDIS_OK) 
    {
        if (message_repl && 
            message_repl->type == REDIS_REPLY_ARRAY && 
            message_repl->elements == 3)
        {

            if (message_repl->element[1] && message_repl->element[1]->str) {
                topic_name = message_repl->element[1]->str;
            }

            if (message_repl->element[2] && message_repl->element[2]->str) {
                message_str = message_repl->element[2]->str;
            }

            if (topic_name == "update_sensors") 
            {
                freeReplyObject(message_repl);
                return SensorMessage();
            }
        }
        freeReplyObject(message_repl);
    }
    try
    {
        json data = json::parse(message_str);
        SensorMessage message;
        message.mac = topic_name;
        message.timestump = data["time"];
        vector<int16_t> temp; 
        for(auto e : data["data"]) temp.push_back(e);
        message.pcm_sound = temp;
        return message;
    }
    catch(const json::exception& err)
    {
        logger1.addWriting("error receiving message: " + string(err.what()), 'E');
        return SensorMessage();
    }           
}

