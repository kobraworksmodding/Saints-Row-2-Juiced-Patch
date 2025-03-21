#pragma once

namespace ErrorManager
{
    enum ExceptionHandlerResult
    {
        EHR_IGNORE = 0,
        EHR_CONTINUE = 1,
        EHR_TERMINATE = 2
    };

    struct ErrorEntry
    {
        uint32_t code;
        const char* codeName;
        ExceptionHandlerResult codeResult;
    };

    extern bool b_HandlerAssigned;
    extern bool b_Enabled;
    void AssignHandler();
    void UnassignHandler();
    void Initialize();

    void IgnoreVectoredExceptions(bool ignored);
}
