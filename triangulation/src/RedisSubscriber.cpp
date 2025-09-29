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

vector<string> csplit(string s, char element) {
    vector<string> str_hex;
    
    for(int i = 0; i < s.size(); i++) {
        if(s[i] == element && i + 3 < s.size() && s[i+1] == 'x') {
            // Нашли \x, теперь извлекаем два HEX символа
            string hex_byte;
            hex_byte += s[i+2];  // первая цифра
            hex_byte += s[i+3];  // вторая цифра
            str_hex.push_back(hex_byte);
            i += 3;  // пропускаем \x и две цифры
        }
    }
    return str_hex;
}

vector <int16_t> hex_to_dec(vector <string> a)
{
    vector <int16_t> result;

    for(int i=0; i<a.size(); i+=2)
    {
        int temp = 0;
        for(int j=0; j<a[i].size(); j++)
        {
            if(a[i+1][j] == 'A') temp += 10*pow(16, 2*a[i].size()-j-1);
            else if(a[i+1][j] == 'B') temp += 11*pow(16, 2*a[i].size()-j-1);
            else if(a[i+1][j] == 'C') temp += 12*pow(16, 2*a[i].size()-j-1);
            else if(a[i+1][j] == 'D') temp += 13*pow(16, 2*a[i].size()-j-1);
            else if(a[i+1][j] == 'E') temp += 14*pow(16, 2*a[i].size()-j-1);
            else if(a[i+1][j] == 'F') temp += 15*pow(16, 2*a[i].size()-j-1);
            else temp += (a[i+1][j] - '0')*pow(16, 2*a[i].size() - j -1);

            if(a[i][j] == 'A') temp += 10*pow(16, a[i].size()-j-1);
            else if(a[i][j] == 'B') temp += 11*pow(16, a[i].size()-j-1);
            else if(a[i][j] == 'C') temp += 12*pow(16, a[i].size()-j-1);
            else if(a[i][j] == 'D') temp += 13*pow(16, a[i].size()-j-1);
            else if(a[i][j] == 'E') temp += 14*pow(16, a[i].size()-j-1);
            else if(a[i][j] == 'F') temp += 15*pow(16, a[i].size()-j-1);
            else temp += (a[i][j] - '0')*pow(16, a[i].size()-j-1);
        }
        result.push_back(temp);
    }
    return result;
}

RedisSubscriber::RedisSubscriber(string host, int port)
{
    Logger logger(".log");
    context = redisConnect(host.c_str(), port);
    if(context == nullptr || (*context).err ) logger.addWriting("Ошибка подключения", 'E');               
    else logger.addWriting("Вы подключились к каналу ", 'I');
}

void RedisSubscriber::subscribe(string topic)
{
    Logger logger(".log");
    string channel_name = "SUBSCRIBE " + topic;
    redisReply* channel_reply = (redisReply*) redisCommand(context, channel_name.c_str());
    if(channel_reply == nullptr) logger.addWriting("Ошибка подписки", 'E');
    else logger.addWriting("Вы подписались на topic " + topic, 'I');
    freeReplyObject(channel_reply);
}

vector<Sensor> RedisSubscriber::updateTopics(RedisSubscriber &subscriber)
{
    Logger logger(".log");
    string message_str = "";
    redisReply* message_repl = nullptr;
    
    // Получаем ответ от Redis
    if(redisGetReply(context, (void**)&message_repl) != REDIS_OK) 
    {
        logger.addWriting("Failed to get Redis reply", 'E');
        if (message_repl) freeReplyObject(message_repl);
        return vector<Sensor>();
    }

    // Обрабатываем ответ Redis
    if (message_repl == nullptr) {
        logger.addWriting("Null Redis reply", 'E');
        return vector<Sensor>();
    }

    if (message_repl->type == REDIS_REPLY_ARRAY && message_repl->elements == 3) {
        // Безопасно извлекаем данные из массива
        if (message_repl->element[2] && 
            message_repl->element[2]->type == REDIS_REPLY_STRING && 
            message_repl->element[2]->str) {
            message_str = string(message_repl->element[2]->str);
        } else {
            logger.addWriting("Invalid message element in Redis reply", 'E');
            freeReplyObject(message_repl);
            return vector<Sensor>();
        }
    } else {
        logger.addWriting("Invalid Redis reply format", 'E');
        freeReplyObject(message_repl);
        return vector<Sensor>();
    }
    
    freeReplyObject(message_repl);

    // Проверяем, что получили непустое сообщение
    if (message_str.empty()) {
        logger.addWriting("Empty message string from Redis", 'E');
        return vector<Sensor>();
    }

    vector<Sensor> sensors;
    
    try {
        // Парсим JSON
        json data = json::parse(message_str);
        
        // Проверяем наличие обязательного поля sensors
        if (!data.contains("sensors") || !data["sensors"].is_array()) {
            logger.addWriting("Missing or invalid 'sensors' field in JSON", 'E');
            return vector<Sensor>();
        }

        // Обрабатываем каждый сенсор
        for (const auto& e : data["sensors"]) {
            // Проверяем обязательные поля
            if (!e.contains("mac") || !e.contains("x") || !e.contains("y")) {
                logger.addWriting("Missing required fields in sensor JSON", 'E');
                continue; // Пропускаем некорректный сенсор, но продолжаем обработку
            }

            // Проверяем типы данных
            if (!e["mac"].is_string() || !e["x"].is_number() || !e["y"].is_number()) {
                logger.addWriting("Invalid data types in sensor JSON", 'E');
                continue;
            }

            string mac = e["mac"];
            
            // Добавляем в topics если еще нет
            if (find(topics.begin(), topics.end(), mac) == topics.end()) {
                topics.push_back(mac);
            }

            // Создаем сенсор
            Sensor temp;
            temp.mac = mac;
            temp.x = e["x"];
            temp.y = e["y"];
            sensors.push_back(temp);
        }

        // Подписываемся на topics
        for (const auto& topic : topics) {
            try {
                subscriber.subscribe(topic);
            } catch (const exception& sub_err) {
                logger.addWriting("Failed to subscribe to topic: " + topic + " - " + string(sub_err.what()), 'E');
            }
        }

        return sensors;

    } catch (const json::exception& err) {
        logger.addWriting("JSON parse error: " + string(err.what()), 'E');
        return vector<Sensor>();
    } catch (const exception& err) {
        logger.addWriting("Unexpected error in updateTopics: " + string(err.what()), 'E');
        return vector<Sensor>();
    }
}

SensorMessage RedisSubscriber::sensor_listen()
{
    Logger logger1(".log");
    string message_str = "", topic_name = "";
    redisReply* message_repl = nullptr;
    if(redisGetReply(context, (void**)&message_repl) == REDIS_OK) 
    {
        if ((*message_repl).type == REDIS_REPLY_ARRAY && (*message_repl).elements == 3)
        {
            string action = (*(*message_repl).element[0]).str;
            topic_name = (*(*message_repl).element[1]).str;
            message_str = (*(*message_repl).element[2]).str;
        }
        freeReplyObject(message_repl);
    }
    
    try
    {
        json data = json::parse(message_str);
        SensorMessage message;
        message.mac = topic_name;
        message.timestump = data["timestump"];
        message.pcm_sound = hex_to_dec(data["pcm_data"]);
        return message;
    }
    catch(js_err& err)
    {
        logger1.addWriting("error receiving message", 'E');
        return SensorMessage();
    }           
}

