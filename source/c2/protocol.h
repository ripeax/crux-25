#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <string>
#include <vector>
#include "json.hpp"

using json = nlohmann::json;

// Command Structure (C2 -> Agent)
struct Command {
    std::string id;
    std::string type;
    std::vector<std::string> args;
    int ttl;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Command, id, type, args, ttl)
};

struct CommandBatch {
    std::string batch_id;
    uint64_t timestamp;
    std::vector<Command> commands;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CommandBatch, batch_id, timestamp, commands)
};

// Log Structure (Agent -> C2)
struct CommandResult {
    std::string task_id;
    std::string status;
    std::string output;
    std::string error;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CommandResult, task_id, status, output, error)
};

struct LogBatch {
    std::string agent_id;
    uint64_t timestamp;
    std::vector<CommandResult> results;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(LogBatch, agent_id, timestamp, results)
};

#endif // PROTOCOL_H
