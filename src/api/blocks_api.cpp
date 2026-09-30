#include "blocks_api.h"

#include "crypto/block_hash.h"
#include "node/node.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <sstream>
#include <vector>

namespace bitcoin2max {

BlocksApi::BlocksApi(const Node& node)
    : node_(node) {}

std::string BlocksApi::getBlocksJson(std::size_t limit) const {
    std::ostringstream json;

    const uint64_t bestHeight = node_.bestHeight();

    json << "[";

    if (bestHeight > 0 && limit > 0) {
        const std::size_t count =
            std::min<std::size_t>(limit, bestHeight + 1);

        bool first = true;

        for (std::size_t i = 0; i < count; ++i) {
            const uint64_t height =
                bestHeight - static_cast<uint64_t>(i);

            const std::string headerHex =
                node_.getBlockHeaderHex(height);

            if (headerHex.empty() || headerHex.size() != 160) {
                continue;
            }

            std::vector<uint8_t> header;
            header.reserve(80);

            try {
                for (std::size_t pos = 0; pos < headerHex.size(); pos += 2) {
                    if (!std::isxdigit(
                            static_cast<unsigned char>(headerHex[pos])) ||
                        !std::isxdigit(
                            static_cast<unsigned char>(headerHex[pos + 1]))) {
                        header.clear();
                        break;
                    }

                    header.push_back(static_cast<uint8_t>(
                        std::stoul(headerHex.substr(pos, 2), nullptr, 16)
                    ));
                }
            } catch (...) {
                continue;
            }

            if (header.size() != 80) {
                continue;
            }

            const std::string hash = crypto::blockHashHex(header);

            if (hash.empty()) {
                continue;
            }

            if (!first) {
                json << ",";
            }

            json << "{"
                 << "\"height\":" << height << ","
                 << "\"hash\":\"" << hash << "\","
                 << "\"headerHex\":\"" << headerHex << "\""
                 << "}";

            first = false;
        }
    }

    json << "]";

    return json.str();
}

} // namespace bitcoin2max
