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

TriangulationService::TriangulationService() : 
    listener(localhost, port),
    updater(localhost, port),
    task_pool(20),
    logger(".log")
{
    updater.subscribe(update_channel);
    sensor_list = updater.updateTopics(listener);
    logger.addWriting("Triangulation service start successfully", 'I');
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
            vector<Sensor> current_sensors;
            {
                lock_guard<mutex> lock(mtx);
                current_sensors = sensor_list;
            }
            vector<Sensor> updated_list = updater.updateTopics(listener);
            {
                lock_guard<mutex> lock(mtx);
                sensor_list = updated_list;
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