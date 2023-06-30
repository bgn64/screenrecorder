#include "pch.h"
#include "Server.h"
#include "ServerHandle.h"

TRACELOGGING_DEFINE_PROVIDER(
    g_hMyComponentProvider,
    "ScreenRecordingTool",
    // Human-readable guid: fe8fc3d0-1e6a-42f2-be28-9f8a0fcf7b04
    (0xfe8fc3d0, 0x1e6a, 0x42f2, 0xbe, 0x28, 0x9f, 0x8a, 0x0f, 0xcf, 0x7b, 0x04));

#define HANDLE_EXCEPTIONS(serverHandle_function_call) \
    try \
    { \
        serverHandle_function_call; \
    } \
    catch (const std::invalid_argument& e) \
    { \
        std::cerr << "\n\tUnexpected error: " << e.what() << std::endl; \
        return; \
    } \
    catch (const std::ios_base::failure& e) \
    { \
        std::cerr << "\n\tUnexpected error: " << e.what() << std::endl; \
        return; \
    } \
    catch (const std::runtime_error& e) \
    { \
        std::cerr << "\n\tUnexpected error: " << e.what() << std::endl; \
        return; \
    }

const std::string helpMessage = "\n\tUsage: ScreenRecordingTool.exe options ...\n\n"
"\t-help start\t- for screen recording start command\n"
"\t-help stop\t- for screen recording stop commands\n";

const std::string startHelpMessage = "\n  ScreenRecordingTool.exe -start ...        Starts screen recording.\n"
"\tUsage:\tScreenRecordingTool.exe -start [-framerate <framerate>] [-framebuffer <# of frames>]\n"
"\tEx>\tScreenRecordingTool.exe -start -framerate 10\n"
"\tEx>\tScreenRecordingTool.exe -start -framerate 1 -framebuffer 5\n\n"
"\t-framerate\tSpecifies the rate at which screenshots will be taken, in frames per second.\n"
"\t-framebuffer\tSpecifies the size of the circular memory buffer in which to store screenshots, in number of screenshots.\n";

const std::string stopHelpMessage = "\n  ScreenRecordingTool.exe -stop ...         Stops screen recording saves all screenshots in buffer to a folder.\n"
"\tUsage:\tScreenRecordingTool.exe -stop <recording folder>\n"
"\tEx>\tScreenRecordingTool.exe -stop \"D:\\ScreenRecordingTool\"\n"
"\n  ScreenRecordingTool.exe -cancel ...       Cancels the screen recording.\n"
"\tUsage:\tScreenRecordingTool.exe -cancel\n";

const std::string invalidCommandSynatxMessage = "\b\tInvalid command syntax.";

void Start(int framerate, int framebuffer)
{
    // Check to make sure recording process does not exist before starting a recording

    std::unique_ptr<ServerHandle> serverHandle = std::make_unique<ServerHandle>();

    if (serverHandle->try_init())
    {
        std::cout << "\n\tThere is already a recording active." << std::endl;

        HANDLE_EXCEPTIONS(serverHandle->disconnect());

        return;
    }

    // Create recording process

    TCHAR szPath[MAX_PATH];

    if (!GetModuleFileName(NULL, szPath, MAX_PATH))
    {
        std::cerr << "\n\tUnexpected error: Failed to get module name." << std::endl;

        return;
    }

    std::wstring cmdLine = std::wstring(szPath) + L" -newserver";
    STARTUPINFO info = { sizeof(info) };
    info.dwFlags = STARTF_USESTDHANDLES;
    info.hStdOutput = NULL;
    info.hStdError = NULL;
    PROCESS_INFORMATION processInfo;

    if (!CreateProcess(szPath, &cmdLine[0], NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo))
    {
        std::cerr << "\n\tUnexpected error: Failed to create recording process." << std::endl;

        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);

        return;
    }

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    // Connect to recording process

    std::this_thread::sleep_for(std::chrono::seconds(1));
    int retries = 0;

    while (!serverHandle->try_init() && retries < 3)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        retries++;
    }

    if (retries == 3) 
    {
        std::cerr << "\n\tUnexpected error: Failed to connect to recording process." << std::endl;

        return;
    }

    HANDLE_EXCEPTIONS(serverHandle->start(framerate, framebuffer));
    HANDLE_EXCEPTIONS(serverHandle->disconnect());
}

void Stop(std::string folderPath)
{
    // Check to make sure recording process exists before telling it to stop recording

    std::unique_ptr<ServerHandle> serverHandle = std::make_unique<ServerHandle>();

    if (!serverHandle->try_init())
    {
        std::cout << "\n\tThere is no recording active." << std::endl;

        return;
    }

    // Tell recording process to stop recording

    HANDLE_EXCEPTIONS(serverHandle->stop(folderPath));
    HANDLE_EXCEPTIONS(serverHandle->kill());
}

void Cancel()
{
    // Check to make sure recording process exists before telling it to stop recording

    std::unique_ptr<ServerHandle> serverHandle = std::make_unique<ServerHandle>();

    if (!serverHandle->try_init())
    {
        std::cout << "\n\tThere is no recording active." << std::endl;

        return;
    }

    // Tell recording process to stop recording

    HANDLE_EXCEPTIONS(serverHandle->cancel());
    HANDLE_EXCEPTIONS(serverHandle->kill());
}

void NewServer()
{
    // Check to make sure recording process does not exist before starting a recording

    std::unique_ptr<ServerHandle> serverHandle = std::make_unique<ServerHandle>();

    if (serverHandle->try_init())
    {
        std::cout << "\n\tThere is already a server active." << std::endl;

        HANDLE_EXCEPTIONS(serverHandle->disconnect());

        return;
    }

    // Create and run server

    std::unique_ptr<Server> server = std::make_unique<Server>();

    if (!server->try_init()) 
    {
        std::cout << "\n\tUnexpected error: Failed to start server." << std::endl;

        return;
    }

    server->run();
}

void NewClient()
{
    // Create serverHandle

    std::unique_ptr<ServerHandle> serverHandle = std::make_unique<ServerHandle>();

    if (!serverHandle->try_init())
    {
        std::cout << "\n\tThere is no server active to conncect to." << std::endl;

        return;
    }
    while (true) 
    {
        std::string input;
        std::cout << ">> ";
        std::getline(std::cin, input);
        CommandLine cmd(input);

        if (cmd.Get(0) == "start") 
        {
            if (cmd.Size() != 3)
            {
                std::cout << "Usage: start <framerate> <framesBufferSize>" << std::endl;

                continue;
            }

            int framerate;

            if (!cmd.TryGetAsInt(1, framerate))
            {
                std::cout << "Usage: framerate must be an integer" << std::endl;

                continue;
            }

            int framesBufferSize;

            if (!cmd.TryGetAsInt(2, framesBufferSize))
            {
                std::cout << "Usage: framesBufferSize must be an integer" << std::endl;

                continue;
            }

            HANDLE_EXCEPTIONS(serverHandle->start(framerate, framesBufferSize));
        }
        else if (cmd.Get(0) == "stop")
        {
            if (cmd.Size() != 2)
            {
                std::cout << "Usage: stop <folder>" << std::endl;

                continue;
            }

            std::string folderPath = cmd.Get(1);

            HANDLE_EXCEPTIONS(serverHandle->stop(folderPath));
        }
        else if (cmd.Get(0) == "cancel")
        {
            HANDLE_EXCEPTIONS(serverHandle->cancel());
        }
        else if (cmd.Get(0) == "disconnect")
        {
            HANDLE_EXCEPTIONS(serverHandle->disconnect());

            break;
        }
        else if (cmd.Get(0) == "kill")
        {
            HANDLE_EXCEPTIONS(serverHandle->kill());

            break;
        }
    }
}

int main(int argc, char* argv[])
{
    CommandLine cmd(argc, argv);

    if (cmd.Size() < 2)
    {
        std::cout << helpMessage << std::endl;
    }
    else if (cmd.Get(1) == "-help") 
    {
        if (cmd.Size() < 3)
        {
            std::cout << helpMessage << std::endl;
        }
        else if (cmd.Get(2) == "start")
        {
            std::cout << startHelpMessage << std::endl;
        }
        else if (cmd.Get(2) == "stop")
        {
            std::cout << stopHelpMessage << std::endl;
        }
        else 
        {
            std::cout << helpMessage << std::endl;
        } 
    }
    else if (cmd.Get(1) == "-start")
    {
        int framerate = 1;
        int framebuffer = 10;

        for (int i = 2; i < cmd.Size(); i++) 
        {
            if (cmd.Get(i) == "-framerate" && cmd.TryGetAsInt(i, framerate))
            {
                i++;
            }
            else if (cmd.Get(i) == "-framebuffer" && cmd.TryGetAsInt(i, framebuffer))
            {
                i++;
            }
            else 
            {
                std::cout << invalidCommandSynatxMessage << std::endl;
                std::cout << startHelpMessage << std::endl;

                return 0;
            }
        }

        Start(framerate, framebuffer);
    }
    else if (cmd.Get(1) == "-stop")
    {
        if (cmd.Size() < 3) 
        {
            std::cout << invalidCommandSynatxMessage << std::endl;
            std::cout << stopHelpMessage << std::endl;

            return 0;
        }

        std::string folderPath = cmd.Get(2);

        try
        {
            StorageFolder::GetFolderFromPathAsync(winrt::to_hstring(folderPath)).get();
        }
        catch (const winrt::hresult_invalid_argument& e)
        {
            std::cout << invalidCommandSynatxMessage << std::endl;
            std::cout << stopHelpMessage << std::endl;

            return 0;
        }
        catch (const winrt::hresult_error& e)
        {
            std::cout << invalidCommandSynatxMessage << std::endl;
            std::cout << stopHelpMessage << std::endl;

            return 0;
        }

        Stop(folderPath);
    }
    else if (cmd.Get(1) == "-cancel")
    {
        Cancel();
    }
    else if (cmd.Get(1) == "-newserver")
    { 
        NewServer();
    }
    else if (cmd.Get(1) == "-newclient")
    {
        NewClient();
    }
    else 
    {
        std::cout << helpMessage << std::endl;
    }
}