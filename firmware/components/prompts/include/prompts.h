#ifndef PROMPTS_PROMPTS_H
#define PROMPTS_PROMPTS_H

#include <functional>
#include <string>

namespace prompts {

typedef std::function<void()> ConfirmCallback;

class Confirm {
    const std::string message;
    const std::string positiveText;
    const std::string negativeText;
    const ConfirmCallback positiveCallback;
    const ConfirmCallback negativeCallback;

public:
    Confirm(std::string  message, std::string  positiveText, std::string  negativeText, ConfirmCallback positive, ConfirmCallback negative);
    void display();

    static void Display(std::string  message, std::string  positiveText, std::string  negativeText, ConfirmCallback positive, ConfirmCallback negative);
};

}

#endif