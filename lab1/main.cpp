#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>
#include <set>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include "tokenizer.c"


std::string to_lowercase(const std::string &str) {
    std::string result;
    result.resize(str.size());
    std::transform(str.begin(), str.end(), result.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    return result;
}


unsigned int hash_prefix(const std::string &prefix) {
    int len = prefix.length();
    unsigned int value = 0;

    if (len >= 3) {
        value = (prefix[0] * 31 * 31) + (prefix[1] * 31) + prefix[2];
    } else if (len == 2) {
        value = (prefix[0] * 31) + prefix[1];
    } else if (len == 1) {
        value = prefix[0];
    }

    return value ;
}



void create_index(std::ostream& index_file, std::ifstream& file) {
    std::string line;
    std::set<std::string> seen_prefixes;
    std::streampos position = 0;

    while (std::getline(file, line)) {
        auto first_space = line.find(' ');
        std::string prefix = to_lowercase(line.substr(0, std::min(first_space, size_t(3))));

        if (seen_prefixes.find(prefix) == seen_prefixes.end()) {
            int hash_value = hash_prefix(prefix);
           
            index_file << hash_value << " " << position << "\n";
           
            seen_prefixes.insert(prefix);
        }
        position += line.length() + 1;  
    }

    file.clear();
    file.seekg(0);
}


std::vector<int> find_word_positions(std::ifstream &index_file, std::ifstream &main_file, const std::string &word) {
    std::vector<int> matches;

    std::string prefix = word.substr(0, std::min(word.size(), size_t(3)));
    int hash_value = hash_prefix(prefix); 

    std::string line;
    long long start = -1, end = -1;
    int index_hash;
    long long position;

    while (std::getline(index_file, line)) {
        std::istringstream ss(line);
        ss >> index_hash >> position;

        if (index_hash == hash_value) {
            start = position;
            break;
        }
    }

    // If the prefix wasn't found in the index, return empty matches
    if (start == -1) {
        std::cout << "No valid range for the prefix: " << prefix << std::endl;
        return matches;
    }

    // Get the end position (find the next prefix or the end of the index)
    while (std::getline(index_file, line)) {
        std::istringstream ss(line);
        ss >> index_hash >> position;

        if (index_hash != hash_value) {
            end = position;
            break;
        }
    }
    
    if (end == -1) {
        main_file.seekg(0, std::ios::end);
        end = main_file.tellg();  
    }

    long long mid;
    std::string line_word;
    
    while (end - start > 1000) { 
        mid = (start + end) / 2;

        main_file.seekg(mid, std::ios::beg);

        std::getline(main_file, line);
        std::getline(main_file, line);  

        std::istringstream ss(line);
        ss >> line_word >> position;

        if (line_word < word) {
            start = mid; 
        } else {
            end = mid;  
        }
    }

    main_file.seekg(start, std::ios::beg);

    while (std::getline(main_file, line)) {
        std::istringstream ss(line);
        ss >> line_word >> position;

        if (line_word == word) {
            matches.push_back(position);
        }

        if (line_word > word) {
            break;  
        }
    }



    return matches;
}


void show_occurrences(const std::vector<int> &positions, std::ifstream &text_file, const std::string &word, int context_size = 30) {
    std::cout << "Found " << positions.size() << " instances of the word \"" << word << "\":\n";
    std::cout << std::string(50, '-') << "\n";

    int displayed = 0;
    const int max_display = 25;
    std::string choice;

    for (auto pos : positions) {
        if (displayed >= max_display) {
            std::cout << "Shown " << max_display << " results. Show more? (y/n): ";
            std::cin >> choice;
            if (choice != "y") break;
            displayed = 0;
            std::cout << std::string(50, '-') << "\n";
        }

        text_file.seekg(std::max(0, pos - context_size));

        std::vector<char> before(context_size + 1, '\0');
        std::vector<char> after(context_size + 1, '\0');
        std::string found_word;

        text_file.read(before.data(), context_size);
        text_file >> found_word;
        text_file.read(after.data(), context_size);

        std::cout << std::setw(3) << (++displayed) << ". ... " << before.data()
                  << "***" << "\033[1;33m"<< found_word <<"\033[0m"<< "***" << after.data() << " ...\n";
        std::cout << std::string(50, '-') << "\n";
    }

    std::cout << "End of results.\n";
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <search_term>\n";
        return 1;
    }
    initialize();
    std::string search_word = argv[1];

    std::ifstream text_stream("korpus");
    std::ifstream index_stream("rawindex.txt");

    if (!text_stream || !index_stream) {
        std::cerr << "Error: Cannot open files.\n";
        return 1;
    }
    std::ifstream index_file("index.txt");
    if (!index_file) {
        std::cerr << "Error opening index file!" << std::endl;
        return 1;
    }

    //create_index(index_file, index_stream);

    std::string search_lower = to_lowercase(search_word);
    std::vector<int> result_positions = find_word_positions(index_file, index_stream, search_lower);

    if (result_positions.empty()) {
        std::cout << "No matches found for the word: " << search_word << std::endl;
    } else {
        show_occurrences(result_positions, text_stream, search_word);
    }

    text_stream.close();
    index_stream.close();
    index_file.close();

    return 0;
}

/*int main() {

    initialize();
    std::string search_word;
    std::cout << "Enter the word you want to search for: ";
    std::getline(std::cin, search_word);

    // Load necessary files
    std::ifstream text_stream("/afs/kth.se/misc/info/kurser/DD2350/adk24/labb1/korpus");
    std::ifstream index_stream("/afs/kth.se/misc/info/kurser/DD2350/adk24/labb1/rawindex.txt");

    if (!text_stream || !index_stream) {
        std::cerr << "Error: Cannot open files.\n";
        return 1;
    }
    std::vector<int> index_table(27000, -1);
    create_index(index_table, index_stream);

    std::string search_lower = to_lowercase(search_word);

    std::vector<int> result_positions = find_word_positions(index_table, index_stream, search_lower);

    if (result_positions.empty()) {
        std::cout << "No matches found for the word: " << search_word << std::endl;
    } else {
        show_occurrences(result_positions, text_stream, search_word);
    }

    text_stream.close();
    index_stream.close();

    return 0;
}*/