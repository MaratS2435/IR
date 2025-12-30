#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <cstdint>

namespace fs = std::filesystem;
using namespace std;

struct IndexEntry {
    string word;
    int32_t doc_id;

    bool operator<(const IndexEntry& other) const {
        if (word != other.word) {
            return word < other.word;
        }
        return doc_id < other.doc_id;
    }
};

int main() {
    string input_dir = "stemmed";
    string output_file = "inverted_index.bin";

    vector<IndexEntry> entries;

    cout << "Reading files and building pair list..." << endl;

    int file_count = 0;
    if (fs::exists(input_dir)) {
        for (const auto& entry : fs::directory_iterator(input_dir)) {
            string filename = entry.path().filename().string();
            size_t dot_pos = filename.find('.');
            if (dot_pos == string::npos) continue;

            try {
                int32_t doc_id = stoi(filename.substr(0, dot_pos));

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

    cout << "Compressing and saving index (BINARY)..." << endl;
    ofstream out(output_file, ios::binary);

    if (!entries.empty()) {
        size_t i = 0;
        while (i < entries.size()) {
            string current_word = entries[i].word;
            vector<int32_t> current_docs;

            while (i < entries.size() && entries[i].word == current_word) {
                if (current_docs.empty() || current_docs.back() != entries[i].doc_id) {
                    current_docs.push_back(entries[i].doc_id);
                }
                i++;
            }

            uint32_t word_len = static_cast<uint32_t>(current_word.size());
            out.write(reinterpret_cast<const char*>(&word_len), sizeof(word_len));

            out.write(current_word.c_str(), word_len);

            uint32_t docs_count = static_cast<uint32_t>(current_docs.size());
            out.write(reinterpret_cast<const char*>(&docs_count), sizeof(docs_count));

            out.write(reinterpret_cast<const char*>(current_docs.data()), current_docs.size() * sizeof(int32_t));
        }
    }

    out.close();
    cout << "Index saved to " << output_file << endl;
    return 0;
}