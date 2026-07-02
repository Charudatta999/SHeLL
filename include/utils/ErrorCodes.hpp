#ifndef UTIL_ERR_CODE_HPP
#define UTIL_ERR_CODE_HPP

const int FAILED_TO_CREATE = 1001;
const int INVALID_STATUS = 1002;
const int VALUE_NOT_FOUND = 1003;
const int PROCESS_RUNNING = 1004;
const int EXIT_COMMAND_NOT_FOUND = 127;
const int EXIT_PERMISSION_DENIED = 126;
// POSIX shell convention: a job ended by signal N reports $? =
// 128 + N.
const int SIGNAL_EXIT_BASE = 128;

#endif // UTIL_ERR_CODE_HPP