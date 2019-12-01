#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <exception>

FILE * tmpmfile(const char * prefix) {
    char path[MAX_PATH], filename[MAX_PATH];
    if (!::GetTempPathA(MAX_PATH, path)) {
        throw std::exception();
    }
    if (!::GetTempFileNameA(path, prefix, 0, filename)) {
        throw std::exception();
    }
    auto handle = ::CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        throw std::exception();
    }
    auto fd = _open_osfhandle((intptr_t)handle, _O_RDWR | _O_BINARY);
    if (fd == -1) {
        throw std::exception();
    }
    auto fs = _fdopen(fd, "r+b");
    if (fs == NULL) {
        throw std::exception();
    }
    return fs;
}
