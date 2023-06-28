#pragma once

#include "pch.h"

// The purpose of this class is to provide a utility for parsing tokens in a command line and converting them to integers.
class CommandLine {
public:
    CommandLine(const std::string& commandLine);
    CommandLine(int argc, char* argv[]);
    int Size() const;

    /**
     * @throws std::out_of_range exception if index is out of range of tokens
     */
    bool TryGetAsInt(int i , int& value) const;

    /**
     * @throws std::out_of_range exception if indices are out of range of tokens
     */
    std::string Get(int startIndex, int stopIndex) const;

    /**
     * @throws std::out_of_range exception if index is out of range of tokens
     */
    std::string Get(int index) const;

private:
    std::vector<std::string> tokens;
};
