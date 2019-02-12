//--------------------------------------------------------------------------------------
// File: Keyboard.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#include <cassert>
#include <exception>
#include <wrl/client.h>
#include "Keyboard.h"



using namespace DirectX;
using Microsoft::WRL::ComPtr;

struct handle_closer { void operator()(HANDLE h) { if (h) CloseHandle(h); } };

typedef std::unique_ptr<void, handle_closer> ScopedHandle;

static_assert(sizeof(Keyboard::State) == (256 / 8), "Size mismatch for State");

namespace
{
	void KeyDown(int key, Keyboard::State& state)
	{
		if (key < 0 || key > 0xfe)
			return;

		auto ptr = reinterpret_cast<uint32_t*>(&state);

		unsigned int bf = 1u << (key & 0x1f);
		ptr[(key >> 5)] |= bf;
	}

	void KeyUp(int key, Keyboard::State& state)
	{
		if (key < 0 || key > 0xfe)
			return;

		auto ptr = reinterpret_cast<uint32_t*>(&state);

		unsigned int bf = 1u << (key & 0x1f);
		ptr[(key >> 5)] &= ~bf;
	}
}


#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)

//======================================================================================
// Win32 desktop implementation
//======================================================================================

//
// For a Win32 desktop application, call this function from your Window Message Procedure
//
// LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
// {
//     switch (message)
//     {
//
//     case WM_ACTIVATEAPP:
//         Keyboard::ProcessMessage(message, wParam, lParam);
//         break;
//
//     case WM_KEYDOWN:
//     case WM_SYSKEYDOWN:
//     case WM_KEYUP:
//     case WM_SYSKEYUP:
//         Keyboard::ProcessMessage(message, wParam, lParam);
//         break;
//
//     }
// }
//

class Keyboard::Impl
{
public:
	Impl(Keyboard* owner) :
		mState{},
		mOwner(owner)
	{
		if (s_keyboard)
		{
			throw std::exception("Keyboard is a singleton");
		}

		s_keyboard = this;
	}

	~Impl()
	{
		s_keyboard = nullptr;
	}

	void GetState(State& state) const
	{
		memcpy(&state, &mState, sizeof(State));
	}

	void Reset()
	{
		memset(&mState, 0, sizeof(State));
	}

	bool IsConnected() const
	{
		return true;
	}

	State           mState;
	Keyboard*       mOwner;

	static Keyboard::Impl* s_keyboard;
};


Keyboard::Impl* Keyboard::Impl::s_keyboard = nullptr;


void Keyboard::ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	auto pImpl = Impl::s_keyboard;

	if (!pImpl)
		return;

	bool down = false;

	switch (message)
	{
	case WM_ACTIVATEAPP:
		pImpl->Reset();
		return;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		down = true;
		break;

	case WM_KEYUP:
	case WM_SYSKEYUP:
		break;

	default:
		return;
	}

	int vk = static_cast<int>(wParam);
	switch (vk)
	{
	case VK_SHIFT:
		vk = MapVirtualKey((lParam & 0x00ff0000) >> 16, MAPVK_VSC_TO_VK_EX);
		if (!down)
		{
			// Workaround to ensure left vs. right shift get cleared when both were pressed at same time
			KeyUp(VK_LSHIFT, pImpl->mState);
			KeyUp(VK_RSHIFT, pImpl->mState);
		}
		break;

	case VK_CONTROL:
		vk = (lParam & 0x01000000) ? VK_RCONTROL : VK_LCONTROL;
		break;

	case VK_MENU:
		vk = (lParam & 0x01000000) ? VK_RMENU : VK_LMENU;
		break;
	}

	if (down)
	{
		KeyDown(vk, pImpl->mState);
	}
	else
	{
		KeyUp(vk, pImpl->mState);
	}
}

#endif



#pragma warning( disable : 4355 )

// Public constructor.
Keyboard::Keyboard() noexcept(false)
	: pImpl(std::make_unique<Impl>(this))
{
}


// Move constructor.
Keyboard::Keyboard(Keyboard&& moveFrom) noexcept
	: pImpl(std::move(moveFrom.pImpl))
{
	pImpl->mOwner = this;
}


// Move assignment.
Keyboard& Keyboard::operator= (Keyboard&& moveFrom) noexcept
{
	pImpl = std::move(moveFrom.pImpl);
	pImpl->mOwner = this;
	return *this;
}


// Public destructor.
Keyboard::~Keyboard()
{
}


Keyboard::State Keyboard::GetState() const
{
	State state;
	pImpl->GetState(state);
	return state;
}


void Keyboard::Reset()
{
	pImpl->Reset();
}


bool Keyboard::IsConnected() const
{
	return pImpl->IsConnected();
}

Keyboard& Keyboard::Get()
{
	if (!Impl::s_keyboard || !Impl::s_keyboard->mOwner)
		throw std::exception("Keyboard is a singleton");

	return *Impl::s_keyboard->mOwner;
}



//======================================================================================
// KeyboardStateTracker
//======================================================================================

void Keyboard::KeyboardStateTracker::Update(const State& state)
{
	auto currPtr = reinterpret_cast<const uint32_t*>(&state);
	auto prevPtr = reinterpret_cast<const uint32_t*>(&lastState);
	auto releasedPtr = reinterpret_cast<uint32_t*>(&released);
	auto pressedPtr = reinterpret_cast<uint32_t*>(&pressed);
	for (size_t j = 0; j < (256 / 32); ++j)
	{
		*pressedPtr = *currPtr & ~(*prevPtr);
		*releasedPtr = ~(*currPtr) & *prevPtr;

		++currPtr;
		++prevPtr;
		++releasedPtr;
		++pressedPtr;
	}

	lastState = state;
}


void Keyboard::KeyboardStateTracker::Reset() noexcept
{
	memset(this, 0, sizeof(KeyboardStateTracker));
}
