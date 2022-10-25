/*
Data Smoothing and Algorithm Library
Andrew Pritchett
10/25/2022

This program handles raw data, in an effort to improve
readability and efficiency, I've tried to make use of
existing functions from the algorithm library where
possible. The output of the program is the pulses found
in each file, followed by their area.
*/

#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>
#include <iterator>
#include <algorithm>
#include <numeric>
#include <map>

using namespace std;
namespace fs = std::filesystem;

int main(int argc, char** argv) {
    if (argc <= 1) {
        cout << "Missing .ini file" << endl;
        std::exit(1);
    }

    string val;
    string key;
    map<string, float> vars = {
        {"vt", -1},
        {"width", -1},
        {"pulse_delta", -1},
        {"drop_ratio", -1},
        {"below_drop_ratio", -1}
    };

    fstream iniStream(argv[1]);
    while (getline(iniStream, key, '=') && getline(iniStream, val)) {
        if (vars.count(key) == 0) { // if the key isn't in the map
            cout << "Invalid or duplicate INI key: " << key << endl;
            exit(1);
        }
        try {
            vars[key] = stof(val); // use a float conversion for all, ints become ints later
        } catch (exception& e) { // if the conversion fails
            cout << "Invalid value for " << key << " in " << argv[1] << endl;
            exit(1);
        }
    }
    // verify that each of the variables was initialized
    // based on the program I'm assuming -1 is a no-go for any of them
    for_each(vars.begin(), vars.end(), [](auto e) {
                if (e.second == -1) {
                cout << "Invalid INI file: missing one or more keys" << endl;
                exit(1);}
            });

    // pull the values out of the map for better readability below
    int vt = vars["vt"];
    int width = vars["width"];
    int pulse_delta = vars["pulse_delta"];
    float drop_ratio = vars["drop_ratio"];
    int below_drop_ratio = vars["below_drop_ratio"];

    string ext(".dat"); // .dat file extensions
    vector<fs::path> files; // vector of the .dat file path pointers
    // store the .dat file paths so I don't have to indent the entire program within another loop
    for (auto &entry : fs::recursive_directory_iterator( fs::current_path() )) {
        if (entry.path().extension() == ext) {
            files.push_back(entry.path());
        }
    }

    for (auto path : files) { // do everything below for each .dat file
        vector<int> raw_data;
        vector<int> smooth_data;

        // read in the raw data from dat
        int numIn;
        fstream fs(path);
        while (fs >> numIn) { raw_data.push_back(-numIn); }

        // copy the first 3, get the smooth data, copy the last 3
        copy(raw_data.begin(), raw_data.begin() + 3, back_inserter(smooth_data));

        auto rdi = raw_data.begin()+3;
        for_each(rdi, raw_data.end() - 3,
                [&rdi, &smooth_data](auto e) {
                    smooth_data.push_back(
                        (*(rdi-3) + 2*(*(rdi-2)) + 3*(*(rdi-1)) + 3*(*(rdi)) + 3*(*(rdi+1)) + 2*(*(rdi+2)) + *(rdi+3)) / 15
                    );
                    rdi++;
                });

        copy(raw_data.end() - 3, raw_data.end(), back_inserter(smooth_data));

        vector<int> pulses;
        auto sdIter = smooth_data.begin();
        auto sdEnd = smooth_data.end();
        for_each(sdIter, sdEnd - 2, [&](auto e) {
                if (*(sdIter + 2) - *sdIter > vt) {
                    pulses.push_back(sdIter - smooth_data.begin());
                    sdIter += 2;
                    while (sdIter != sdEnd && *sdIter < *(sdIter + 1)) {
                        sdIter++; // continue the pulse until it starts to decrease
                    }
                }
                sdIter++;
            });

        // if the file has no pulses, say it's invalid, else print name and then data
        if (pulses.size() == 0) {
            cout <<"Invalid file: " << path.filename().string() << endl << endl;
            continue; // out of main loop
        }

        vector<int> piggyback; // hold verified piggybacks (indexes of the pulses to delete)
        auto pi = pulses.begin();
        for_each(pi, pulses.end() - 1,
            [&](auto e) {
                auto nxtp = pi + 1; // iter -> next ele
                if (*nxtp - *pi <= pulse_delta) {
                    auto peak = max_element(smooth_data.begin() + *pi, smooth_data.begin() + *nxtp); // get the peak of the subpulse
                    int drop_count = count_if(peak, smooth_data.begin() + *nxtp, 
                        [=] (auto c) { return c < drop_ratio * (*peak); }
                    );
                    if (drop_count > below_drop_ratio) {
                        piggyback.push_back(pi - pulses.begin()); //push index of pulse to piggyback
                    }
                }
                pi++;
            });

        // output the file name
        cout << path.filename().string() << ":" << endl;
        // output any piggybacks
        for_each(piggyback.begin(), piggyback.end(), [pulses](auto i){cout << "Found piggyback at " << pulses[i] << endl;});

        // go in reverse through piggybacks and delete from pulses
        // I tried a remove_if and this method is better
        auto rbeg = piggyback.rbegin();
        while (rbeg != piggyback.rend()) { pulses.erase(pulses.begin() + *rbeg++); }

        /// calculate area below here
        auto nextPulse = pulses.begin();
        nextPulse++;
        int endI;
        for_each(pulses.begin(), pulses.end(),
            [raw_data, width, lastEle = pulses.back(), &nextPulse, &endI] (auto ele) {
                if (ele != lastEle) {endI = *nextPulse - ele < width ? *nextPulse : ele + width;}
                else {endI = ele + width < (int)raw_data.size() ? ele + width : (int)raw_data.size();}
                nextPulse++;
                cout << ele << " (" << accumulate(raw_data.begin() + ele, raw_data.begin() + endI, 0) << ")" << endl;
            });
        cout << endl;
    }
    std::exit(0);
} // end of main