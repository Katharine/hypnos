#ifndef coreback_coreback_h
#define coreback_coreback_h

#include <any>
#include <functional>
#include <Arduino.h>

namespace coreback {

namespace {
    struct fn_info {
        std::function<std::any()> fn_ret;
        std::function<void(std::any)> callback_ret;
        std::function<void()> fn_void;
        std::function<void()> callback_void;
    };
}

void run_elsewhere(std::function<std::any()> fn, std::function<void(std::any)> callback) {
    fn_info *info = new fn_info{
        .fn_ret = fn,
        .callback_ret = callback,
    };
    rp2040.fifo.push(reinterpret_cast<uint32_t>(info));
}

void run_elsewhere(std::function<void()> fn, std::function<void()> callback) {
    fn_info *info = new fn_info{
        .fn_void = fn,
        .callback_void = callback,
    };
    rp2040.fifo.push(reinterpret_cast<uint32_t>(info));
}

void run_elsewhere(std::function<void()> fn) {
    fn_info *info = new fn_info{
        .fn_void = fn,
    };
    rp2040.fifo.push(reinterpret_cast<uint32_t>(info));
}

void tick(bool blocking) {
    uint32_t popped;
    if (blocking) {
        popped = rp2040.fifo.pop();
    } else {
        if (!rp2040.fifo.pop_nb(&popped)) {
            return;
        }
    }
    fn_info *info = reinterpret_cast<fn_info*>(popped);
    if (info->fn_ret) {
        std::any result = info->fn_ret();
        run_elsewhere(std::bind(info->callback_ret, result));
    } else if (info->fn_void) {
        info->fn_void();
        run_elsewhere(info->callback_void);
    }
    delete info;
}

}

#endif
