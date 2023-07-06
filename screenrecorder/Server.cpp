#include "pch.h"
#include "Server.h"
#include "ScreenRecordingToolProvider.h"
#include "Pipe.h"
#include "MonitorInfo.h"
#include "SimpleCapture.h"
#include "CommandLine.h"

#define HANDLE_EXCEPTION(pipe_function_call) \
    try \
    { \
        pipe_function_call; \
    } \
    catch (const std::ios_base::failure& e) \
    { \
        isDisconnect = true;\
        \
        continue; \
    } \

Server::Server() : isCapturing(false), isDisconnect(false), isKill(false)
{
    TraceLoggingRegister(g_hMyComponentProvider);
}

bool Server::try_init() 
{
    return m_pipe.try_init(L"myPipe", Pipe::SERVER);
}

Server::~Server() 
{
    TraceLoggingUnregister(g_hMyComponentProvider);
}

std::string Server::start(const CommandLine& cmd)
{
    if (cmd.Size() != 4)
    {
        return std::to_string(FAILURE) + " Server Usage: start <framerate> <framesBufferSize> <monitorIndex>";
    }

    if (isCapturing)
    {
        return std::to_string(FAILURE) + " Server Usage: recording already started";
    }

    int framerate;

    if (!cmd.TryGetAsInt(1, framerate))
    {
        return std::to_string(FAILURE) + " Server Usage: framerate must be and integer";
    }

    int framesBufferSize;

    if (!cmd.TryGetAsInt(2, framesBufferSize))
    {
        return std::to_string(FAILURE) + " Server Usage: framesBufferSize must be an integer";
    }

    int monitorIndex;
    std::vector<MonitorInfo> monitors = MonitorInfo::EnumerateAllMonitors(true);

    if (!cmd.TryGetAsInt(3, monitorIndex))
    {
        return std::to_string(FAILURE) + " Server Usage: monitorIndex must be an integer";
    }

    if (monitorIndex < 0 || monitors.size() <= monitorIndex) 
    {
        return std::to_string(FAILURE) + " Server Usage: monitorIndex out of range";
    }

    auto d3dDevice = util::CreateD3DDevice();
    auto dxgiDevice = d3dDevice.as<IDXGIDevice>();
    auto device = CreateDirect3DDevice(dxgiDevice.get());

    MonitorInfo monitorInfo = monitors[monitorIndex];
    auto item = util::CreateCaptureItemForMonitor(monitorInfo.MonitorHandle);

    auto buffer = CircularFrameBuffer::with_frame_capacity(framesBufferSize);
    m_simpleCapture = std::make_unique<SimpleCapture>(device, item, framerate, buffer);

    m_simpleCapture->StartCapture();
    isCapturing = true;

    return std::to_string(SUCCESS);
}


std::string Server::stop(const CommandLine& cmd)
{
    if (cmd.Size() != 2)
    {
        return std::to_string(FAILURE) + " Server Usage: stop <folderPath>";
    }

    if (!isCapturing)
    {
        return std::to_string(FAILURE) + " Server Usage: recording not started";
    }

    StorageFolder storageFolder = NULL;

    try 
    {
        storageFolder = StorageFolder::GetFolderFromPathAsync(winrt::to_hstring(cmd.Get(1))).get();
    }
    catch (const winrt::hresult_invalid_argument& e)
    {
        return std::to_string(FAILURE) + " Server Usage: invalid argument to open folder";
    }
    catch (const winrt::hresult_error& e)
    {
        return std::to_string(FAILURE) + " Server Usage: the specified path is invalid";
    }

    m_simpleCapture->CloseAndSave(storageFolder);
    isCapturing = false;
    
    return std::to_string(SUCCESS);
}

std::string Server::cancel(const CommandLine& cmd)
{
    if (!isCapturing)
    {
        return std::to_string(FAILURE) + " Server Usage: recording not started";
    }

    m_simpleCapture->Close();
    isCapturing = false;

    return std::to_string(SUCCESS);
}

std::string Server::disconnect(const CommandLine& cmd)
{
    isDisconnect = true;

    return std::to_string(SUCCESS);
}

std::string Server::kill(const CommandLine& cmd)
{
    isDisconnect = true;
    isKill = true;

    return std::to_string(SUCCESS);
}

void Server::run()
{
    while (!isKill)
    {
        std::cout << "Waiting for client " << std::endl;
        isDisconnect = false;

        try
        {
            m_pipe.connect();
        }
        catch (std::ios_base::failure)
        {
            return;
        }
        
        std::cout << "Connected to client " << std::endl;

        while (!isDisconnect)
        {
            std::cout << "\tWaiting for request from client " << std::endl;

            std::string input;
            HANDLE_EXCEPTION(input = m_pipe.receive());
            CommandLine cmd(input);
            
            std::cout << "\tReceived " << cmd.Get(0, cmd.Size()) << " from client" << std::endl;

            int request;

            if (!cmd.TryGetAsInt(0, request))
            {
                HANDLE_EXCEPTION(m_pipe.send(std::to_string(FAILURE) + " Server Usage: " + cmd.Get(0) + " is not a valid server request"));

                continue;
            }

            switch (request) 
            {
            case START:
                HANDLE_EXCEPTION(m_pipe.send(start(cmd)));

                break;
            case STOP:
                HANDLE_EXCEPTION(m_pipe.send(stop(cmd)));

                break;
            case CANCEL:
                HANDLE_EXCEPTION(m_pipe.send(cancel(cmd)));

                break;
            case DISCONNECT:
                HANDLE_EXCEPTION(m_pipe.send(disconnect(cmd)));

                break;
            case KILL:
                HANDLE_EXCEPTION(m_pipe.send(kill(cmd)));

                break;
            default:
                HANDLE_EXCEPTION(m_pipe.send(std::to_string(FAILURE) + " Server Usage: [start <framerate> | stop <folder> | cancel | disconnect | kill]"));
            }

            std::cout << std::endl;
        }

        try
        {
            m_pipe.disconnect();
        }
        catch (std::ios_base::failure)
        {
            return;
        }  
    }
}