#pragma once

#include "pch.h"
#include "Pipe.h"

// The purpose of this class is to implement RPC's for the Server.h class. It allows a client to send recording requests to the server.
class ServerHandle {
public:
    ServerHandle();
    ~ServerHandle();

    bool try_init();

    /**
     * @throws std::ios_base::failure if communication with server fails unexpectedly
     * @throws std::invalid_argument if we receive an error from the server
     * @throws std::runtime_error if server responds with an unknown format
     */
    void start(int framerate, int framesBufferSize, int monitorIndex);

    /**
     * @throws std::ios_base::failure if communication with server fails unexpectedly
     * @throws std::invalid_argument if we receive an error from the server
     * @throws std::runtime_error if server responds with an unknown format
     */
    void stop(const std::string& folderPath);

    /**
     * @throws std::ios_base::failure if communication with server fails unexpectedly
     * @throws std::runtime_error if server responds with an unknown format
     */
    void cancel();

    /**
     * @throws std::ios_base::failure if communication with server fails unexpectedly
     * @throws std::runtime_error if server responds with an unknown format
     */
    void disconnect();

    /**
     * @throws std::ios_base::failure if communication with server fails unexpectedly
     * @throws std::runtime_error if server responds with an unknown format
     */
    void kill();

private:
    Pipe m_pipe;
};