#pragma once
#ifndef CONFIG_H
#define CONFIG_H
#include <iostream>
using namespace std;

namespace cfg
{
    // Для редиса:
    const string localhost = "localhost";
    const string host = "localhost"; 
    const int port = 6379;
    const string update_channel = "update_sensors";
    const int count_of_pool = 20;
    const string publish_channel = "";
    //Для БПФ
    const int N = 2048;
    const int sound_speed = 343;
    const double PI = 3.14159265358979323846;
    const int Fs = 48000; // Частота дискретизации
}

#endif //CONFIG_H
