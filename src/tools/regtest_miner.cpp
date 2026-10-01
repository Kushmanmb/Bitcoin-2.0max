#include "consensus/regtest_chain.h"

#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
class FileLock {
public:
    FileLock(const std::string& path, bool create) {
        fd_ = ::open(path.c_str(), O_RDWR | O_NOFOLLOW | (create ? O_CREAT : 0), 0600);
        if (fd_ < 0) throw std::runtime_error("Cannot open chain file");
        struct stat status{};
        if (::fstat(fd_, &status) != 0 || !S_ISREG(status.st_mode)) {
            ::close(fd_);
            throw std::runtime_error("Chain path must be a regular file");
        }
        if (::flock(fd_, LOCK_EX | LOCK_NB) != 0) {
            ::close(fd_);
            throw std::runtime_error("Chain file is in use by another process");
        }
    }
    ~FileLock() { ::close(fd_); }
    FileLock(const FileLock&) = delete;
    FileLock& operator=(const FileLock&) = delete;
    void sync() const {
        if (::fsync(fd_) != 0) throw std::runtime_error("Cannot sync chain file");
    }
private:
    int fd_{-1};
};
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--help") {
            std::cout << "Private, coinbase-only regtest harness. No network or wallet.\n"
                      << "Usage: bitcoin2max_regtest <chain-file> [--mine <1..10000>]\n"
                      << "Without --mine, revalidate the saved chain.\n";
            return 0;
        }
        if (argc != 2 && argc != 4) throw std::runtime_error("Use --help for usage");
        uint64_t count = 0;
        if (argc == 4) {
            const std::string number = argv[3];
            if (std::string(argv[2]) != "--mine" || number.empty() ||
                number.find_first_not_of("0123456789") != std::string::npos)
                throw std::runtime_error("Expected --mine followed by a positive integer");
            count = std::stoull(number);
            if (count == 0 || count > 10'000) throw std::runtime_error("Mine count must be 1..10000");
        }
        const std::string path = argv[1];
        FileLock lock(path, count != 0);
        std::ifstream input(path);
        if (!input) throw std::runtime_error("Cannot read chain file");
        // An empty file is initialized only by an explicit mining request.
        const bool empty = input.peek() == std::ifstream::traits_type::eof();
        if (empty && !count) throw std::runtime_error("Empty chain file; use --mine to initialize");
        auto chain = empty ? bitcoin2max::regtest::Chain{} :
                             bitcoin2max::regtest::Chain::load(input);
        input.close();
        if (count > bitcoin2max::regtest::MAX_TEST_HEIGHT - chain.height())
            throw std::runtime_error("Requested mining exceeds regtest height limit");
        if (count) {
            std::ofstream output(path, std::ios::app);
            if (!output) throw std::runtime_error("Cannot append to chain file");
            if (empty) output << "B2MX-REGTEST-V1\n";
            for (uint64_t i = 0; i < count; ++i) {
                auto block = bitcoin2max::regtest::mineBlock(chain.height() + 1, chain.tip());
                const auto result = chain.append(block);
                if (!result.valid) throw std::runtime_error(result.error);
                bitcoin2max::regtest::Chain::writeRecord(output, chain.height(), block);
            }
            output.flush();
            if (!output) throw std::runtime_error("Cannot flush chain file");
            lock.sync();
        }
        std::cout << "network=bitcoin2max-private-regtest\n"
                  << "height=" << chain.height() << '\n'
                  << "tip=" << bitcoin2max::regtest::displayHash(chain.tip()) << '\n'
                  << "issued_satoshis=" << chain.issued() << '\n'
                  << "mature_reward_satoshis=" << chain.matureRewards() << '\n'
                  << "Test rewards only. Wallet spending and P2P sync are not implemented.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Regtest error: " << error.what() << '\n';
        return 1;
    }
}
