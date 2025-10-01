#include <iostream>
#include <vector>
#include <tuple>
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <string>
#include <complex>
#include <cmath>
#include <utility>
#include <valarray>


#include "../include/Hyper.h"
#include "../include/config.h"
#include "../include/Sensor.h"
#include "../include/SensorMessage.h"
#include "../include/SensorMessage.h"
#include "../include/Triangulator.h"

using namespace std;
using namespace cfg;

Triangulator::Triangulator(vector <Sensor> sensors, vector <SensorMessage> sensors_messages)
{
    this->sensors = sensors;
    this->sensors_messages = sensors_messages;
}

Triangulator::Triangulator(const Point& p1) {
    points.push_back(p1);
}

Triangulator::Triangulator(const Point& p1, const Point& p2) {
    points.push_back(p1);
    points.push_back(p2);
}

Triangulator::Triangulator(const Point& p1, const Point& p2, const Point& p3) {
    points.push_back(p1);
    points.push_back(p2);
    points.push_back(p3);
}

Point Triangulator::combine() const {
    double s_max = 120;
    size_t n = points.size();
    if (n == 1) {
        auto [x, y, s] = points[0];
        return {x, y, 1};
    }
    if (n == 2) {
        const auto& [x1, y1, s1] = points[0];
        const auto& [x2, y2, s2] = points[1];
        std::vector<double> s_val {s1, s2};
        double s_min = *std::min_element(begin(s_val), end(s_val))-1;
        double w1 = (s1-s_min) / (s_max-s_min);
        double w2 = (s2-s_min) / (s_max-s_min);
        double w_sum = w1 + w2;
        if (w_sum == 0) {
            return {0,0,0};
        }
        double x =  (w1 * x1 + w2 * x2)/w_sum;
        double y =  (w1 * y1 + w2 * y2)/w_sum;
        return {x, y, 2};
    }
    if (n == 3) {
        const auto& [x1, y1, s1] = points[0];
        const auto& [x2, y2, s2] = points[1];
        const auto& [x3, y3, s3] = points[2];
        std::vector<double> s_val {s1, s2, s3};
        double s_min = *std::min_element(begin(s_val), end(s_val))-1;
        double w1 = (s1-s_min) / (s_max-s_min);
        double w2 = (s2-s_min) / (s_max-s_min);
        double w3 = (s3-s_min) / (s_max-s_min);
        double w_sum = w1 + w2 + w3;
        if (w_sum == 0) {
            return {0,0,0};
        }
        double x =  (w1 * x1 + w2 * x2 + w3 * x3)/w_sum;
        double y =  (w1 * y1 + w2 * y2 + w3 * y3)/w_sum;
        std::cout << x << ";" << y << std::endl;
        return {x, y, 3};
    }
    throw std::runtime_error("Invalid number of points");
}

pair<double, double> SpecialNewton(const Hyper& h1, const Hyper& h2) {
    pair<double, double> result(NAN, NAN);

    double t1 = 0.5, t2 = 0.5;
    const double tolerance = 1e-8;
    const int max_iterations = 100;

    for (int iter = 0; iter < max_iterations; iter++) 
    {
        double x1_local = h1.orient * h1.a * cosh(t1);
        double y1_local = h1.b * sinh(t1);
        double x1 = x1_local * cos(h1.alpha) - y1_local * sin(h1.alpha) + h1.x;
        double y1 = x1_local * sin(h1.alpha) + y1_local * cos(h1.alpha) + h1.y;

        double x2_local = h2.orient * h2.a * cosh(t2);
        double y2_local = h2.b * sinh(t2);
        double x2 = x2_local * cos(h2.alpha) - y2_local * sin(h2.alpha) + h2.x;
        double y2 = x2_local * sin(h2.alpha) + y2_local * cos(h2.alpha) + h2.y;

        double f1 = x1 - x2;
        double f2 = y1 - y2;

        if (abs(f1) < tolerance && abs(f2) < tolerance) {
            result = make_pair(x1, y1);
            break;
        }

        // --- Якобиан ---
        double dx1_dt1 = h1.orient * h1.a * sinh(t1) * cos(h1.alpha)
                       - h1.b * cosh(t1) * sin(h1.alpha);
        double dy1_dt1 = h1.orient * h1.a * sinh(t1) * sin(h1.alpha)
                       + h1.b * cosh(t1) * cos(h1.alpha);

        double dx2_dt2 = h2.orient * h2.a * sinh(t2) * cos(h2.alpha)
                       - h2.b * cosh(t2) * sin(h2.alpha);
        double dy2_dt2 = h2.orient * h2.a * sinh(t2) * sin(h2.alpha)
                       + h2.b * cosh(t2) * cos(h2.alpha);

        double J[2][2] = {
            { dx1_dt1, -dx2_dt2 },
            { dy1_dt1, -dy2_dt2 }
        };

        double det = J[0][0] * J[1][1] - J[0][1] * J[1][0];
        if (abs(det) < 1e-14) {
            break;
        }

        double invJ[2][2] = {
            { J[1][1] / det, -J[0][1] / det },
            { -J[1][0] / det, J[0][0] / det }
        };

        double dt1 = -(invJ[0][0] * f1 + invJ[0][1] * f2);
        double dt2 = -(invJ[1][0] * f1 + invJ[1][1] * f2);

        t1 += dt1;
        t2 += dt2;

        if (abs(t1) > 20 || abs(t2) > 20) {
            break;
        }
    }

    return result;
}


double CrossCorrelation(SensorMessage message1, SensorMessage message2)
{
    // Нормируем сигналы
    double avg1 = 0, avg2 = 0;
    
    // Вычисляем средние
    for(auto e : message1.pcm_sound) avg1 += e;
    avg1 /= message1.pcm_sound.size();
    for(auto& e : message1.pcm_sound) e -= avg1;  // Добавлен & для изменения исходных данных
    
    for(auto e : message2.pcm_sound) avg2 += e;
    avg2 /= message2.pcm_sound.size();
    for(auto& e : message2.pcm_sound) e -= avg2;  // Добавлен & для изменения исходных данных
    
    cout << avg1 << " - it is avg 1\n";
    cout << avg2 << " - it is avg 2\n";

    vector<double> R(N, 0.0); // Вектор размером N
    
    for(int tau = 0; tau < N; tau++)
    {
        double sum = 0;
        for(int n = 0; n < N - tau; n++) // Избегаем выхода за границы
        {
            sum += message1.pcm_sound[n] * message2.pcm_sound[n + tau];
        }
        R[tau] = sum;
    }

    int max_id = max_element(R.begin(), R.end()) - R.begin();
    return static_cast<double>(max_id) * 20.83e-6;
}

pair<double, double> Triangulator::PointDeterminate() { //##
    pair<double, double> cords(0, 0);
    vector <double> amp;
    amp.push_back(*max_element(sensors_messages[0].pcm_sound.begin(), sensors_messages[0].pcm_sound.end()));
    amp.push_back(*max_element(sensors_messages[1].pcm_sound.begin(), sensors_messages[1].pcm_sound.end()));
    amp.push_back(*max_element(sensors_messages[2].pcm_sound.begin(), sensors_messages[2].pcm_sound.end()));

    // Вычисляем фазу и частоту для каждого датчика
    // Вычисляем разности времен
    double delta_t1 = CrossCorrelation(sensors_messages[0], sensors_messages[1]); cout << delta_t1 << "\n";
    double delta_t2 = CrossCorrelation(sensors_messages[0], sensors_messages[2]); cout << delta_t2 << "\n";
    cout << delta_t1 << "  " << delta_t2 << " t\n";
    // Вычисляем разности расстояний
    double delta_d1 = delta_t1 * sound_speed;
    double delta_d2 = delta_t2 * sound_speed;
    
    // Координаты датчиков
    double x1 = sensors[0].x, y1 = sensors[0].y;
    double x2 = sensors[1].x, y2 = sensors[1].y;
    double x3 = sensors[2].x, y3 = sensors[2].y;
    
    cout << sensors.size() << " " << amp.size() << "\n";
    cout << sensors_messages.size() << "\n";

    // Вычисляем параметры первой гиперболы (между датчиками 1 и 2)
    double R = sqrt(pow((x2 - x1), 2) + pow(y2- y1, 2));
    double r1 = abs(0.5*(R+delta_d1)), r2 = abs(0.5*(R-delta_d1));
    double a1 = R/2 - r2;
    double b1 = sqrt(R*R/4 - a1*a1);
    
    // Угол поворота первой гиперболы
    double alpha1 = atan((y2 - y1)/(x2 - x1));
    if(abs(alpha1 - PI) < 0.01) alpha1 = 0;
    else if(alpha1 < 0) alpha1 = alpha1 + PI;

    // Центр первой гиперболы
    double xc1 = (x1 + x2) / 2;
    double yc1 = (y1 + y2) / 2;
    
    // Определяем ориентацию через амплитуды 
    int orient1 = 0;
    if(amp[0] > amp[1])
    {
        if(sensors[0].y > sensors[1].y) orient1 = 1;
        else if(sensors[0].y < sensors[1].y) orient1 = -1;
        else orient1 = 0;
    }
    else if(amp[0] < amp[1])
    {
        if(sensors[0].y > sensors[1].y) orient1 = -1;
        else if(sensors[0].y < sensors[1].y) orient1 = 1;
        else orient1 = 0;
    }
    
    cout << a1 << " " << b1 << " " << alpha1 << " 1: a, b, alpha\n";
    cout << xc1 << " " << yc1 << " " << orient1 << " x, y, o\n";
    
    Hyper hyper1(xc1, yc1, a1, b1, alpha1, orient1);
    
    // Вычисляем параметры второй гиперболы (между датчиками 1 и 3)
    
    R = sqrt(pow(x3 - x1, 2) + pow(y3 - y1, 2));
    r1 = abs(0.5*(R+delta_d2)); r2 = abs(0.5*(R-delta_d2)); 
    double a2 = R/2 - r2;
    double b2 = sqrt(R*R/4 - a2*a2);

    double xc2 = (x1 + x3) / 2;
    double yc2 = (y1 + y3) / 2;
    double alpha2 = atan((y3 - y1)/(x3 - x1));
    if(abs(alpha2 - PI) < 0.01) alpha2 = 0;
    else if(alpha2 < 0) alpha2 = alpha2 + PI; 

    int orient2 = 0;
    if(amp[0] > amp[2])
    {
        if(sensors[0].y > sensors[2].y) orient2 = 1;
        else if(sensors[0].y < sensors[2].y) orient2 = -1;
        else orient2 = 0;
    }
    else if(amp[0] < amp[2])
    {
        if(sensors[0].y > sensors[2].y) orient2 = -1;
        else if(sensors[0].y < sensors[2].y) orient2 = 1;
        else orient2 = 0;
    }
    
    cout << a2 << " " << b2 << " " << alpha2 << " 2: a, b, alpha\n";
    cout << xc2 << " " << yc2 << " " << orient2 << " x, y, o\n";
    Hyper hyper2(xc2, yc2, a2, b2, alpha2, orient2);

    cords = SpecialNewton(hyper1, hyper2);
    
    return cords;
}

