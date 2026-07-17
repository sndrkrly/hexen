#pragma once

#include <string>
#include <string_view>

// Optimized string replacement - modifies in place
inline void remove_substring(std::string& str, std::string_view toRemove) {
    if (toRemove.empty()) return;
    
    size_t pos = 0;
    while ((pos = str.find(toRemove, pos)) != std::string::npos) {
        str.erase(pos, toRemove.length());
    }
}

// Cache-friendly time formatting
inline std::string format_time(float seconds) {
    if (seconds < 0) seconds = 0;
    
    int minutes = static_cast<int>(seconds) / 60;
    int secs = static_cast<int>(seconds) % 60;
    
    // Pre-allocate to avoid reallocations
    std::string buffer;
    buffer.reserve(16);
    
    if (minutes < 10) buffer += '0';
    buffer += std::to_string(minutes);
    buffer += ':';
    if (secs < 10) buffer += '0';
    buffer += std::to_string(secs);
    
    return buffer;
}
