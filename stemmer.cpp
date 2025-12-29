#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <cstring>

namespace fs = std::filesystem;
using namespace std;

const vector<string> ENDINGS = {
    "вшими", "ями", "ими", "ого", "ому", "ыми", "ыми",
    "его", "ему", "ем", "ми", "ая", "ое", "ые", "ие", "ый", "ой", "ей", "ий", "ую", "юю", "ая", "яя",
    "ом", "ах", "ях", "ых", "их",
    "ть", "ти", "ла", "на", "ете", "йте", "ли", "й", "л", "ем", "н", "ю",
    "а", "е", "и", "о", "у", "ы", "э", "ю", "я"
};

string stem_word(string word) {
    if (word.length() <= 6) return word; 

    for (const string& ending : ENDINGS) {
        if (word.length() > ending.length()) {
            if (word.compare(word.length() - ending.length(), ending.length(), ending) == 0) {
                return word.substr(0, word.length() - ending.length());
            }
        }
    }
    return word;
}

int main() {
    string input_dir = "tokens";
    string output_dir = "stemmed";
    
    if (!fs::exists(output_dir)) fs::create_directory(output_dir);

    cout << "Start Stemming..." << endl;
    int file_count = 0;

    if (fs::exists(input_dir)) {
        for (const auto& entry : fs::directory_iterator(input_dir)) {
            ifstream infile(entry.path());
            
            string out_path = output_dir + "/" + entry.path().filename().string();
            ofstream outfile(out_path);

            if (infile.is_open() && outfile.is_open()) {
                string word;
                while (infile >> word) {
                    string stemmed = stem_word(word);
                    outfile << stemmed << " ";
                }
                file_count++;
                if (file_count % 500 == 0) cout << "Processed " << file_count << " files..." << endl;
            }
        }
    } else {
        cerr << "Error: Directory 'tokens' not found. Run tokenizer first." << endl;
    }

    cout << "Stemmed " << file_count << " files." << endl;
    return 0;
}