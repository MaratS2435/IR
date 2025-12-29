#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;
using namespace std;

struct IndexEntry {
    string word;
    int doc_id;

    bool operator<(const IndexEntry& other) const {
        if (word != other.word) {
            return word < other.word;
        }
        return doc_id < other.doc_id;
    }
};

int main() {
    string input_dir = "stemmed";
    string output_file = "inverted_index.txt";
    
    vector<IndexEntry> entries;

    cout << "Reading files and building pair list..." << endl;
    
    int file_count = 0;
    if (fs::exists(input_dir)) {
        for (const auto& entry : fs::directory_iterator(input_dir)) {
            string filename = entry.path().filename().string();
            size_t dot_pos = filename.find('.');
            if (dot_pos == string::npos) continue;
            
            try {
                int doc_id = stoi(filename.substr(0, dot_pos));
                
                ifstream file(entry.path());
                string word;
                if (file.is_open()) {
                    while (file >> word) {
                        entries.push_back({word, doc_id});
                    }
                }
                file_count++;
                if (file_count % 500 == 0) cout << "Parsed " << file_count << " docs..." << endl;
            } catch (...) {
                continue; 
            }
        }
    }

    cout << "Total entries loaded: " << entries.size() << endl;

    cout << "Sorting entries..." << endl;
    sort(entries.begin(), entries.end());

    cout << "Compressing and saving index..." << endl;
    ofstream out(output_file);
    
    if (!entries.empty()) {
        string current_word = entries[0].word;
        out << current_word << ":";
        out << entries[0].doc_id;
        
        for (size_t i = 1; i < entries.size(); ++i) {
            if (entries[i].word == current_word) {
                if (entries[i].doc_id != entries[i-1].doc_id) {
                    out << " " << entries[i].doc_id;
                }
            } else {
                out << endl;
                current_word = entries[i].word;
                out << current_word << ":" << entries[i].doc_id;
            }
        }
        out << endl;
    }

    cout << "Index saved to " << output_file << endl;
    return 0;
}