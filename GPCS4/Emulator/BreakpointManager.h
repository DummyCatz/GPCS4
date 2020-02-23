#pragma once
#include <memory>

struct BuiltInDebuggerImpl;

class BreakpointManager
{
public:
	friend BuiltInDebuggerImpl;
	static BreakpointManager* get();

	bool addBreakpoint(void* p);
	bool addBreakpoint(std::string const& nativeModName, size_t offset);
	bool addBreakPoint(std::string const& modName,
					   std::string const& libName,
					   uint64_t nid);

	BreakpointManager(BreakpointManager const &) = delete;
	~BreakpointManager();

private:
	BreakpointManager();
	std::unique_ptr<BuiltInDebuggerImpl> m_impl;
};
