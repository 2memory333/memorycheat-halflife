#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>

int findMyProc(const char* procname) {

	HANDLE hSnapshot;
	PROCESSENTRY32 pe;
	int pid = 0;
	BOOL hResult;

	// snapshot of all processes in the system
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hSnapshot) return 0;

	// initializing size: needed for using Process32First
	pe.dwSize = sizeof(PROCESSENTRY32);

	// info about first process encountered in a system snapshot
	hResult = Process32First(hSnapshot, &pe);

	// retrieve information about the processes
	// and exit if unsuccessful
	while (hResult) {
		// if we find the process: return process ID
		if (strcmp(procname, pe.szExeFile) == 0) {
			pid = pe.th32ProcessID;
			break;
		}
		hResult = Process32Next(hSnapshot, &pe);
	}

	// closes an open handle (CreateToolhelp32Snapshot)
	CloseHandle(hSnapshot);
	return pid;
}

uintptr_t GetModuleBaseAddress(DWORD processId, const wchar_t* ModuleTarget) {
	HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
	if (snapshotHandle == INVALID_HANDLE_VALUE) {
		return NULL;
	}
	MODULEENTRY32W moduleEntry = { };
	moduleEntry.dwSize = sizeof(MODULEENTRY32W);
	if (Module32FirstW(snapshotHandle, &moduleEntry)) {
		do {
			if (_wcsicmp(moduleEntry.szModule, ModuleTarget) == 0) {
				CloseHandle(snapshotHandle);
				return reinterpret_cast<uintptr_t>(moduleEntry.modBaseAddr);
			}
		} while (Module32NextW(snapshotHandle, &moduleEntry));
	}
	CloseHandle(snapshotHandle);
	return NULL;
}

int width = 800;
int height = 600;
float save; //view matrix icin save
int health,enemybool;
int value;
float x, y, z;
DWORD dinolocal, xclass;
DWORD procID = findMyProc("bms.exe");
DWORD BaseModule = GetModuleBaseAddress(procID, L"server.dll"); //ikinci degiskene, cheatenginedeki yesil adi yaz.
HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, false, procID);
DWORD matrixviewadres = 0x10879914;

struct view_matrix_t {
	float* operator[ ](int index) {
		return matrix[index];
	}
	float matrix[4][4];
};

struct Vector3 {
	float x, y, z;
};

view_matrix_t matrixview;

void getmatrixview(view_matrix_t* matrix)
{
	for (int i = 0; i < 4; i++) {
		ReadProcessMemory(handle, LPCVOID(matrixviewadres + i * 4), &save, sizeof(save), nullptr);
		(*matrix)[0][i] = save;
	}

	for (int i = 0; i < 4; i++) {
		ReadProcessMemory(handle, LPCVOID(matrixviewadres + 16 + i * 4), &save, sizeof(save), nullptr);
		(*matrix)[1][i] = save;
	}

	for (int i = 0; i < 4; i++) {
		ReadProcessMemory(handle, LPCVOID(matrixviewadres + 32 + i * 4), &save, sizeof(save), nullptr);
		(*matrix)[3][i] = save;
	}
}

//çizdigimiz 2d sekillerin 3 boyuta uygulayan fonksiyon
Vector3 WorldToScreen(const Vector3 pos, view_matrix_t matrix) {
	float _x = matrix[0][0] * pos.x + matrix[0][1] * pos.y + matrix[0][2] * pos.z + matrix[0][3];
	float _y = matrix[1][0] * pos.x + matrix[1][1] * pos.y + matrix[1][2] * pos.z + matrix[1][3];

	float w = matrix[3][0] * pos.x + matrix[3][1] * pos.y + matrix[3][2] * pos.z + matrix[3][3];

	float inv_w = 1.f / w;
	_x *= inv_w;
	_y *= inv_w;

	float x = width * .5f;
	float y = height * .5f;

	x += 0.5f * _x * width + 0.5f;
	y -= 0.5f * _y * height + 0.5f;

	return{ x,y,w };
}

void DrawGoofyBox(HDC hdc, float sX, float sY, float eX, float eY)
{
	Rectangle(hdc, sX, sY, eX, eY);
	
}

void DrawLine(HDC hdc, float sX, float sY, float eX, float eY)
{
	int a, b = 0;
	HPEN hOPen;
	HPEN hNPen = CreatePen(PS_SOLID, 3, 0x000000EF);
	hOPen = (HPEN)SelectObject(hdc, hNPen);
	MoveToEx(hdc, sX, sY, NULL);
	a = LineTo(hdc, eX, eY);
	DeleteObject(SelectObject(hdc, hOPen));
}

void draw(HDC hdc) {
	ReadProcessMemory(handle, LPCVOID(BaseModule + 0x83DD2C), &dinolocal, sizeof(dinolocal), nullptr);
	Sleep(10); 
	ReadProcessMemory(handle, LPCVOID(dinolocal + 0x14), &xclass, sizeof(xclass), nullptr);
	ReadProcessMemory(handle, LPCVOID(xclass + 0x2c), &y, sizeof(y), nullptr);
	ReadProcessMemory(handle, LPCVOID(dinolocal + 0x280), &x, sizeof(x), nullptr);
	ReadProcessMemory(handle, LPCVOID(dinolocal + 0x288), &z, sizeof(z), nullptr);

	Vector3 pos = Vector3{ x,y,z };
	getmatrixview(&matrixview);
	Vector3 screenPos = WorldToScreen(pos, matrixview);
	if (screenPos.z >= 0.01f) {
		DrawLine(hdc, width / 2.f, height, screenPos.x, screenPos.y); //ilk 2 degisken baslangic konumu
	}
	Vector3 head;
    head.x = pos.x;
    head.y = pos.y;
    head.z = pos.z ;
    Vector3 screenHead = WorldToScreen(head, matrixview);
    DrawGoofyBox(hdc, screenHead.x - 10.f, screenHead.y - 30, screenPos.x + 30, screenPos.y + 20);
	}





LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {

	static HDC hdcBuffer = NULL;
	static HBITMAP hbmBuffer = NULL;
	switch (message) {

	case WM_CREATE: {
		hdcBuffer = CreateCompatibleDC(NULL);
		hbmBuffer = CreateCompatibleBitmap(GetDC(hwnd), width, height);
		SelectObject(hdcBuffer, hbmBuffer);
		SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
		SetLayeredWindowAttributes(hwnd, RGB(255, 255, 255), 0, LWA_COLORKEY);
		break;
	}
	case WM_ERASEBKGND: {
		return TRUE;
	}

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		FillRect(hdcBuffer, &ps.rcPaint, (HBRUSH)GetStockObject(WHITE_BRUSH));
		draw(hdcBuffer);
		BitBlt(hdc, 0, 0, width, height, hdcBuffer, 0, 0, SRCCOPY);
		EndPaint(hwnd, &ps);
		InvalidateRect(hwnd, 0, TRUE);
		break;
	}
	case WM_DESTROY: {
		DeleteDC(hdcBuffer);
		DeleteObject(hbmBuffer);
		PostQuitMessage(0);
	}
	default: {
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
		   return 0;
	}
}

int __stdcall WinMain(HINSTANCE instance, HINSTANCE pInstance, LPSTR lpCmd, int cmdShow) {
	const char* CLASS_NAME = "window class";
	WNDCLASSEX wc = { };
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = WndProc;
	wc.lpszClassName = CLASS_NAME;
	wc.hInstance = instance;
	wc.style = CS_HREDRAW | CS_VREDRAW;

	RegisterClassEx(&wc);

	HWND hwnd = CreateWindowEx(0, CLASS_NAME, "esp",
		WS_EX_TRANSPARENT | WS_EX_TOPMOST, CW_USEDEFAULT, CW_USEDEFAULT,
		width,
		height,
		NULL,
		NULL,
		instance,
		NULL
	);

	if (hwnd == NULL) {
		return 1;
	}

	SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	ShowWindow(hwnd, cmdShow);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}