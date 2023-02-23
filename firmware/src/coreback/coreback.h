#ifndef coreback_coreback_h
#define coreback_coreback_h

#include <any>
#include <functional>

/**
 * Coreback provides a mechanism to run a function on the other core, and optionally make a callback
 * on the original core when that code completes.
 * 
 * Since the RP2040 has two cores, we'll just run the code on whichever core we aren't already on.
 */
namespace coreback {

/**
 * Run fn() on the other core. When it's done call callback() on the original core, passing it fn's return value.
 */
void run_elsewhere(std::function<std::any()> fn, std::function<void(std::any)> callback);
/**
 * Run fn() on the other core. When it's done, call callback() on the original core.
 */
void run_elsewhere(std::function<void()> fn, std::function<void()> callback);
/**
 * Run fn() on the other core.
 */
void run_elsewhere(std::function<void()> fn);
/**
 * Call regularly to check the queue and execute one function from it. If blocking is true, blocks
 * until something is on the queue; otherwise returns immediately.
 */
void tick(bool blocking);

}

#endif
