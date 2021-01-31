#pragma once

class Multicont {

public:

	Multicont() : pid_(0) {};

	bool RunMulticont();
	DWORD GetPid() { return pid_; }

private:

	bool RemoveDebugPrivileges(HANDLE token);
	HANDLE GetProcessHandle();

	DWORD pid_;

};

