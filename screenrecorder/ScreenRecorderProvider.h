#pragma once

#include "pch.h"

// Forward-declare the g_hMyComponentProvider variable that you will use for tracing in this component
TRACELOGGING_DECLARE_PROVIDER(g_hMyComponentProvider);

#define ReceivedFrameEvent() \
    TraceLoggingWrite(g_hMyComponentProvider, \
        "ReceivedFrame", \
        TraceLoggingDescription("Received frame in frame pool."))

#define StoredFrameEvent() \
    TraceLoggingWrite(g_hMyComponentProvider, \
        "StoredFrame", \
        TraceLoggingDescription("Stored frame to save to disk later."))