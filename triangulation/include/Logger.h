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
        ofstream fout;
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
            fout << res; 
            cout << res;
        }

        Logger(string name)
        {
            filename = name;
            fout.open(filename, ios::app);
        }

        ~Logger()
        {
            fout.close();
        }

};

#endif //LOGGER_H
