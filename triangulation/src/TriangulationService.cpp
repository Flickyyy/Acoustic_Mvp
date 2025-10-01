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

void sensor_output(vector <Sensor> a)
{
    for(auto e : a) cout << e.mac << "\n";
}

TriangulationService::TriangulationService() : 
    updater("localhost", 6379),    // отдельный контекст для апдейтов
    listener("localhost", 6379),   // отдельный контекст для прослушивания  
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

bool contain(vector <Sensor> a, Sensor k)
{
    for(auto e : a)
    {
        if(e.mac == k.mac) return true;
    }
    return false;
}

void TriangulationService::start()
{
    atomic <bool> running{true};
    mutex mtx;

    
    // прослушивание датчиков
    thread listen_thread([&]()
    {
        while (running) 
        {
            SensorMessage message = listener.sensor_listen();
            {
                lock_guard<mutex> lock(mtx);
                sensors_messages.push_back(message);
            }
            this_thread::sleep_for(chrono::milliseconds(5));
        }
    });
    
    //обновление списка датчиков, обновление топиков
    thread update_thread([&]()
    {
        while (running)
        {
            vector<Sensor> updated_list = updater.updateTopics();
            {
                lock_guard<mutex> lock(mtx);
                for(int i=0; i<updated_list.size(); i++)
                {
                    if(!contain(sensor_list, updated_list[i])) sensor_list.push_back(updated_list[i]); 
                }
            }
            this_thread::sleep_for(chrono::milliseconds(5));
        }
    });
    
    //Заполнение пула задач
    while (running) {
        try {
            vector<Sensor> current_sensors;
            vector <SensorMessage> current_messages;
            
            {
                lock_guard<mutex> lock(mtx);
                current_sensors = sensor_list;
                current_messages = sensors_messages;
            }
            if(current_messages.size()>=3 and current_messages[0].mac != current_messages[current_messages.size()-1].mac) // <---- исправить
            {
                shared_ptr<TriangulationTask> task = make_shared<TriangulationTask>(current_sensors, current_messages);
                task_pool.addTask(task);
            }
            else continue;
            
            this_thread::sleep_for(chrono::milliseconds(10));
            
        } catch (const exception& e) {
            cerr << "Main thread error: " << e.what() << endl;
        }
    }

}


void TriangulationService::stop()
{
    running = false;
    task_pool.finishAllThreads();

    Logger logger(".log");
    logger.addWriting("TriangulationService stopped successfully", 'I');
    //?? а что тут писать??
} 