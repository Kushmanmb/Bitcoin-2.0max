#pragma once

#include <string>

namespace bitcoin2max {

class Node;

// Builds the public, read-only JSON representation of node status.
// This exposes operational information only — never private keys,
// wallet secrets, RPC credentials, or signing functionality.
class StatusApi {
public:
    explicit StatusApi(const Node& node);

    std::string getStatusJson() const;

private:
    const Node& node_;
};

} // namespace bitcoin2max
