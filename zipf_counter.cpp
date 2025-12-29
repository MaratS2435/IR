#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;
using namespace std;

struct TermStats {
    string word;
    int count;
};

bool compareFreq(const TermStats& a, const TermStats& b) {
    return a.count > b.count;
}

int main() {
    string input_dir = "stemmed";
    string output_file = "zipf_data.csv";
    
    vector<string> all_words;

    cout << "Reading files..." << endl;
    if (fs::exists(input_dir)) {
        for (const auto& entry : fs::directory_iterator(input_dir)) {
            ifstream file(entry.path());
            string word;
            if (file.is_open()) {
                while (file >> word) {
                    if (word.length() > 0) {
                        all_words.push_back(word);
                    }
                }
            }
        }
    } else {
        cerr << "Directory 'stemmed' not found!" << endl;
        return 1;
    }

    cout << "Total words loaded: " << all_words.size() << endl;
    cout << "Sorting words..." << endl;
    
    sort(all_words.begin(), all_words.end());

    cout << "Counting frequencies..." << endl;
    
    vector<TermStats> frequencies;
    if (!all_words.empty()) {
        int current_count = 1;
        for (size_t i = 1; i < all_words.size(); ++i) {
            if (all_words[i] == all_words[i-1]) {
                current_count++;
            } else {
                frequencies.push_back({all_words[i-1], current_count});
                current_count = 1;
            }
        }
        frequencies.push_back({all_words.back(), current_count});
    }

    sort(frequencies.begin(), frequencies.end(), compareFreq);

    cout << "Writing CSV..." << endl;
    ofstream out(output_file);
    out << "rank,word,frequency" << endl;
    
    for (size_t i = 0; i < frequencies.size(); ++i) {
        out << (i + 1) << "," << frequencies[i].word << "," << frequencies[i].count << endl;
    }

    cout << "Unique terms: " << frequencies.size() << endl;
    return 0;
}