#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <queue>
#include <vector>
#include <algorithm>

using namespace std;

class BitWriter {
private:
    ofstream& os;
    unsigned char buffer;
    int buffer_bits;

public:
    BitWriter(ofstream& stream) : os(stream), buffer(0), buffer_bits(0) {}

    void write_bit(char bit) {
        buffer = (buffer << 1) | (bit - '0');
        buffer_bits++;

        if (buffer_bits == 8) {
            os.write(reinterpret_cast<const char*>(&buffer), 1);
            buffer = 0;
            buffer_bits = 0;
        }
    }

    void flush() {
        if (buffer_bits > 0) {
            buffer = buffer << (8 - buffer_bits);
            os.write(reinterpret_cast<const char*>(&buffer), 1);
            
            unsigned char padding_bits = 8 - buffer_bits;
            os.write(reinterpret_cast<const char*>(&padding_bits), 1);
        } else {
            unsigned char padding_bits = 0;
            os.write(reinterpret_cast<const char*>(&padding_bits), 1);
        }
        buffer = 0;
        buffer_bits = 0;
    }
    
    void write_byte(unsigned char b) {
        os.write(reinterpret_cast<const char*>(&b), 1);
    }
};

class BitReader {
private:
    ifstream& is;
    unsigned char buffer;
    int buffer_bits;
    long long total_bits_to_read;
    long long bits_read_count;

public:
    BitReader(ifstream& stream, long long total_bits) : is(stream), buffer(0), buffer_bits(0), 
        total_bits_to_read(total_bits), bits_read_count(0) {}

    char read_bit() {
        if (bits_read_count >= total_bits_to_read) return '\0';

        if (buffer_bits == 0) {
            if (!is.read(reinterpret_cast<char*>(&buffer), 1)) return '\0';
            buffer_bits = 8;
        }

        char bit = (buffer >> (buffer_bits - 1)) & 1;
        buffer_bits--;
        bits_read_count++;
        return bit + '0';
    }
};

struct HuffmanNode {
    char data;
    int frequency;
    HuffmanNode *left, *right;

    HuffmanNode(char data, int frequency) : data(data), frequency(frequency), left(nullptr), right(nullptr) {}
    ~HuffmanNode() { delete left; delete right; }
};

struct CompareNode {
    bool operator()(HuffmanNode* a, HuffmanNode* b) {
        return a->frequency > b->frequency;
    }
};

map<char, int> calcular_frequencia(const string& sequence) {
    map<char, int> freq;
    for (char c : sequence) {
        if (c == 'A' || c == 'C' || c == 'G' || c == 'T') {
             freq[c]++;
        }
    }
    return freq;
}

HuffmanNode* construir_arvore(const map<char, int>& freq) {
    priority_queue<HuffmanNode*, vector<HuffmanNode*>, CompareNode> pq;

    for (const auto& pair : freq) {
        pq.push(new HuffmanNode(pair.first, pair.second));
    }

    while (pq.size() > 1) {
        HuffmanNode* left = pq.top(); pq.pop();
        HuffmanNode* right = pq.top(); pq.pop();

        HuffmanNode* parent = new HuffmanNode('\0', left->frequency + right->frequency);
        parent->left = left;
        parent->right = right;
        pq.push(parent);
    }
    return pq.empty() ? nullptr : pq.top();
}

void gerar_codigos(HuffmanNode* root, string code, map<char, string>& codes) {
    if (!root) return;

    if (root->data != '\0') {
        codes[root->data] = code;
    }

    gerar_codigos(root->left, code + "0", codes);
    gerar_codigos(root->right, code + "1", codes);
}

void salvar_tabela(ofstream& output, const map<char, string>& codes) {
    unsigned char num_symbols = codes.size();
    output.write(reinterpret_cast<const char*>(&num_symbols), 1);
    
    for (const auto& pair : codes) {
        output.write(&pair.first, 1);
        
        unsigned char code_len = pair.second.length();
        output.write(reinterpret_cast<const char*>(&code_len), 1);
        
        BitWriter bw(output);
        for(char bit : pair.second) {
            bw.write_bit(bit);
        }
        bw.flush(); 
    }
}

map<string, char> carregar_tabela(ifstream& input) {
    map<string, char> code_to_char;
    unsigned char num_symbols;
    
    if (!input.read(reinterpret_cast<char*>(&num_symbols), 1)) throw runtime_error("Erro ao ler número de símbolos.");
    
    for (int i = 0; i < num_symbols; ++i) {
        char dna_base;
        unsigned char code_len;
        
        if (!input.read(&dna_base, 1)) throw runtime_error("Erro ao ler base de DNA.");
        if (!input.read(reinterpret_cast<char*>(&code_len), 1)) throw runtime_error("Erro ao ler comprimento do código.");
        
        string code = "";
        char byte_lido;
        int bits_lidos = 0;
        
        while (bits_lidos < code_len) {
            if (bits_lidos % 8 == 0) {
                if (!input.read(&byte_lido, 1)) throw runtime_error("Erro ao ler byte do código.");
            }
            
            char bit = ((byte_lido >> (7 - (bits_lidos % 8))) & 1) + '0';
            code += bit;
            bits_lidos++;
        }
        
        code_to_char[code] = dna_base;
    }
    return code_to_char;
}

void compactar_huffman(const string& input_filename, const string& output_filename) {
    ifstream input(input_filename);
    ofstream output(output_filename, ios::binary);
    
    if (!input.is_open() || !output.is_open()) throw runtime_error("Erro ao abrir arquivos.");
    
    string sequence((istreambuf_iterator<char>(input)), istreambuf_iterator<char>());
    input.close();

    map<char, int> freq = calcular_frequencia(sequence);
    string valid_sequence;
    long long total_bits = 0;

    for(char c : sequence) {
        if (freq.count(c)) valid_sequence += c;
    }
    
    if (valid_sequence.empty()) {
        cout << "Sequência de DNA vazia ou inválida." << endl;
        return;
    }

    HuffmanNode* root = construir_arvore(freq);
    map<char, string> huffman_codes;
    gerar_codigos(root, "", huffman_codes);

    salvar_tabela(output, huffman_codes);
    
    for (char c : valid_sequence) {
        total_bits += huffman_codes.at(c).length();
    }
    output.write(reinterpret_cast<const char*>(&total_bits), sizeof(total_bits));

    BitWriter bw(output);
    for (char c : valid_sequence) {
        for (char bit : huffman_codes.at(c)) {
            bw.write_bit(bit);
        }
    }
    bw.flush(); 
    
    cout << "Compactação Huffman concluída." << endl;
    cout << "Tamanho original (bits): " << valid_sequence.length() * 8 << endl;
    cout << "Tamanho compactado (bits - DADOS): " << total_bits << endl;
    cout << "Compactado em: " << output_filename << endl;

    delete root;
}

void descompactar_huffman(const string& input_filename, const string& output_filename) {
    ifstream input(input_filename, ios::binary);
    ofstream output(output_filename);
    
    if (!input.is_open() || !output.is_open()) throw runtime_error("Erro ao abrir arquivos.");

    map<string, char> code_to_char = carregar_tabela(input);
    
    long long total_bits;
    if (!input.read(reinterpret_cast<char*>(&total_bits), sizeof(total_bits))) throw runtime_error("Erro ao ler total de bits.");

    unsigned char padding_bits;
    input.seekg(-1, ios::end); 
    input.read(reinterpret_cast<char*>(&padding_bits), 1);
    input.seekg(sizeof(unsigned char) + sizeof(long long) + 
                code_to_char.size() * (1 + 1 + 1) + 
                sizeof(long long) , ios::beg); 
    
    long long total_bytes = (total_bits + padding_bits) / 8;
    input.seekg(-(total_bytes + 1), ios::cur);
    
    long long bits_lidos = 0;
    string current_code = "";
    unsigned char byte_lido;
    int bits_buffer = 0;
    long long decoded_count = 0;

    while (bits_lidos < total_bits) {
        if (bits_buffer == 0) {
            if (!input.read(reinterpret_cast<char*>(&byte_lido), 1)) break;
            bits_buffer = 8;
        }

        char bit = ((byte_lido >> (bits_buffer - 1)) & 1) + '0';
        bits_buffer--;
        bits_lidos++;
        current_code += bit;

        if (code_to_char.count(current_code)) {
            output << code_to_char.at(current_code);
            current_code = "";
            decoded_count++;
        }
    }
    
    cout << "Descompactação Huffman concluída. Bases descompactadas: " << decoded_count << endl;
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
            compactar_huffman(input_file, output_file);
        } else if (mode == "descompactar") {
            descompactar_huffman(input_file, output_file);
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