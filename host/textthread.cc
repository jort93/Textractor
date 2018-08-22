// textthread.cc
// 8/24/2013 jichi
// Branch IHF/TextThread.cpp, rev 133
#ifdef _MSC_VER
# pragma warning (disable:4100)   // C4100: unreference formal parameter
#endif // _MSC_VER

#include "textthread.h"
#include <mutex>
#include "../vnrhook/include/const.h"
#include "winmutex.h"

#define TT_LOCK std::lock_guard<std::recursive_mutex> ttLocker(ttMutex) // Synchronized scope for accessing private data

TextThread::TextThread(ThreadParameter tp, DWORD status) :
	deletionEvent(CreateEventW(nullptr, FALSE, FALSE, NULL)),
	flushThread([&]() { while (WaitForSingleObject(deletionEvent, 100), Flush()); }),
	timestamp(GetTickCount()),
	Output(nullptr),
	tp(tp),
	status(status)
{}

TextThread::~TextThread()
{
	status = -1UL;
	SetEvent(deletionEvent);
	flushThread.join();
	CloseHandle(deletionEvent);
}

std::wstring TextThread::GetStore()
{
	TT_LOCK;
	return storage;
}

bool TextThread::Flush()
{
	TT_LOCK;
	if (status == -1UL) return false;
	if (timestamp - GetTickCount() < 250 || buffer.size() == 0) return true; // TODO: let user change delay before sentence is flushed
	std::wstring sentence;
	if (status & USING_UNICODE)
	{
		sentence = std::wstring((wchar_t*)buffer.data(), buffer.size() / 2);
	}
	else
	{
		wchar_t* converted = new wchar_t[buffer.size()];
		sentence = std::wstring(converted, MultiByteToWideChar(status & USING_UTF8 ? CP_UTF8 : 932, 0, buffer.data(), buffer.size(), converted, buffer.size()));
		delete[] converted;
	}
	AddSentence(sentence);
	buffer.clear();
	return true;
}

void TextThread::AddSentence(std::wstring sentence)
{
	TT_LOCK;
	if (Output) sentence = Output(this, sentence);
	storage.append(sentence);
}

void TextThread::AddText(const BYTE *con, int len)
{
	TT_LOCK;
	buffer.insert(buffer.end(), con, con + len);
	timestamp = GetTickCount();
}

// EOF
