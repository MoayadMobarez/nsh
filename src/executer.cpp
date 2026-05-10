#include "executer.hpp"

#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#include <iostream>

#include "builtins.hpp"

void Executer::execute(const std::vector<std::string>& tokens) {
    if (tokens.empty()) {
        return;
    }

    if (Builtins::handle(tokens)) {
        return;
    }

    std::vector<std::string> args;
    std::string outputFile;
    bool background = false;

    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == ">") {
            if (i + 1 < tokens.size()) {
                outputFile = tokens[i + 1];
            }
            break;
        }

        if (tokens[i] == "&") {
            background = true;
            continue;
        }

        args.push_back(tokens[i]);
    }

    if (args.empty()) {
        return;
    }

    std::vector<const char*> argv;

    for (const std::string& arg : args) {
        argv.push_back(arg.c_str());
    }

    argv.push_back(nullptr);

    pid_t pid = fork();

    if (pid < 0) {
        std::cerr << args[0] << ": failed to execute command" << std::endl;
    } else if (pid == 0) {
        if (!outputFile.empty()) {
            int fd = open(outputFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);

            if (fd < 0) {
                std::cerr << outputFile << ": failed to open file" << std::endl;
                _exit(1);
            }

            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        int status = execvp(argv[0], const_cast<char* const*>(argv.data()));

        if (status != 0) {
            std::string msg = "failed to execute command";

            if (errno == ENOENT) {
                msg = "command not found";
            }

            std::cerr << args[0] << ": " << msg << std::endl;
        }

        _exit(1);
    } else {
        if (background) {
            std::cout << "[" << pid << "]" << std::endl;
        } else {
            waitpid(pid, nullptr, 0);
        }
    }
}
