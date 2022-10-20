// main.cpp
// p4 data smoothing

#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>
#include <istream>
#include <iosfwd>
#include <iterator>

using namespace std;
namespace fs = std::filesystem;


void transform(iterator& iter, vector<int>& end, int pos) {
    (iter[-3] + 2*iter[-2] + 3*iter[-1] + 3*iter[0] + 3*iter[1] + 2*iter[2] + iter[3]) / 15;
}

int main()
{
    string path("./"); // current working directory
    string ext(".dat"); // .dat file extensions

    vector<fs::path> files; // vector of the file path pointers

    // store the .dat file paths so I don't have to indent the entire program within a loop
    for (auto &p : fs::recursive_directory_iterator(path))
    {
        if (p.path().extension() == ext)
            files.push_back(p.path());
    }

    for (auto p : files) {
        fstream fs(p);
        vector<int> raw_data;

        int num;
        while (fs >> num) {
            raw_data.emplace_back(num);
        }

        vector<int> data_copy;
        copy(begin(raw_data), end(raw_data), data_copy);

        for (int i = 3; i < raw_data.size() - 3; i++) {

        }
    }
}