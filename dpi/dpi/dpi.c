#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LEN 255
#define MAX_COUNT 10

#define PER_MONITOR_SETTINGS_REGISTRY_PATH "Control Panel\\Desktop\\PerMonitorSettings\\"
#define CFG_FILENAME "dpi.cfg"

typedef struct
{
	CHAR monitorName[MAX_LEN];
	DISPLAY_DEVICE device;
	DISPLAY_DEVICE monitorDevice;
	CHAR registryName[MAX_LEN];
} monitorInfo_t;

LONG ChangeResolution(const char* deviceName, int width, int height);
BOOL GetResolution(const char* deviceName, int* width, int* height);
void SetDpi(const char* registryKeyName, DWORD level); // returns TRUE if changed

BOOL GetDisplayDevice(int index, DISPLAY_DEVICE* device, DISPLAY_DEVICE* monitorDevice);
int ListMonitors(monitorInfo_t* result, int maxCount);
BOOL GetMonitor(const char* monitorName, monitorInfo_t* result);

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("Usage: dpi [level] [monitorName]\n");
		printf("level (default 0) - dpi level:\n");
		printf("\t     0 = recommended level\n");
		printf("\t-1..-x = lower dpi\n");
		printf("\t  1..y = higher dpi \n");
		printf("\t  list = list all display devices\n");
		printf("monitorName - name of monitor, one of listed values. If not specified then program reads it from the dpi.cfg file\n");
		printf("Example usage:\n");
		printf("\tdpi -1\n");
		printf("\tdpi -1 LGD047A\n");
		return 0;
	}

	if (strcmp(argv[1], "list") == 0)
	{
		monitorInfo_t monitors[MAX_COUNT];
		int count = ListMonitors(monitors, MAX_COUNT);
		for (int i = 0; i < count; i++)
		{
			printf("%s\n", monitors[i].monitorName);
		}
		return 0;
	}

	DWORD targetValue = (DWORD)atoi(argv[1]);
	char* monitorName = NULL;
	char buffer[MAX_LEN];
	if (argc >= 3)
	{
		monitorName = argv[2];
	}
	else
	{
		printf("No monitor name specified, looking for "CFG_FILENAME"...\n");
		FILE* file;
		fopen_s(&file, CFG_FILENAME, "r"); // check working directory
		if (file == NULL)
		{
			// check directory where executable is located
			HMODULE hModule = GetModuleHandle(NULL);
			CHAR path[MAX_PATH];
			GetModuleFileName(hModule, path, MAX_PATH);
			char* last = strrchr(path, '\\');
			*(last + 1) = 0;
			
			strcat_s(path, MAX_PATH, CFG_FILENAME);
			printf(CFG_FILENAME" in working directory not found. Checking %s\n", path);

			fopen_s(&file, path, "r");
			if (file == NULL) printf("%s not found\n", path);
		}

		if (file != NULL)
		{
			fscanf_s(file, "%254s", buffer, MAX_LEN);
			fclose(file);
			monitorName = (char*)&buffer;
		}
	}

	if (monitorName == NULL)
	{
		printf("Monitor name not specified. Use [monitorName] argument or dpi.cfg file\n");
		return -1;
	}

	printf("Looking for monitor %s...\n", monitorName);
	monitorInfo_t monitor;
	if (!GetMonitor(monitorName, &monitor))
	{
		printf("Monitor %s not found!\n", monitorName);
		return -1;
	}
	printf("Found monitor %s with following identifiers:\n", monitor.monitorName);
	printf("DeviceID: %s\n"
		"DeviceName: %s\n"
		"Monitor DeviceID: %s\n"
		"Monitor DeviceName: %s\n"
		"Registry key name: %s\n",
		monitor.device.DeviceID, monitor.device.DeviceName, monitor.monitorDevice.DeviceID, monitor.monitorDevice.DeviceName, monitor.registryName);

	printf("Changing DPI value in registry to %d\n", targetValue);
	SetDpi(monitor.registryName, targetValue);

	int width, height;
	if (!GetResolution(monitor.device.DeviceName, &width, &height))
	{
		printf("Error while obtaining current resolution %d %d\n", width, height);
		return -1;
	}

	printf("Changing resolution temporarily to %d x %d...\n", width - 1, height - 1);
	ChangeResolution(monitor.device.DeviceName, width - 1, height - 1);

	printf("Restoring resolution to %d x %d...\n", width, height);
	ChangeResolution(monitor.device.DeviceName, width, height);

	return 0;
}

void SetDpi(const char* registryKeyName, DWORD level)
{
	char keyName[MAX_LEN] = PER_MONITOR_SETTINGS_REGISTRY_PATH;
	strcat_s(keyName, MAX_LEN, registryKeyName);

	HKEY hkey;

	RegOpenKeyEx(HKEY_CURRENT_USER, keyName, 0, KEY_SET_VALUE | KEY_QUERY_VALUE, &hkey);
	RegSetValueEx(hkey, TEXT("DpiValue"), 0, REG_DWORD, (const BYTE*)&level, sizeof(DWORD));

	RegCloseKey(hkey);
}

LONG ChangeResolution(const char* deviceName, int width, int height)
{
	DEVMODE devMode;
	devMode.dmSize = sizeof(DEVMODE);
	devMode.dmDriverExtra = 0;

	devMode.dmPelsWidth = width;
	devMode.dmPelsHeight = height;
	devMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

	return ChangeDisplaySettingsEx(deviceName, &devMode, NULL, 0, NULL);
}

BOOL GetResolution(const char* deviceName, int* width, int* height)
{
	DEVMODE devMode;
	devMode.dmSize = sizeof(DEVMODE);
	devMode.dmDriverExtra = 0;

	if (EnumDisplaySettings(deviceName, ENUM_CURRENT_SETTINGS, &devMode))
	{
		*width = devMode.dmPelsWidth;
		*height = devMode.dmPelsHeight;
		return TRUE;
	}

	return FALSE;
}

BOOL GetDisplayDevice(int index, DISPLAY_DEVICE* device, DISPLAY_DEVICE* monitorDevice)
{
	device->cb = sizeof(DISPLAY_DEVICE);
	if (!EnumDisplayDevices(NULL, index, device, 0)) return FALSE;

	monitorDevice->cb = sizeof(DISPLAY_DEVICE);
	if (!EnumDisplayDevices(device->DeviceName, 0, monitorDevice, 0)) return FALSE;

	return TRUE;
}

int ListMonitors(monitorInfo_t* result, int maxCount)
{
	DISPLAY_DEVICE devices[MAX_COUNT];
	DISPLAY_DEVICE monitorDevices[MAX_COUNT];
	char regNames[MAX_COUNT * MAX_LEN];

	// Enumerate all monitors
	int devicesCount = 0;
	while (GetDisplayDevice(devicesCount, &devices[devicesCount], &monitorDevices[devicesCount])) devicesCount++;

	// Enumerate all registry keys
	HKEY hkey;
	RegOpenKeyEx(HKEY_CURRENT_USER, TEXT(PER_MONITOR_SETTINGS_REGISTRY_PATH), 0, KEY_READ, &hkey);
	DWORD keyNameLen = MAX_LEN;
	int regKeysCount = 0;
	while (RegEnumKeyEx(hkey, regKeysCount, regNames + regKeysCount * MAX_LEN, &keyNameLen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
	{
		regKeysCount++;
	}

	RegCloseKey(hkey);

	// Match devices and registry keys
	int currentCount = 0;
	int currentDevId = 0;
	while (maxCount && currentDevId < devicesCount)
	{
		DISPLAY_DEVICE* device = &devices[currentDevId];
		DISPLAY_DEVICE* monitorDevice = &monitorDevices[currentDevId];
		currentDevId++;

		// Extract monitor name
		char* firstBSlash = strstr(monitorDevice->DeviceID, "\\");
		if (firstBSlash == NULL) continue;
		char* begin = firstBSlash + 1;
		char* secondBSlash = strstr(begin, "\\");
		if (secondBSlash == NULL) continue;

		size_t size = secondBSlash - begin;

		char monitorName[MAX_LEN];
		memcpy(monitorName, begin, size);
		monitorName[size] = 0;

		// Match registry key
		char* regKey = NULL;
		for (int i = 0; i < regKeysCount; i++)
		{
			char* iRegKey = regNames + i * MAX_LEN;
			if (strstr(iRegKey, monitorName) == iRegKey)
			{
				regKey = iRegKey;
				break;
			}
		}
		if (regKey == NULL) continue;

		// Copy results
		strcpy_s(result->monitorName, MAX_LEN, monitorName);
		memcpy(&result->device, device, sizeof(DISPLAY_DEVICE));
		memcpy(&result->monitorDevice, monitorDevice, sizeof(DISPLAY_DEVICE));
		strcpy_s(result->registryName, MAX_LEN, regKey);
		result++;
		currentCount++;
	}

	return currentCount;
}

BOOL GetMonitor(const char* monitorName, monitorInfo_t* result)
{
	monitorInfo_t allMonitors[MAX_COUNT];
	int count = ListMonitors(allMonitors, MAX_COUNT);
	for (int i = 0; i < count; i++)
	{
		monitorInfo_t* mon = &allMonitors[i];
		if (strcmp(monitorName, mon->monitorName) == 0)
		{
			memcpy(result, mon, sizeof(monitorInfo_t));
			return TRUE;
		}
	}
	return FALSE;
}