#pragma once

#include "pch.h"
#include "SimpleCapture.h"
#include "Pipe.h"
#include "CommandLine.h"

// The purpose of this class is to receive screen recording requests from clients and proces them.
class Server {
public:
    enum Request { START, STOP, CANCEL, DISCONNECT, KILL };
    enum Response { SUCCESS, FAILURE };
    Server();
    ~Server();

    bool try_init();

    /**
     * @throws std::ios_base::failure if server fails unexpectedly
     */
    void run();

private:
    std::string start(const CommandLine& cmd);
    std::string stop(const CommandLine& cmd);
    std::string cancel(const CommandLine& cmd);
    std::string disconnect(const CommandLine& cmd);
    std::string kill(const CommandLine& cmd);
    Pipe m_pipe;
    std::unique_ptr<SimpleCapture> m_simpleCapture;
    bool isCapturing;
    bool isDisconnect;
    bool isKill;
};