#include "base64.h"
#include <sodium.h>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <filesystem>
#include <string_view>

#define PRIVATE_KEYFILE ".keys/my.private_key"

std::string private_key;


void generate_keys() {
    std::string public_key(crypto_sign_PUBLICKEYBYTES, '\0');

    private_key.resize(crypto_sign_SECRETKEYBYTES, '\0');

    crypto_sign_keypair(reinterpret_cast<unsigned char*>(public_key.data()), 
            reinterpret_cast<unsigned char*>(private_key.data()));

    std::filesystem::create_directory(".keys");

    std::ofstream private_keyfile(PRIVATE_KEYFILE);
    private_keyfile << macaron::Base64::Encode(private_key);
    std::ofstream public_keyfile(".keys/my.public_key");
    public_keyfile << macaron::Base64::Encode(public_key);

    std::cout << "\nKeys written to " PRIVATE_KEYFILE " and .keys/my.public_key\n\n";
    std::cout << "Public key: " << macaron::Base64::Encode(public_key) << "\n" << std::endl;
}



bool validate_signature(std::tm &t, const std::string &message, 
        const std::string &public_key, const std::string &signature) {
    std::stringstream ss;

    std::time_t time = std::mktime(&t);
    std::tm utc = *std::gmtime(&time);
    ss << std::put_time(&utc, "%Y-%m-%d %H:%M") << ' ' << message;
    
    std::string line = ss.str();
    //std::cerr << "Validating: '" << line << "'\n";
    return crypto_sign_verify_detached(reinterpret_cast<const unsigned char*>(signature.data()), 
            reinterpret_cast<unsigned char*>(line.data()), line.size(), 
            reinterpret_cast<const unsigned char*>(public_key.data())) == 0;   
}



bool parse_received_message(const std::string &line) {
    std::string user;
    char separator;
    std::stringstream line_ss(line);
    std::tm t = {};
    std::time_t now_time = std::time(nullptr);
    std::tm now = *std::localtime(&now_time);

    // parse date/time prefix
    if(!(line_ss >> std::get_time(&t, "%Y-%m-%d %H:%M:%S"))) {
        line_ss.clear();
        line_ss.str(line);

        if(!(line_ss >> std::get_time(&t, "%H:%M:%S")))
            return false;

        t.tm_year = now.tm_year;
        t.tm_mon = now.tm_mon;
        t.tm_mday = now.tm_mday;
    }

    t.tm_isdst = now.tm_isdst;

    // parse sender
    if(!(line_ss >> user >> separator))
        return false;
    
    if(user[0] == '<')
        user.erase(0);
    if(user[0] == '+' || user[0] == '@')
        user.erase(0);
    if(user.back() == '>')
        user.pop_back();

    // get sender's public key
    std::ifstream pub_keyfile(".keys/" + user + ".pubkey");
    if(!pub_keyfile) {
        std::cout << "No public key for " << user << std::endl;
        return true;
    }

    std::string public_key_str;
    if(!(pub_keyfile >> public_key_str)) {
        std::cout << "Empty public key file for " << user << std::endl;
        return true;
    }

    std::string public_key;
    try {
        public_key = macaron::Base64::Decode(public_key_str);
        if(public_key.size() != crypto_sign_PUBLICKEYBYTES) {
            std::cout << "Invalid public key length for " << user << std::endl;
            return true;
        }
    } catch(std::invalid_argument &) {
        std::cout << "Invalid public key file for " << user << std::endl;
        return true;
    }

    // parse message content
    if(separator != '|' && separator != '>')
        line_ss.putback(separator);
    
    std::string message = line_ss.str().substr(line_ss.tellg());

    std::size_t not_space = message.find_first_not_of(" ");
    if(not_space != std::string::npos) {
        message = message.substr(not_space);
        message = message.substr(0, message.find_last_not_of(" ") + 1);
    }

    // extract signature
    std::size_t last_space = message.find_last_of(" ");
    if(last_space == std::string::npos) {
        std::cout << "No signature on message" << std::endl;
        return true;
    }

    std::string signature;
    try {
        signature = macaron::Base64::Decode(message.substr(last_space + 1));
        if(signature.size() != crypto_sign_BYTES) {
            std::cout << "Signature is wrong length" << std::endl;
            return false;
        }
    } catch(std::invalid_argument &) {
        std::cout << "Signature is not valid base64 encoding" << std::endl;
        return false;
    }

    message = message.substr(0, last_space);

    // validate signature
    t.tm_sec = 0;
    t.tm_min -= 5;
    int attempts = 0;

    while(!validate_signature(t, message, public_key, signature)) {
        t.tm_min++;
        if(++attempts == 11) {
            std::cout << "INVALID SIGNATURE" << std::endl;
            return true;
        }
    }

    std::cout << "Valid signature" << std::endl;
    return true;
}


void sign_message(const std::string &message) {
    std::time_t now_time = std::time(nullptr);
    std::tm utc = *std::gmtime(&now_time);

    std::stringstream ss;
    ss << std::put_time(&utc, "%Y-%m-%d %H:%M") << ' ' << message;

    std::string signature(crypto_sign_BYTES, '\0');
    std::string line = ss.str();

    //std::cerr << "signing: '" << line << "'\n";
    crypto_sign_detached(reinterpret_cast<unsigned char*>(signature.data()), nullptr, 
            reinterpret_cast<unsigned char*>(line.data()), line.size(), 
            reinterpret_cast<unsigned char*>(private_key.data()));

    std::cout << '\n' << message << ' ' << macaron::Base64::Encode(signature) << '\n' << std::endl;
}


int main(int argc, char **argv) {
    if(argc > 1) {
        if(argc > 2 || argv[1] != std::string_view("--gen-key")) {
            std::cout << "\nUsage: " << argv[0] << " [--gen-key]\n\n";
            std::cout << "Enter either a message to sign or a message to verify via stdin\n\n";
            std::cout << "Messages of the format `yyyy-mm-dd hh:mm:ss sender | message signature`\n";
            std::cout << "   or of the format `hh::mm:ss sender | message signature`\n";
            std::cout << "   will be interpreted as a message whose signature you wish to be verified.\n";
            std::cout << "   The current date is assumed if not specified. The | separator is optional.\n";
            std::cout << "   The provided timestamp is assumed to be local time, which is then\n";
            std::cout << "   converted to UTC before validating. A skew of about 5 minutes is allowed.\n\n";
            std::cout << "Otherwise, the message is assumed to be a message you wish to append a signature to.\n" << std::endl;
            return 1;
        }

        generate_keys();
        return 0;
    }

    std::ifstream private_keyfile(PRIVATE_KEYFILE);
    if(!private_keyfile) {
        private_keyfile.close();
        private_keyfile.clear();
        std::cout << "\nNo private key found. Generating...\n\n";
        generate_keys();

        private_keyfile.open(PRIVATE_KEYFILE);
        if(!private_keyfile) {
            std::cout << "Unable to access " PRIVATE_KEYFILE "\n" << std::endl;
            return 2;
        }

        try {

        } catch(std::invalid_argument &) {
        }
    }

    std::string private_key_str;
    if(!(private_keyfile >> private_key_str)) {
        std::cout << "Empty private key file! Use --gen-key to generate a new one." << std::endl;
        return 3;
    }

    try {
        private_key = macaron::Base64::Decode(private_key_str);
        if(private_key.size() != crypto_sign_SECRETKEYBYTES) {
            std::cout << "Invalid private key length! Use --gen-key to generate a new one." << std::endl;
            return 4;
        }
    } catch(std::invalid_argument &) {
        std::cout << "Invalid private key file! Use --gen-key to generate a new one." << std::endl;
        return 5;
    }


    std::string line;

    while(std::getline(std::cin, line)) {
        if(!parse_received_message(line)) {
            sign_message(line);
        }
    }

    return 0;
}
