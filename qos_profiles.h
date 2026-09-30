#pragma once
#include <dds/dds.hpp>

namespace QoSProfile {
inline dds::sub::qos::DataReaderQos VideoSub(const dds::sub::Subscriber& sub) {
    return sub.default_datareader_qos()
           << dds::core::policy::Reliability::BestEffort()
           << dds::core::policy::Durability::Volatile()
           << dds::core::policy::History::KeepLast(3);
}
}