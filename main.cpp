// main.cpp
// p4 data smoothing

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

int main(int argc, char** argv)
{
    if (argc <= 1) {
        cout << "Missing .ini file" << endl;
        std::exit(1);
    }

    fstream iniStream(argv[1]);

    string val;
    string key;

    map<string, float> vars = {
        {"vt", -1},
        {"width", -1},
        {"pulse_delta", -1},
        {"drop_ratio", -1},
        {"below_drop_ratio", -1}
    };

    while (getline(iniStream, key, '=') && getline(iniStream, val)) {
        if (vars.count(key) == 0) { // if the key isn't in the map
            cout << "Invalid or duplicate INI key: " << key << endl;
            exit(1);
        }
        try {
            vars[key] = stof(val); // use a float conversion for all, ints become ints later
        } catch (exception e) { // if the conversion fails
            cout << "Invalid value for " << key << " in " << argv[1] << endl;
            exit(1);
        }
    }
    // verify that each of the variables was initialized
    // based on the program I'm assuming -1 is a no-go for any of them
    for (auto pair : vars) {
        if (pair.second == -1) {
            cout << "Invalid INI file: missing one or more keys" << endl;
            exit(1);
        }
    }

    // pull the values out of the map for better readability below
    int vt = vars["vt"];
    int width = vars["width"];
    int pulse_delta = vars["pulse_delta"];
    float drop_ratio = vars["drop_ratio"];
    int below_drop_ratio = vars["below_drop_ratio"];

    string ext(".dat"); // .dat file extensions
    vector<fs::directory_entry> files; // vector of the .dat file path pointers
    // store the .dat file paths so I don't have to indent the entire program within a loop
    for (auto &p : fs::recursive_directory_iterator( fs::current_path() )) {
        if (p.path().extension() == ext) { files.push_back(p); }
    }

    for (auto p : files) { // do everything below for each .dat file
        fstream fs(p.path());
        vector<int> raw_data;
        vector<int> smooth_data;

        // read in the raw data from dat
        int num;
        while (fs >> num) {
            raw_data.push_back(-num); // invert and push
        }

        // copy the first 3, get the smooth data, copy the last 3
        copy(raw_data.begin(), raw_data.begin() + 3, back_inserter(smooth_data));
        int smooth;
        for (size_t i = 3; i < raw_data.size() - 3; i++) {
            smooth = (raw_data[i-3] + 2*(raw_data[i-2]) + 3*(raw_data[i-1]) + 3*(raw_data[i]) + 3*(raw_data[i+1]) + 2*(raw_data[i+2]) + raw_data[i+3]) / 15;
            smooth_data.push_back(smooth);
        }
        copy(raw_data.end() - 3, raw_data.end(), back_inserter(smooth_data));

        vector<int> pulses;
        for (size_t i = 0; i < smooth_data.size() - 2; i++) {
            if (smooth_data[i+2] - smooth_data[i] > vt) {
                pulses.push_back(i);
                i+=2;
                while (i < smooth_data.size() && smooth_data[i] < smooth_data[i+1]) {
                    i++; // continue the pulse until it starts to decrease
                }
            }
        }

        // if the file has no pulses, say it's invalid, else print name and then data
        if (pulses.size() == 0) {
            cout <<"Invalid file: " << p.path().filename().string() << endl;
            cout << endl;
            continue; // out of main loop
        }

        vector<int> piggyback; // hold verified piggybacks (indexes of the pulses to delete)
        for (size_t i = 0; i < pulses.size() - 1; i++) {
            auto first = pulses[i];
            auto next = pulses[i+1];
            if (next - first <= pulse_delta) { // distance betweeen two indexes <+ pulse_delta
                int peak = *max_element(smooth_data.begin() + first, smooth_data.begin() + next); // get the peak of the subpulse
                int drop_count = 0;

                auto start_at_peak = find(smooth_data.begin() + first, smooth_data.begin() + next, peak); // iterator for subpulse starting at the peak
                for (auto start = start_at_peak; start != smooth_data.begin() + next; start++) {
                    if ( *start < (drop_ratio * peak) ) { drop_count++; }
                }
                if (drop_count > below_drop_ratio) { piggyback.push_back(i); }
            }
        }

        // output the file name
        cout << p.path().filename().string() << ":" << endl;
        // output any piggybacks
        for (auto pi : piggyback) {cout << "Found piggyback at " << pulses[pi] << endl;}

        // go in reverse through piggybacks and delete from pulses
        auto rbeg = piggyback.rbegin();
        while (rbeg != piggyback.rend()) { pulses.erase(pulses.begin() + *rbeg++); }

        /// calculate area below here
        int end; // actual end index
        for (auto i = 0; i < pulses.size(); i++) {
            if (i == pulses.size() - 1) { // last pulse - either to i+(width) sample or end of raw_data
                end = pulses[i] + width < raw_data.size() ? pulses[i] + width : raw_data.size();
            }
            else { // not last, either to i+(width) sample or next pulse, whichever is first
                end = pulses[i+1] - pulses[i] < width ? pulses[i+1] : pulses[i] + width;
            }
            int area = accumulate(raw_data.begin() + pulses[i], raw_data.begin() + end, 0);
            cout << pulses[i] << " (" << area << ")" << endl; // output
        }
        cout << endl;   
        // end of file loop
    }
    std::exit(0);
    // end of main
}