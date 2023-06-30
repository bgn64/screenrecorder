#include "pch.h"
#include "ServerHandle.h"
#include "Server.h"
#include "Pipe.h"

ServerHandle::ServerHandle()
{
}

bool ServerHandle::try_init()
{
    return m_pipe.try_init(L"myPipe", Pipe::CLIENT);
}

ServerHandle::~ServerHandle()
{
}

void ServerHandle::start(int framerate, int framesBufferSize, int monitorIndex)
{
    std::string request = std::to_string(Server::START) + " " + std::to_string(framerate) + " " + std::to_string(framesBufferSize) + " " + std::to_string(monitorIndex);
    m_pipe.send(request);

    CommandLine cmd(m_pipe.receive());

    int response;

    if (!cmd.TryGetAsInt(0, response))
    {
        throw std::runtime_error(cmd.Get(0) + " is not a valid server response");
    }

    switch (response) 
    {
    case Server::SUCCESS:
        break;
    case Server::FAILURE:
        throw std::invalid_argument("Received the following error message from server: " + cmd.Get(1, cmd.Size()));

        break;
    default:
        throw std::runtime_error(cmd.Get(0) + " is not a valid server response");
    }
}


void ServerHandle::stop(const std::string& folderPath)
{
    std::string request = std::to_string(Server::STOP) + " " + folderPath;
    m_pipe.send(request);

    CommandLine cmd(m_pipe.receive());

    int response;

    if (!cmd.TryGetAsInt(0, response))
    {
        throw std::runtime_error(cmd.Get(0) + " is not a valid server response");
    }

    switch (response) 
    {
    case Server::SUCCESS:
        break;
    case Server::FAILURE:
        throw std::invalid_argument("Received the following error message from server: " + cmd.Get(1, cmd.Size()));

        break;
    default:
        throw std::runtime_error(cmd.Get(0) + " is not a valid server response");
    }
}

void ServerHandle::cancel()
{
    std::string request = std::to_string(Server::CANCEL);
    m_pipe.send(request);

    CommandLine cmd(m_pipe.receive());

    int response;

    if (!cmd.TryGetAsInt(0, response))
    {
        throw std::runtime_error(cmd.Get(0) + " is not a valid server response");
    }

    switch (response) 
    {
    case Server::SUCCESS:
        break;
    case Server::FAILURE:
        throw std::invalid_argument("Received the following error message from server: " + cmd.Get(1, cmd.Size()));

        break;
    default:
        throw std::runtime_error(cmd.Get(0) + " is not a valid server response");
    }
}

void ServerHandle::disconnect()
{
    std::string request = std::to_string(Server::DISCONNECT);
    m_pipe.send(request);

    CommandLine cmd(m_pipe.receive());

    int response;

    if (!cmd.TryGetAsInt(0, response))
    {
        throw std::runtime_error(cmd.Get(0) + " is not a valid server response");
    }

    switch (response) 
    {
    case Server::SUCCESS:
        break;
    case Server::FAILURE:
        throw std::invalid_argument("Received the following error message from server: " + cmd.Get(1, cmd.Size()));

        break;
    default:
        throw std::runtime_error(cmd.Get(0) + " is not a valid server response");
    }
}

void ServerHandle::kill()
{
    std::string request = std::to_string(Server::KILL);
    m_pipe.send(request);

    CommandLine cmd(m_pipe.receive());

    int response;

    if (!cmd.TryGetAsInt(0, response))
    {
        throw std::runtime_error(cmd.Get(0) + " is not a valid server response");
    }

    switch (response) 
    {
    case Server::SUCCESS:
        break;
    case Server::FAILURE:
        throw std::invalid_argument("Received the following error message from server: " + cmd.Get(1, cmd.Size()));

        break;
    default:
        throw std::runtime_error(cmd.Get(0) + " is not a valid server response");
    }
}