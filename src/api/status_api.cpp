#include "status_api.h"

#include "../node/node.h"

#include <sstream>

namespace bitcoin2max {

StatusApi::StatusApi(const Node& node)
    : node_(node) {}

std::string StatusApi::getStatusJson() const {
    std::ostringstream json;

    json << "{";
    json << "\"status\":\"" << (node_.isRunning() ? "online" : "offline") << "\",";
    json << "\"running\":" << (node_.isRunning() ? "true" : "false") << ",";
    json << "\"blockHeight\":" << node_.bestHeight() << ",";
    json << "\"peers\":" << node_.peerCount() << ",";
    json << "\"mempool\":" << node_.mempoolSize() << ",";
    json << "\"electrumConnected\":"
         << (node_.electrumConnected() ? "true" : "false");
    json << "}";

    return json.str();
}

} // namespace bitcoin2max
