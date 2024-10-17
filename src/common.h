#pragma once

#include <chrono>

inline void change_blink(bool *const pBlink,
                         std::chrono::milliseconds *const pTimer,
                         const bool blink_or_not,
                         std::chrono::milliseconds &timerDuration) {
  *pBlink = blink_or_not;
  *pTimer = timerDuration;
}
