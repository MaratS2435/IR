#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;
using namespace std;
using namespace std::chrono;

void replace_all(string& str, const string& from, const string& to) {
    if(from.empty()) return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); 
    }
}

bool is_ascii_separator(char c) {
    string separators = " \n\t\r.,!?:;()[]{}\"'`<>/|\\-=+_~@#$%^&*";
    return separators.find(c) != string::npos;
}

vector<string> tokenize(string text) {
    replace_all(text, "—", " "); 
    replace_all(text, "«", " "); 
    replace_all(text, "»", " "); 
    replace_all(text, "…", " ");
    replace_all(text, "–", " ");
    replace_all(text, "“", " ");
    replace_all(text, "”", " ");
    
    vector<string> tokens;
    string current_token;
    
    for (char c : text) {
        if (is_ascii_separator(c)) {
            if (!current_token.empty()) {
                tokens.push_back(current_token);
                current_token.clear();
            }
        } else {
            current_token += c;
        }
    }
    
    if (!current_token.empty()) {
        tokens.push_back(current_token);
    }
    
    return tokens;
}

int main() {
    string input_dir = "corpus";
    string output_dir = "tokens"; 
    
    if (!fs::exists(output_dir)) fs::create_directory(output_dir);

    cout << "Start tokenization..." << endl;
    int file_count = 0;

    if (fs::exists(input_dir)) {
        for (const auto& entry : fs::directory_iterator(input_dir)) {
            ifstream file(entry.path());
            if (file.is_open()) {
                string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
                
                vector<string> file_tokens = tokenize(content);
                
                string out_path = output_dir + "/" + entry.path().filename().string();
                ofstream outfile(out_path);
                
                for (const string& t : file_tokens) {
                    if (t.length() > 1) {
                        outfile << t << " ";
                    }
                }
                
                file_count++;
                if (file_count % 500 == 0) cout << "Processed " << file_count << " files..." << endl;
            }
        }
    }
    cout << "Processed " << file_count << " files." << endl;
    return 0;
}