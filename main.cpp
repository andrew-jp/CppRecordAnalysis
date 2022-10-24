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

/// @brief Read in the given filepath to an initialization file
///        and store the data in the passed in map.
/// @param filePath path to .ini file
/// @param vars <string, double> map to hold variable names and values in pair
void read(char* filePath, map<string, double> & vars) {
    map<string, bool> var_check = {
        {"vt", false},
        {"width", false},
        {"pulse_delta", false},
        {"drop_ratio", false},
        {"below_drop_ratio", false}
    };
    fstream arg_file(filePath);
    string label;
    double val = 0.00;
    char temp;

    while (arg_file >> temp) {
        if (temp == '=') { // read continuously until =
            // invalid label
            if (vars.count(label) == 0) {
                cout << "Invalid or duplicate INI key: " << label << endl;
                exit(1);
            }
            
            arg_file >> val;
            // bad numeric value 
            if (arg_file.fail()) {
                cout << "Invalid value for " << label << " in " << filePath << endl;
                exit(1);
            }

            // label and value are valid, add to map and zero vars
            vars[label] = val;
            var_check[label] = true;
            label = "";
            temp = 0;
        }
        else {
            label += temp; // haven't hit =, add char to label string
        }
    }

    // verify we got all the keys
    for (auto e : var_check) {
        if (e.second == false) {
            cout << "Invalid INI file: missing one or more keys" << endl;
            exit(0);
        }
    }
    // end of read() function
}

int main(int argc, char** argv)
{
    if (argc <= 1) {
        cout << "Missing .ini file" << endl;
        std::exit(1);
    }

    map<string, double> variables = {
        {"vt", 0}, // voltage threshold, is Vi+2 - Vi > VT, it's a pulse
        {"width", 0},
        {"pulse_delta", 0},
        {"drop_ratio", 0.00}, // 
        {"below_drop_ratio", 0} // number of values less than drop ratio
    };

    read(argv[1], variables); // initialize the above map or exit on error in read()

    int vt = variables["vt"];
    int width = variables["width"];
    int pulse_delta = variables["pulse_delta"];
    float drop_ratio = variables["drop_ratio"];
    int below_drop_ratio = variables["below_drop_ratio"];

    string path(".\\"); // current working directory
    string ext(".dat"); // .dat file extensions

    vector<fs::directory_entry> files; // vector of the file path pointers
    // store the .dat file paths so I don't have to indent the entire program within a loop
    for (auto &p : fs::recursive_directory_iterator(path)) {
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

        //
        // Get the smooth data
        //

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
            if (smooth_data[i+2] - smooth_data[i] > vt) {
                pulses.push_back(i);
                i+=2;
                while (i < smooth_data.size() && smooth_data[i] < smooth_data[i+1]) {
                    i++; // continue the pulse until it starts to decrease
                }
            }
        }


        vector<int> piggyback;
        for (size_t i = 0; i < pulses.size() - 1; i++) {
            if (pulses[i+1] - pulses[i] <= pulse_delta) {

                // look for piggybacks
                vector<int> sub_pulse; // smooth data between pulse[i] and pulse[i+1]
                copy(smooth_data.begin() + pulses[i], smooth_data.begin() + pulses[i+1], back_inserter(sub_pulse));

                int peak = *max_element(sub_pulse.begin(), sub_pulse.end()); // get the peak of the subpulse
                int below_drop_count = 0;
                auto start_at_peak = find(sub_pulse.begin(), sub_pulse.end(), peak); // iterator for subpulse starting at the peak
                for (auto start = start_at_peak; start != sub_pulse.end(); start++) {
                    if ( *start < (drop_ratio * peak) ) { below_drop_count++; }
                }

                // if it's a piggyback add it to the delete list
                if (below_drop_count > below_drop_ratio) {
                    piggyback.push_back(i);
                }
            }

        }

        // if the file has no pulses, say it's invalid, else print name and then data
        if (pulses.size() == 0) {
            cout <<"Invalid file: " << p.path().filename().string() << endl;
            break;
        }

        // output the file name
        cout << p.path().filename().string() << ":" << endl;
        // output any piggybacks
        for (auto pi : piggyback) {cout << "Found piggyback at " << pulses[pi] << endl;}

        // go in reverse through piggybacks and delete from pulses
        auto rbeg = piggyback.rbegin();
        while (rbeg != piggyback.rend()) {
            pulses.erase(pulses.begin() + *rbeg); // erase piggyback from pulses
            rbeg++; // increment the iter
        }

        /// calculate area below here

        auto start = pulses.begin();
        auto end = pulses.end();
        int area;
        while (true) {
            auto next = start + 1;
            area = 0;

            if (next != end) {
                if (*next - *start <= width) { // pulses -> start of next pulse, not (width)
                    area = accumulate(raw_data.begin() + *start, raw_data.begin() + *(start + 1), 0);
                }
            }
            else { // last pulse index in the list
                if (*start + width >= (int)raw_data.size()) { // hits end before (width)
                    area = accumulate(raw_data.begin() + *start, raw_data.end(), 0);
                }
            }

            if (!area) { // full (width) area
                area = accumulate(raw_data.begin() + *start, raw_data.begin() + *start + width, 0);
            }
            
            // output
            cout << *start << " (" << area << ")" << endl;

            if (++start == end) {
                cout << endl;
                break;
            }
        }        
        // end of file loop
    }
    std::exit(0);
    // end of main
}