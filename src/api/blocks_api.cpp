#include "blocks_api.h"

#include "node/node.h"

#include <algorithm>
#include <sstream>

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

            if (headerHex.empty()) {
                continue;
            }

            if (!first) {
                json << ",";
            }

            json << "{"
                 << "\"height\":" << height << ","
                 << "\"headerHex\":\"" << headerHex << "\""
                 << "}";

            first = false;
        }
    }

    json << "]";

    return json.str();
}

} // namespace bitcoin2max
