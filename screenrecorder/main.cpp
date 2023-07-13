#include "pch.h"
#include "Server.h"
#include "Client.h"
#include "CommandLine.h"
#include "Request.h"
#include "Response.h"

TRACELOGGING_DEFINE_PROVIDER(
    g_hMyComponentProvider,
    "ScreenRecorder",
    // Human-readable guid: fe8fc3d0-1e6a-42f2-be28-9f8a0fcf7b04
    (0xfe8fc3d0, 0x1e6a, 0x42f2, 0xbe, 0x28, 0x9f, 0x8a, 0x0f, 0xcf, 0x7b, 0x04));

const std::string helpMessage = "\n\tUsage: screenrecorder.exe options ...\n\n"
"\t-help start\t- for screen recording start command\n"
"\t-help stop\t- for screen recording stop commands\n";

const std::string startHelpMessage = "\n  screenrecorder.exe -start ...        Starts screen recording.\n"
"\tUsage:\tscreenrecorder.exe -start [-framerate <framerate>] [-framebuffer <# of frames>] [-monitor <monitor # to record>]\n"
"\tEx>\tscreenrecorder.exe -start -framerate 10\n"
"\tEx>\tscreenrecorder.exe -start -framerate 1 -framebuffer 5 -monitor 0\n\n"
"\t-framerate\tSpecifies the rate at which screenshots will be taken, in frames per second.\n"
"\t-framebuffer\tSpecifies the size of the circular memory buffer in which to store screenshots, in number of screenshots.\n"
"\t-framebuffer\tSpecifies which monitor screen to record, as an integer.\n";

const std::string stopHelpMessage = "\n  screenrecorder.exe -stop ...         Stops screen recording saves all screenshots in buffer to a folder.\n"
"\tUsage:\tscreenrecorder.exe -stop <recording folder>\n"
"\tEx>\tscreenrecorder.exe -stop \"D:\\screenrecorder\"\n"
"\n  screenrecorder.exe -cancel ...       Cancels the screen recording.\n"
"\tUsage:\tscreenrecorder.exe -cancel\n";

const std::string invalidCommandSynatxMessage = "\b\tInvalid command syntax.\n";

const std::string recordingAlreadyStarted = "\b\tThere is already a recording in process.\n";
const std::string recordingNotStartedMessage = "\b\tThere is no recording in process.\n";

const std::string failedToCommunicateWithServerProcessMessage = "\b\tFailed to communicate with recording process.\n";
const std::string failedToCreateServerProcessMessage = "\b\tFailed to create the recording process.\n";
const std::string failedToConnectToServerProcessMessage = "\b\tFailed to connect to the recording process.\n";

const std::string unknownEnumCaseMessage = "\b\tReceived an unknown response from the recording process.\n";
const std::string defaultEnumCaseMessage = "\b\tReceived an unhandled response from the recording process.\n";

const std::string defaultSeverExceptioinMessage = "\b\tReceived an unknown exception from the recording process.\n";

bool TryCreateRecordingProcess()
{
    TCHAR szPath[MAX_PATH];

    if (!GetModuleFileName(NULL, szPath, MAX_PATH))
    {
        return false;
    }

    std::wstring cmdLine = std::wstring(szPath) + L" -newserver";
    STARTUPINFO info = { sizeof(info) };
    info.dwFlags = STARTF_USESTDHANDLES;
    info.hStdOutput = NULL;
    info.hStdError = NULL;
    PROCESS_INFORMATION processInfo;

    if (!CreateProcess(szPath, &cmdLine[0], NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo))
    {
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);

        return false;
    }

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    return true;
}

void start(CommandLine& commandLine)
{
    int framerate, monitorIndex, bufferCapacity;
    bool isMegabytes;

    try
    {
        commandLine.GetStartArgs(framerate, monitorIndex, bufferCapacity, isMegabytes);
    }
    catch (const std::invalid_argument& e)
    {
        std::cout << invalidCommandSynatxMessage << std::endl;
        std::cout << startHelpMessage << std::endl;

        return;
    }

    Request startRequest = Request::BuildStartRequest(framerate, monitorIndex, bufferCapacity, isMegabytes);
    Request disconnectRequest = Request::BuildDisconnectRequest();
    Request killRequest = Request::BuildKillRequest();
    Response response;
    Client client;

    if (client.try_connect())
    {
        std::cout << recordingAlreadyStarted << std::endl;

        try
        {
            client.send(disconnectRequest);
        }
        catch (const std::ios_base::failure& e)
        {
            std::cout << failedToCommunicateWithServerProcessMessage << std::endl;
        }

        return;
    }

    if (!TryCreateRecordingProcess())
    {
        std::cout << failedToCreateServerProcessMessage << std::endl;

        return;
    }

    if (!client.try_connect(3))
    {
        std::cout << failedToConnectToServerProcessMessage << std::endl;

        return;
    }

    try
    {
        response = client.send(startRequest);
    }
    catch (const std::ios_base::failure& e)
    {
        std::cout << failedToCommunicateWithServerProcessMessage << std::endl;

        return;
    }

    std::exception e;

    switch (response.ParseResponseType())
    {
    case ResponseType::Success:
        break;
    case ResponseType::Exception:
        try
        {
            response.ParseExceptionArgs(e);

            std::cout << e.what() << std::endl;
        }
        catch (const std::invalid_argument& e)
        {
            std::cout << defaultSeverExceptioinMessage << std::endl;
        }

        try
        {
            client.send(killRequest);
        }
        catch (const std::ios_base::failure& e)
        {
            std::cout << failedToCommunicateWithServerProcessMessage << std::endl;
        }

        return;
    case ResponseType::Unknown:
        std::cout << unknownEnumCaseMessage << std::endl;

        try
        {
            client.send(killRequest);
        }
        catch (const std::ios_base::failure& e)
        {
            std::cout << failedToCommunicateWithServerProcessMessage << std::endl;
        }

        return;
    default:
        std::cout << defaultEnumCaseMessage << std::endl;

        try
        {
            client.send(killRequest);
        }
        catch (const std::ios_base::failure& e)
        {
            std::cout << failedToCommunicateWithServerProcessMessage << std::endl;
        }

        return;
    }

    try
    {
        client.send(disconnectRequest);
    }
    catch (const std::ios_base::failure& e)
    {
        std::cout << failedToCommunicateWithServerProcessMessage << std::endl;
    }
}

void stop(CommandLine& commandLine)
{
    std::string folder;

    try
    {
        commandLine.GetStopArgs(folder);
    }
    catch (const std::invalid_argument& e)
    {
        std::cout << invalidCommandSynatxMessage << std::endl;
        std::cout << stopHelpMessage << std::endl;

        return;
    }

    Request stopRequest = Request::BuildStopRequest(folder);
    Request disconnectRequest = Request::BuildDisconnectRequest();
    Request killRequest = Request::BuildKillRequest();
    Response response;
    Client client;

    if (!client.try_connect())
    {
        std::cout << recordingNotStartedMessage << std::endl;

        return;
    }

    try
    {
        response = client.send(stopRequest);
    }
    catch (const std::ios_base::failure& e)
    {
        std::cout << failedToCommunicateWithServerProcessMessage << std::endl;

        return;
    }

    std::exception e;

    switch (response.ParseResponseType())
    {
    case ResponseType::Success:
        break;
    case ResponseType::Exception:
        try
        {
            response.ParseExceptionArgs(e);

            std::cout << e.what() << std::endl;
        }
        catch (const std::invalid_argument& e)
        {
            std::cout << defaultSeverExceptioinMessage << std::endl;
        }

        try
        {
            client.send(disconnectRequest);
        }
        catch (const std::ios_base::failure& e)
        {
            std::cout << failedToCommunicateWithServerProcessMessage << std::endl;
        }

        return;
    case ResponseType::Unknown:
        std::cout << unknownEnumCaseMessage << std::endl;

        try
        {
            client.send(disconnectRequest);
        }
        catch (const std::ios_base::failure& e)
        {
            std::cout << failedToCommunicateWithServerProcessMessage << std::endl;
        }

        return;
    default:
        std::cout << defaultEnumCaseMessage << std::endl;

        try
        {
            client.send(disconnectRequest);
        }
        catch (const std::ios_base::failure& e)
        {
            std::cout << failedToCommunicateWithServerProcessMessage << std::endl;
        }

        return;
    }

    try
    {
        client.send(killRequest);
    }
    catch (const std::ios_base::failure& e)
    {
        // print could not commnicate with server or something
    }
}

void cancel(CommandLine& commandLine)
{
    Request request = Request::BuildCancelRequest();
    Request disconnectRequest = Request::BuildDisconnectRequest();
    Request killRequest = Request::BuildKillRequest();
    Response response;
    Client client;

    if (!client.try_connect()) 
    {
        std::cout << recordingNotStartedMessage << std::endl;

        return;
    }

    try
    {
        response = client.send(request);
    }
    catch (const std::ios_base::failure& e)
    {
        std::cout << failedToCommunicateWithServerProcessMessage << std::endl;

        return;
    }

    std::exception e;

    switch (response.ParseResponseType())
    {
    case ResponseType::Success:
        break;
    case ResponseType::Exception:
        try
        {
            response.ParseExceptionArgs(e);

            std::cout << e.what() << std::endl;
        }
        catch (const std::invalid_argument& e)
        {
            std::cout << defaultSeverExceptioinMessage << std::endl;
        }

        try
        {
            client.send(disconnectRequest);
        }
        catch (const std::ios_base::failure& e)
        {
            std::cout << failedToCommunicateWithServerProcessMessage << std::endl;
        }

        return;
    case ResponseType::Unknown:
        std::cout << unknownEnumCaseMessage << std::endl;

        try
        {
            client.send(disconnectRequest);
        }
        catch (const std::ios_base::failure& e)
        {
            std::cout << failedToCommunicateWithServerProcessMessage << std::endl;
        }

        return;
    default:
        std::cout << defaultEnumCaseMessage << std::endl;

        try
        {
            client.send(disconnectRequest);
        }
        catch (const std::ios_base::failure& e)
        {
            std::cout << failedToCommunicateWithServerProcessMessage << std::endl;
        }

        return;
    }

    try 
    {
        client.send(killRequest);
    }
    catch (const std::ios_base::failure& e)
    {
        std::cout << failedToCommunicateWithServerProcessMessage << std::endl;
    }
}

void new_server()
{
    Request disconnectRequest = Request::BuildDisconnectRequest();
    Response response;
    Client client;

    if (client.try_connect())
    {
        try
        {
            client.send(disconnectRequest);
        }
        catch (const std::ios_base::failure& e)
        {
        }

        return;
    }

    std::unique_ptr<Server> server = std::make_unique<Server>();

    if (!server->try_init())
    {
        return;
    }

    server->run();
}

int main(int argc, char* argv[])
{
    CommandLine commandLine(argc, argv);

    switch (commandLine.GetCommandType())
    {
        case CommandType::Start:
            start(commandLine);

            break;
        case CommandType::Stop:
            stop(commandLine);

            break;
        case CommandType::Cancel:
            cancel(commandLine);

            break;
        case CommandType::NewServer:
            new_server();

            break;
        case CommandType::Unknown:
        default:
            std::cout << invalidCommandSynatxMessage << std::endl;
            std::cout << helpMessage << std::endl;
    }
}