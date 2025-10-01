#pragma once
#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <vector>
#include <ctime>
#include <string>
#include <fstream>

using namespace std;

class Logger
{
    private:
        string filename;
        ifstream fin;
    public:

        void addWriting(string message, char type)
        {
            string res = "";
            switch(type)
            {
                case 'E':
                    res += "[ERROR]";
                    exit(1);
                    break;
                case 'I':
                    res += "[INFO]";
                    break;
                default:
                    res += "[WARNING]";
                    break;
            }

            res += " " + message + "\n";
            fin >> res; 
            cout << res;
        }

        Logger(string name)
        {
            filename = name;
            fin.open(filename);
        }

        ~Logger()
        {
            fin.close();
        }

};

#endif //LOGGER_H
