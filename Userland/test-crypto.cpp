#include <LibCore/ArgsParser.h>
#include <LibCore/File.h>
#include <LibCrypto/Cipher/AES.h>
#include <LibLine/Editor.h>
#include <stdio.h>

static const char* secret_key = "WellHelloFreinds";
static const char* cipher = "AES_CBC";
static const char* filename = nullptr;
static int key_bits = 128;
static bool encrypting = false;
static bool binary = false;
static bool interactive = false;

void print_buffer(const ByteBuffer& buffer, size_t split)
{
    for (size_t i = 0; i < buffer.size(); ++i) {
        if (i % split == 0 && i)
            puts("");
        printf("%02x", buffer[i]);
    }
    puts("");
}

void aes_cbc(const char* message, size_t len)
{
    auto buffer = ByteBuffer::wrap(message, len);
    // FIXME: Take iv as an optional parameter
    auto iv = ByteBuffer::create_zeroed(Crypto::AESCipher::block_size());

    if (encrypting) {
        Crypto::AESCipher::CBCMode cipher(secret_key, key_bits, Crypto::Intent::Encryption);

        auto enc = cipher.create_aligned_buffer(buffer.size());
        cipher.encrypt(buffer, enc, iv);

        if (binary)
            printf("%.*s", (int)enc.size(), enc.data());
        else
            print_buffer(enc, Crypto::AESCipher::block_size());
    } else {
        Crypto::AESCipher::CBCMode cipher(secret_key, key_bits, Crypto::Intent::Decryption);
        auto dec = cipher.create_aligned_buffer(buffer.size());
        cipher.decrypt(buffer, dec, iv);
        printf("%.*s\n", (int)dec.size(), dec.data());
    }
}

auto main(int argc, char** argv) -> int
{
    Core::ArgsParser parser;
    parser.add_option(secret_key, "Set the secret key (must be key-bits bits)", "secret-key", 'k', "secret key");
    parser.add_option(key_bits, "Size of the key", "key-bits", 'b', "key-bits");
    parser.add_option(filename, "Read from file", "file", 'f', "from file");
    parser.add_option(encrypting, "Encrypt the message", "encrypt", 'e');
    parser.add_option(binary, "Force binary output", "force-binary", 0);
    parser.add_option(interactive, "Force binary output", "interactive", 'i');
    parser.parse(argc, argv);

    if (StringView(cipher) == "AES_CBC") {
        if (!Crypto::AESCipher::KeyType::is_valid_key_size(key_bits)) {
            printf("Invalid key size for AES: %d\n", key_bits);
            return 1;
        }
        if (strlen(secret_key) != (size_t)key_bits / 8) {
            printf("Key must be exactly %d bytes long\n", key_bits / 8);
            return 1;
        }
        if (interactive) {
            Line::Editor editor;
            editor.initialize();
            for (;;) {
                auto line = editor.get_line("> ");
                aes_cbc(line.characters(), line.length());
            }
        } else {
            if (filename == nullptr) {
                puts("must specify a file name");
                return 1;
            }
            if (!Core::File::exists(filename)) {
                puts("File does not exist");
                return 1;
            }
            auto file = Core::File::open(filename, Core::IODevice::OpenMode::ReadOnly);
            auto buffer = file->read_all();
            aes_cbc((const char*)buffer.data(), buffer.size());
        }
    } else {
        printf("Unknown cipher suite '%s'", cipher);
        return 1;
    }

    return 0;
}
