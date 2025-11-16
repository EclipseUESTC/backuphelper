#pragma once
#include <string>

enum class TaskStatus {
    PENDING,
    RUNNING,
    COMPLETED,
    FAILED
};

enum class ScheduleType {
    MANUAL,
    SCHEDULED,
    REALTIME
};

inline std::string toString(TaskStatus status) {
    switch (status) {
        case TaskStatus::PENDING: return "PENDING";
        case TaskStatus::RUNNING: return "RUNNING";
        case TaskStatus::COMPLETED: return "COMPLETED";
        case TaskStatus::FAILED: return "FAILED";
        default: return "UNKNOWN";
    }
}

inline std::string toString(ScheduleType type) {
    switch (type) {
        case ScheduleType::MANUAL: return "MANUAL";
        case ScheduleType::SCHEDULED: return "SCHEDULED";
        case ScheduleType::REALTIME: return "REALTIME";
        default: return "UNKNOWN";
    }
}