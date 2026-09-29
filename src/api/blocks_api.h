#pragma once

#include <cstddef>
#include <string>

namespace bitcoin2max {

class Node;

class BlocksApi {
public:
    explicit BlocksApi(const Node& node);

    // Returns a JSON array containing recent block information.
    std::string getBlocksJson(std::size_t limit = 10) const;

private:
    const Node& node_;
};

} // namespace bitcoin2max
