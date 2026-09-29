#include "blocks_api.h"

#include "node/node.h"

#include <sstream>

namespace bitcoin2max {

BlocksApi::BlocksApi(const Node& node)
    : node_(node) {}

std::string BlocksApi::getBlocksJson(std::size_t limit) const {
    std::ostringstream json;

    const uint64_t height = node_.bestHeight();
const std::string bestHash = node_.bestBlockHash();

json << "[";

    if (height > 0 && limit > 0) {
    const std::string headerHex =
        node_.getBlockHeaderHex(height);

    json << "{"
         << "\"height\":" << height << ","
         << "\"hash\":\"" << bestHash << "\","
         << "\"headerHex\":\"" << headerHex << "\""
         << "}";
}

    json << "]";

    return json.str();
}

} // namespace bitcoin2max
