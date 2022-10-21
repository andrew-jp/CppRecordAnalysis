
#include <iostream>
#include <fstream>
#include <string>
#include <map>

using namespace std;

class IniData {
    public:
    map<string, double> vars = {
        {"vt", 0}, // voltage threshold, is Vi+2 - Vi > VT, it's a pulse
        {"width", 0},
        {"pulse_delta", 0},
        {"drop_ratio", 0.00}, // 
        {"below_drop_ratio", 0} // number of values less than drop ratio
    };

    map<string, bool> var_check = {
        {"vt", false},
        {"width", false},
        {"pulse_delta", false},
        {"drop_ratio", false},
        {"below_drop_ratio", false}
    };

    void read(char* filePath) {
        fstream arg_file(filePath);

        std::string label;
        double val = 0.00;
        char temp;
        while (arg_file >> temp) {
            if (temp == '=') { // read continuously until =
                // invalid label
                if (this->vars.count(label) == 0) {
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
                this->vars[label] = val;
                this->var_check[label] = true;
                label = "";
                temp = 0;
            }
            else {
                label += temp; // haven't hit =, add char to label string
            }
        }

        // verify we got all the keys
        for (auto e : this->var_check) {
            if (e.second == false) {
                cout << "Invalid INI file: missing one or more keys" << endl;
                exit(0);
            }
        }
    }
};
