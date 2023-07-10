#include "pch.h"
#include "CommandLine.h"


CommandLine::CommandLine(const std::string& commandLine) 
{
    std::istringstream iss(commandLine);
    std::string token;

    while (iss >> token) 
    {
        tokens.push_back(token);
    }
}

CommandLine::CommandLine(int argc, char* argv[]) 
{
    for (int i = 0; i < argc; ++i) 
    {
        tokens.emplace_back(argv[i]);
    }
}

int CommandLine::Size() const
{
    return tokens.size();
}

bool CommandLine::TryGetAsInt(int i, int& value) const 
{
    if (i < 0 || i >= tokens.size()) 
    {
        throw std::out_of_range("Token index out of range");
    }

    try 
    {
        value = std::stoi(tokens[i]);

        return true;
    }
    catch (...) 
    {
        return false;
    }
}

bool CommandLine::TryGetAsBool(int i, bool& value) const
{
    if (i < 0 || i >= tokens.size())
    {
        throw std::out_of_range("Token index out of range");
    }

    try
    {
        value = std::stoi(tokens[i]) != 0;

        return true;
    }
    catch (...)
    {
        return false;
    }
}


std::string CommandLine::Get(int startIndex, int stopIndex) const 
{
    if (startIndex > stopIndex || startIndex > tokens.size() || stopIndex < 0)
    {
        throw std::out_of_range("Invalid start or stop index");
    }

    std::ostringstream oss;

    for (int i = startIndex; i < stopIndex; ++i) 
    {
        if (i > startIndex) 
        {
            oss << ' ';
        }
        oss << tokens[i];
    }
    return oss.str();
}

std::string CommandLine::Get(int index) const 
{
    if (index < 0 || index >= tokens.size()) 
    {
        throw std::out_of_range("Token index out of range");
    }

    return tokens[index];
}