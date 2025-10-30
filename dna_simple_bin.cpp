#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>

using namespace std;

const map<char, unsigned char> DNA_TO_BIN = {
    {'A', 0b00},
    {'C', 0b01},
    {'G', 0b10},
    {'T', 0b11}
};

const map<unsigned char, char> BIN_TO_DNA = {
    {0b00, 'A'},
    {0b01, 'C'},
    {0b10, 'G'},
    {0b11, 'T'}
};

void compactar_binario_simples(const string& input_filename, const string& output_filename) {
    ifstream input(input_filename);
    ofstream output(output_filename, ios::binary);
    
    if (!input.is_open() || !output.is_open()) {
        throw runtime_error("Erro ao abrir arquivos.");
    }

    string sequence((istreambuf_iterator<char>(input)), istreambuf_iterator<char>());
    input.close();

    long long dna_size = 0;
    string valid_sequence;
    for(char c : sequence) {
        if (DNA_TO_BIN.count(c)) {
            valid_sequence += c;
        }
    }
    dna_size = valid_sequence.length();
    
    output.write(reinterpret_cast<const char*>(&dna_size), sizeof(dna_size));
    
    unsigned char byte_atual = 0;
    int bits_escritos = 0;

    for (char c : valid_sequence) {
        unsigned char codigo = DNA_TO_BIN.at(c);
        
        byte_atual |= (codigo << (6 - bits_escritos));
        
        bits_escritos += 2;

        if (bits_escritos == 8) {
            output.write(reinterpret_cast<const char*>(&byte_atual), 1);
            byte_atual = 0;
            bits_escritos = 0;
        }
    }

    if (bits_escritos > 0) {
        output.write(reinterpret_cast<const char*>(&byte_atual), 1);
    }
    
    cout << "Compactação binária simples concluída. Tamanho original: " << dna_size << " bases (aprox. " << dna_size << " bytes)." << endl;
    cout << "Compactado em: " << output_filename << endl;
}

void descompactar_binario_simples(const string& input_filename, const string& output_filename) {
    ifstream input(input_filename, ios::binary);
    ofstream output(output_filename);

    if (!input.is_open() || !output.is_open()) {
        throw runtime_error("Erro ao abrir arquivos.");
    }

    long long dna_size;
    input.read(reinterpret_cast<char*>(&dna_size), sizeof(dna_size));

    unsigned char byte_lido;
    long long bases_lidas = 0;
    
    while (bases_lidas < dna_size && input.read(reinterpret_cast<char*>(&byte_lido), 1)) {
        for (int i = 0; i < 4; ++i) {
            if (bases_lidas >= dna_size) break;

            unsigned char codigo = (byte_lido >> (6 - i * 2)) & 0b11; 

            output << BIN_TO_DNA.at(codigo);
            bases_lidas++;
        }
    }
    
    cout << "Descompactação binária simples concluída. Bases descompactadas: " << bases_lidas << endl;
    cout << "Descompactado em: " << output_filename << endl;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Uso: " << argv[0] << " <compactar|descompactar> <arquivo_entrada> <arquivo_saida.bin>" << endl;
        return 1;
    }

    string mode = argv[1];
    string input_file = argv[2];
    string output_file = argv[3];

    try {
        if (mode == "compactar") {
            compactar_binario_simples(input_file, output_file);
        } else if (mode == "descompactar") {
            descompactar_binario_simples(input_file, output_file);
        } else {
            cerr << "Modo inválido. Use 'compactar' ou 'descompactar'." << endl;
            return 1;
        }
    } catch (const exception& e) {
        cerr << "Erro: " << e.what() << endl;
        return 1;
    }

    return 0;
}