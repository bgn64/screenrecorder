#pragma once

#include "pch.h"

// The purpose of this class is to wrap communication via a named pipe. It allows a server and client to send messages to each other.
class Pipe {
public:
    enum Mode { SERVER, CLIENT };
    Pipe();
    ~Pipe();
    bool try_init(const std::wstring& name, Mode mode);

    /**
     * @throws std::ios_base::failure if function fails
     */
    void send(const std::string& message);

    /**
     * @throws std::ios_base::failure if function fails
     */
    std::string receive();

    /**
     * @throws std::invalid_argument if called on a SERVER pipe
     * @throws std::ios_base::failure if attempt to disconnect fails
     */
    void disconnect();

    /**
     * @throws std::invalid_argument if called on a SERVER pipe
     * @throws std::ios_base::failure if attempt to connect fails
     */
    void connect();

private:
    std::wstring m_name;
    HANDLE m_hpipe;
    Mode m_mode;
};