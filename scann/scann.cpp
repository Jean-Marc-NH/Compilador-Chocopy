#include "scann.h"

Scann::Scann(const std::string& filename) : current_line(1), current_col(1) {
    source_file.open(filename, std::ios::binary);
    if (!source_file.is_open()) {
        throw std::runtime_error("Error crítico: Imposible abrir el archivo fuente.");
    }
}

Scann::~Scann() {
    if (source_file.is_open()) {
        source_file.close();
    }
}

int Scann::getChar() {
    int ch = source_file.get();

    if (ch == EOF) {
        return EOF;
    }

    if (ch == '\n') {
        current_line++;
        current_col = 1;
    } else {
        current_col++;
    }

    return ch;
}