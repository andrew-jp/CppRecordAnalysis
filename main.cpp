// main.cpp
// p4 data smoothing

#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>
#include <iterator>
#include <algorithm>

#include "IniData.cpp"

using namespace std;
namespace fs = std::filesystem;


int main(int argc, char** argv)
{
    IniData id;
    if (1 < argc) {
        id = IniData();
        id.read(argv[1]);
    }
    else {
        cout << "Missing .ini file" << endl;
        exit(1);
    }

    string path(".\\"); // current working directory
    string ext(".dat"); // .dat file extensions

    vector<fs::directory_entry> files; // vector of the file path pointers

    // store the .dat file paths so I don't have to indent the entire program within a loop
    for (auto &p : fs::recursive_directory_iterator(path))
    {
        if (p.path().extension() == ext)
            files.push_back(p);
    }

    for (auto p : files) {
        fstream fs(p.path());
        vector<int> raw_data;
        vector<int> smooth_data;

        int num;
        while (fs >> num) {
            raw_data.push_back(-num); // get all the raw data and invert
        }

        // get the first 3
        copy(raw_data.begin(), raw_data.begin() + 3, back_inserter(smooth_data));
        int smooth;
        for (size_t i = 3; i < raw_data.size() - 3; i++) {
            smooth = (raw_data[i-3] + 2*(raw_data[i-2]) + 3*(raw_data[i-1]) + 3*(raw_data[i]) + 3*(raw_data[i+1]) + 2*(raw_data[i+2]) + raw_data[i+3]) / 15;
            smooth_data.push_back(smooth);
        }
        // get the last 3
        copy(raw_data.end() - 3, raw_data.end(), back_inserter(smooth_data));

        vector<int> pulses;
        for (size_t i = 0; i < smooth_data.size() - 2; i++) {
            if (smooth_data[i+2] - smooth_data[i] > id.vars["vt"]) {
                pulses.push_back(i);
                i+=2;
                while (i < smooth_data.size() && smooth_data[i] < smooth_data[i+1]) {
                    i++; // continue the pulse until it starts to decrease
                }
            }
        }

        vector<int> toDelete;
        for (size_t i = 0; i < pulses.size() - 1; i++) {
            if (pulses[i+1] - pulses[i] <= id.vars["pulse_delta"]) {

                // look for piggybacks
                vector<int> sub_pulse;
                copy(smooth_data.begin()+pulses[i], smooth_data.begin()+pulses[i+1], back_inserter(sub_pulse));

                int peak = *max_element(sub_pulse.begin(), sub_pulse.end());

                bool flag = true;
                int below_drop_count = 0;
                for (size_t j = 0; j < sub_pulse.size(); j++) {
                    if (flag && sub_pulse[j] != peak) {continue;}
                    else if (sub_pulse[j] == peak) {flag = false;}
                    else {
                        if (sub_pulse[j] < (id.vars["drop_ratio"] * peak)) {
                            below_drop_count++;
                        }
                    }
                }

                if (below_drop_count > id.vars["below_drop_ratio"]) {
                    toDelete.push_back(i);
                }
            }
            // else pass
        }

        cout << p.path().filename().string() << ":" << endl; // output the file name
        // for (auto e:pulses) {cout << e << endl;}
        // cout << endl;

        int size_delta = 0; // since I'm deleting things with index's, I have to adjust
                            // the index I'm deleting by the number deleted
        for (auto i:toDelete) {
            cout << "Found piggyback at " << pulses[i] << endl;
        }
        for (auto i:toDelete) {
            pulses.erase(pulses.begin() + i - size_delta);
            size_delta++;
        }

        size_t cap = pulses.size() - 1;
        for (size_t i = 0; i < pulses.size(); i++) {
            int area = 0;
            int index = pulses[i];
            if (i == cap) {
                while (index < raw_data.size() && index < pulses[i] + 100) {
                    area += raw_data[index];
                    index++;
                }
                cout << pulses[i] << " (" << area << ")" << endl;
            }
            else {
                while (index < (pulses[i] + 100) && index < pulses[i+1]) {
                    area += raw_data[index];
                    index++;
                }
                cout << pulses[i] << " (" << area << ")" << endl;
            }
        }
        cout << endl;
    }
}