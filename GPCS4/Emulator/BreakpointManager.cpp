#include "GPCS4Common.h"
#include "BreakpointManager.h"
#include "SceModuleSystem.h"

#include <unordered_map>
#ifdef GPCS4_WINDOWS
#include <Windows.h>

struct BuiltInDebuggerImpl
{
	static long __stdcall exceptionHanlder(void* p)
	{
		auto pExp = reinterpret_cast<PEXCEPTION_POINTERS>(p);
		auto& instMap = BreakpointManager::get()->m_impl->instMap;

		auto code = pExp->ExceptionRecord->ExceptionCode;
		if (code != 0xc0000096)
		{
			return EXCEPTION_CONTINUE_SEARCH;
		}

		auto expAddr = pExp->ExceptionRecord->ExceptionAddress;
		auto it      = instMap.find(expAddr);
		if (it == instMap.end())
		{
			return EXCEPTION_CONTINUE_SEARCH;
		}

		DWORD op = 0;
		VirtualProtect(expAddr, 1, PAGE_EXECUTE_READWRITE, &op);
		*reinterpret_cast<uint8_t*>(expAddr) = it->second;
		VirtualProtect(expAddr, 1, op, &op);

		return EXCEPTION_CONTINUE_EXECUTION;
	}

	bool addBreakpoint(void *address)
	{
		DWORD op = 0;

		auto code = reinterpret_cast<uint8_t*>(address);

		// backup the original byte.
		instMap.insert(std::make_pair(address, *code));

		VirtualProtect(address, 1, PAGE_EXECUTE_READWRITE, &op);

		// modify the first by of instruction to hlt, which can trigger an exception
		*code = 0xf4;
		VirtualProtect(address, 1, op, &op);

		return true;
	}

	BuiltInDebuggerImpl():
		modSystem{*CSceModuleSystem::GetInstance()}
	{
		auto p = AddVectoredContinueHandler(TRUE, reinterpret_cast<PVECTORED_EXCEPTION_HANDLER>(exceptionHanlder));
		if (p != nullptr)
		{
			initialized = true;
		}
	}

	std::unordered_map<void*, uint8_t> instMap;
	CSceModuleSystem& modSystem;
	bool initialized;
};
#else
struct BuiltInDebuggerImpl
{
	bool addBreakPoint(void *address)
	{
		return false;
	}

	std::unordered_map<void*, uint8_t> instMap;
	bool initialized = false;
};

#endif

BreakpointManager::~BreakpointManager() = default;
BreakpointManager::BreakpointManager()
{
	m_impl = std::make_unique<BuiltInDebuggerImpl>();
}

BreakpointManager* BreakpointManager::get()
{
	static BreakpointManager instance;
	return &instance;
}

bool BreakpointManager::addBreakpoint(void* p)
{
	bool ret = false;
	do
	{
		if (m_impl->initialized == false)
		{
			ret = false;
			break;
		}

		ret = m_impl->addBreakpoint(p);

	} while (false);

	return ret;
}

bool BreakpointManager::addBreakpoint(std::string const& nativeModName, size_t offset)
{
	bool ret = false;
	auto &modSys = m_impl->modSystem;
	do 
	{
		NativeModule* mod = nullptr;
		if (!modSys.getNativeModule(nativeModName, &mod))
		{
			ret = false;
			break;
		}

		uint8_t *address = mod->getMappedMemory().get();
		auto breakAddr = reinterpret_cast<void*>(address + offset); 

		ret = addBreakpoint(breakAddr);
	} while (false);

	return ret;
}

bool BreakpointManager::addBreakPoint(std::string const& modName,
									std::string const& libName,
									uint64_t nid)
{
	bool ret = false;
	auto &modSys = m_impl->modSystem;
	do
	{
		void* address = modSys.getSymbolAddress(modName, libName, nid);
		if (address == nullptr)
		{
			ret = false;
			break;
		}

		ret = addBreakpoint(address);

	} while (false);

	return ret;
}
