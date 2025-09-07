#include "ControlMessage.h"
#include <sstream>

std::string ControlMessage::Message::toString() const {
    std::ostringstream oss;
    
    switch (type) {
        case Type::KEY_PRESS:
            oss << "KEY_PRESS(button=" << param1 << ", time=" << timestamp << ")";
            break;
        case Type::KEY_RELEASE:
            oss << "KEY_RELEASE(button=" << param1 << ", time=" << timestamp << ")";
            break;
        case Type::CLOCK_TICK:
            oss << "CLOCK_TICK(ticks=" << param1 << ", time=" << timestamp << ")";
            break;
        case Type::TIME_ADVANCE:
            oss << "TIME_ADVANCE(ms=" << param1 << ", time=" << timestamp << ")";
            break;
        case Type::START:
            oss << "START(time=" << timestamp << ")";
            break;
        case Type::STOP:
            oss << "STOP(time=" << timestamp << ")";
            break;
        case Type::RESET:
            oss << "RESET(time=" << timestamp << ")";
            break;
        case Type::SAVE_STATE:
            oss << "SAVE_STATE(file=" << stringParam << ", time=" << timestamp << ")";
            break;
        case Type::LOAD_STATE:
            oss << "LOAD_STATE(file=" << stringParam << ", time=" << timestamp << ")";
            break;
        case Type::VERIFY_STATE:
            oss << "VERIFY_STATE(expected=" << stringParam.substr(0, 50) << "..., time=" << timestamp << ")";
            break;
        case Type::SET_TEMPO:
            oss << "SET_TEMPO(bpm=" << param1 << ", time=" << timestamp << ")";
            break;
        case Type::QUERY_STATE:
            oss << "QUERY_STATE(time=" << timestamp << ")";
            break;
        default:
            oss << "UNKNOWN(" << static_cast<int>(type) << ")";
            break;
    }
    
    return oss.str();
}