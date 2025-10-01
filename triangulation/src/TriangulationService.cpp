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

#include "../include/TriangulationService.h"
#include "../include/ThreadPool.h"
#include "../include/RedisSubscriber.h"
#include "../include/RedisPublisher.h"
#include "../include/Triangulator.h"
#include "../include/Sensor.h"
#include "../include/Logger.h"
#include "../include/config.h"

using namespace std;
using namespace cfg;

void message_output(vector <SensorMessage> a)
{
    for(auto e : a) cout << e.timestump << "\n";
}

bool contain(vector <Sensor> a, Sensor k)
{
    for(auto e : a)
    {
        if(e.mac == k.mac) return true;
    }
    return false;
}

bool contains_mac(vector <SensorMessage> messages, string mac)
{
    for(int i=0; i<messages.size(); i++) 
    {
        if(messages[i].mac == mac ) return true;
    }
    return false;
}

vector <SensorMessage> worked_messages(vector <SensorMessage> messages)
{
    vector <SensorMessage> result;
    result.push_back(messages[messages.size()-1]);
    for(int i=messages.size()-2; i>0; i--)
    {
        if(!contains_mac(result, messages[i].mac) &&
           abs(static_cast<double>(result[0].timestump - messages[i].timestump)) < 70)
        {
            result.push_back(messages[i]);
        }

        if(result.size() == 3) return result;
    }
    return vector <SensorMessage> ();
}

vector <Sensor> curr_sens(vector <SensorMessage>  messages, vector <Sensor> sensors)
{
    vector <Sensor> result;
    for(auto e : sensors)
    {
        if(contains_mac(messages, e.mac)) result.push_back(e);
        if(result.size() == 3) return result;
    }
    return vector <Sensor> ();
}

TriangulationService::TriangulationService() : 
    updater(host, port), 
    listener(host, port), 
    task_pool(20),
    logger(".log")
{
    updater.subscribe("update_sensors");
    sensor_list = updater.updateTopics();
    
    for (const auto& sensor : sensor_list) {
        listener.subscribe(sensor.mac);
    }
    
    logger.addWriting("Triangulation service start successfully", 'I');
}

void TriangulationService::start()
{
    running = true;
    mutex mtx;

    thread listen_thread([this, &mtx]()  // Захватываем this
    {
        while (this->running)  
        {
            SensorMessage message = this->listener.sensor_listen();
            {
                lock_guard<mutex> lock(mtx);
                this->sensors_messages.push_back(message);
            }
            for(auto e : this->sensors_messages) cout << e.timestump << " - time\n";
            this_thread::sleep_for(chrono::milliseconds(5));
        }
    });
    
    thread update_thread([this, &mtx]()  // Захватываем this
    {
        while (this->running)  
        {
            vector<Sensor> updated_list = this->updater.updateTopics();
            {
                lock_guard<mutex> lock(mtx);
                for(int i=0; i<updated_list.size(); i++)
                {
                    if(!contain(this->sensor_list, updated_list[i])) this->sensor_list.push_back(updated_list[i]); 
                }
            }
            this_thread::sleep_for(chrono::milliseconds(5));
        }
    });
    
    
    //Заполнение пула задач
    while (this->running) {
        try 
        {
            vector<Sensor> current_sensors;
            vector<SensorMessage> current_messages;
            
            if(this->sensor_list.size() != 0 && 
               this->sensors_messages.size() != 0)
            {
                {
                    lock_guard<mutex> lock(mtx);
                    current_messages = worked_messages(this->sensors_messages);
                    current_sensors = curr_sens(current_messages, this->sensor_list);
                }
                
                if (current_messages.size() == 3 && current_sensors.size() == 3) {
                    shared_ptr<TriangulationTask> task = make_shared<TriangulationTask>(current_sensors, current_messages);
                    this->task_pool.addTask(task);
                    this->sensors_messages = vector <SensorMessage> ();
                    this->sensor_list = vector <Sensor> ();
                }
            }
            
            
            this_thread::sleep_for(chrono::milliseconds(10));
            
        } 
        catch (const exception& e) 
        {
            cerr << "Main thread error: " << e.what() << endl;
        }
    }
    

    listen_thread.join();
    update_thread.join();
}


void TriangulationService::stop()
{
    running = false;
    task_pool.finishAllThreads();

    Logger logger(".log");
    logger.addWriting("TriangulationService stopped successfully", 'I');
} 