#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdint>

using namespace std;

struct IndexEntry {
    string word;
    vector<int32_t> doc_ids;
};

string stem_word(string word) {
    const vector<string> ENDINGS = {
        "вшими", "ями", "ими", "ого", "ому", "ыми", "ыми",
        "его", "ему", "ем", "ми", "ая", "ое", "ые", "ие", "ый", "ой", "ей", "ий", "ую", "юю", "ая", "яя",
        "ом", "ах", "ях", "ых", "их", "ть", "ти", "ла", "на", "ете", "йте", "ли", "й", "л", "ем", "н", "ю",
        "а", "е", "и", "о", "у", "ы", "э", "ю", "я"
    };
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

vector<IndexEntry> load_index(const string& filename) {
    vector<IndexEntry> index;
    ifstream file(filename, ios::binary);

    if (!file.is_open()) {
        cerr << "Error: Could not open " << filename << endl;
        return index;
    }

    while (file.peek() != EOF) {
        IndexEntry entry;
        uint32_t word_len;

        if (!file.read(reinterpret_cast<char*>(&word_len), sizeof(word_len))) break;

        entry.word.resize(word_len);
        file.read(&entry.word[0], word_len);

        uint32_t docs_count;
        file.read(reinterpret_cast<char*>(&docs_count), sizeof(docs_count));

        entry.doc_ids.resize(docs_count);
        file.read(reinterpret_cast<char*>(entry.doc_ids.data()), docs_count * sizeof(int32_t));

        index.push_back(entry);
    }

    return index;
}

int binary_search_index(const vector<IndexEntry>& index, const string& word) {
    int left = 0;
    int right = index.size() - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (index[mid].word == word) {
            return mid;
        }
        if (index[mid].word < word) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return -1;
}

vector<int32_t> intersect_lists(const vector<int32_t>& a, const vector<int32_t>& b) {
    vector<int32_t> result;
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] < b[j]) {
            i++;
        } else if (a[i] > b[j]) {
            j++;
        } else {
            result.push_back(a[i]);
            i++; j++;
        }
    }
    return result;
}

vector<int32_t> unite_lists(const vector<int32_t>& a, const vector<int32_t>& b) {
    vector<int32_t> result;
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] < b[j]) {
            result.push_back(a[i]);
            i++;
        } else if (a[i] > b[j]) {
            result.push_back(b[j]);
            j++;
        } else {
            result.push_back(a[i]);
            i++; j++;
        }
    }
    while (i < a.size()) result.push_back(a[i++]);
    while (j < b.size()) result.push_back(b[j++]);
    return result;
}

vector<int32_t> diff_lists(const vector<int32_t>& a, const vector<int32_t>& b) {
    vector<int32_t> result;
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] < b[j]) {
            result.push_back(a[i]);
            i++;
        } else if (a[i] > b[j]) {
            j++;
        } else {
            i++; j++;
        }
    }
    while (i < a.size()) result.push_back(a[i++]);
    return result;
}


int main() {
    cout << "Loading index..." << endl;
    vector<IndexEntry> index = load_index("inverted_index.bin");
    cout << "Index loaded. Words count: " << index.size() << endl;

    string line;
    while (true) {
        cout << "\nSearch > ";
        getline(cin, line);
        if (line == "exit") break;
        if (line.empty()) continue;

        stringstream ss(line);
        string token;
        vector<string> tokens;
        while (ss >> token) tokens.push_back(token);

        if (tokens.empty()) continue;

        string w1 = stem_word(tokens[0]);
        int idx = binary_search_index(index, w1);
        vector<int32_t> current_docs;
        if (idx != -1) current_docs = index[idx].doc_ids;

        for (size_t i = 1; i < tokens.size(); i += 2) {
            if (i + 1 >= tokens.size()) break;

            string op = tokens[i];
            string w2 = stem_word(tokens[i+1]);

            int idx2 = binary_search_index(index, w2);
            vector<int32_t> next_docs;
            if (idx2 != -1) next_docs = index[idx2].doc_ids;

            if (op == "&") {
                current_docs = intersect_lists(current_docs, next_docs);
            } else if (op == "|") {
                current_docs = unite_lists(current_docs, next_docs);
            } else if (op == "!") {
                current_docs = diff_lists(current_docs, next_docs);
            }
        }

        if (current_docs.empty()) {
            cout << "Nothing found." << endl;
        } else {
            cout << "Found " << current_docs.size() << " documents: ";
            int limit = 0;
            for (int32_t id : current_docs) {
                cout << id << " ";
                if (++limit > 20) { cout << "..."; break; }
            }
            cout << endl;
        }
    }

    return 0;
}