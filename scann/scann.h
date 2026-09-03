#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>

class Scann {
    private:

    std::ifstream source_file;
    int current_line;
    int current_col;

    public:
    Scann(const std::string& filename);
    ~Scann();
    int getChar();
    void peekChar();
    void getToken();
};