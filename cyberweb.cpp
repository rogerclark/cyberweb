#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define CW_WNDCLASS_MAINWINDOW L"CwMainWindow"

enum { RESPONSE_BUFFER_MAX = 2048 };

typedef struct CW_APP_STATE {
	HINSTANCE instance;
	HWND hWndMain;
	HWND hWndAddressBar;
	HWND hWndGoButton;
	char responseBuffer[RESPONSE_BUFFER_MAX];
	UINT responseLength;
} CW_APP_STATE;

CW_APP_STATE appState = { 0 };



BOOL CwNavigate(const wchar_t* url) {
	BOOL success = FALSE;
	enum { HOSTNAME_BUFFER_MAX = 256 };
	char hostname[HOSTNAME_BUFFER_MAX] = { 0 };
	const char* resource = "/";

	WideCharToMultiByte(CP_ACP, 0, url, lstrlen(url), hostname, HOSTNAME_BUFFER_MAX, "?", NULL);

	struct hostent* host = gethostbyname(hostname);
	if (host == NULL) {
		// handle the error later
	} else {
		int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sock == -1) {
			// handle the error later
		} else {
			sockaddr_in server = { 0 };
			server.sin_port = htons(80);
			server.sin_family = AF_INET;
			server.sin_addr.s_addr = *((unsigned long*)host->h_addr_list[0]);

			int connectResult = connect(sock, (sockaddr*)&server, sizeof(sockaddr_in));
			if (connectResult == -1) {
				// handle the error later
			} else {
				enum { REQUEST_BUFFER_MAX = 256 };
				char requestBuffer[REQUEST_BUFFER_MAX];

				// GET /index.html HTTP/1.1\r\n
				// Host: cyberweb.surf\r\n
				// \r\n

				int requestLength = wsprintfA(requestBuffer,
					"GET %s HTTP/1.1\r\n"
					"Host: %s\r\n"
					"\r\n", resource, hostname);
				int sendResult = send(sock, requestBuffer, requestLength, 0);
				if (sendResult == -1) {
					// handle the error later
				} else {					
					int recvResult = recv(sock, appState.responseBuffer, RESPONSE_BUFFER_MAX, 0);

					if (recvResult == -1) {
						// handle the error later
					} else if (recvResult == 0) {
						OutputDebugStringW(L"Connection closed gracefully");
					} else {
						appState.responseLength = recvResult;
					}

					success = TRUE;
				}
			}
		}
	}
	return success;
}

LRESULT CALLBACK CwMainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CREATE: {
		appState.hWndMain = hWnd;

		appState.hWndAddressBar = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"cyberweb.link",
			WS_CHILD, 0, 0, 0, 0, hWnd, NULL, appState.instance, NULL);
		ShowWindow(appState.hWndAddressBar, SW_SHOW);

		appState.hWndGoButton = CreateWindowEx(WS_EX_CLIENTEDGE, L"BUTTON", L"Go",
			WS_CHILD | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, NULL, appState.instance, NULL);
		ShowWindow(appState.hWndGoButton, SW_SHOW);

		break;
	}
	case WM_COMMAND: {
		UINT notifyCode = HIWORD(wParam);
		switch (notifyCode) {
			case BN_CLICKED: {
				enum { URL_BUFSIZE = 256 };
				wchar_t url[URL_BUFSIZE];
				GetWindowText(appState.hWndAddressBar, url, URL_BUFSIZE);

				CwNavigate(url);

				// force WM_PAINT
				InvalidateRect(hWnd, NULL, TRUE);
				
				break;
			}
		}
		break;
	}
	case WM_SIZE: {
		enum { UI_CONTROL_PADDING = 2 };

		RECT rectClient;
		GetClientRect(hWnd, &rectClient);

		UINT yPosition = 0;

		{
			UINT goButtonWidth = 35;
			UINT addressBarWidth = rectClient.right - rectClient.left - goButtonWidth - (UI_CONTROL_PADDING * 2);
			UINT goButtonXPosition = addressBarWidth + UI_CONTROL_PADDING;
			UINT addressBarHeight = 25;

			MoveWindow(appState.hWndAddressBar,
				UI_CONTROL_PADDING, UI_CONTROL_PADDING,
				addressBarWidth, addressBarHeight, TRUE);

			MoveWindow(appState.hWndGoButton,
				goButtonXPosition, UI_CONTROL_PADDING, goButtonWidth, addressBarHeight, TRUE);

			yPosition += addressBarHeight + UI_CONTROL_PADDING;
		}

		break;
	}
	case WM_PAINT: {
		RECT rectClient;
		PAINTSTRUCT ps = { 0 };
		const wchar_t* demoText = L"Welcome to CyberWeb";

		BeginPaint(hWnd, &ps);

		GetClientRect(hWnd, &rectClient);
		FillRect(ps.hdc, &rectClient, (HBRUSH)GetSysColorBrush(COLOR_WINDOW));

		RECT rectDemoText = rectClient;
		rectDemoText.top = 29;

		if (appState.responseLength == 0) {
			DrawTextW(ps.hdc, demoText, lstrlen(demoText), &rectDemoText, 0);
		} else {
			DrawTextA(ps.hdc, appState.responseBuffer, appState.responseLength, &rectDemoText, 0);
		}

		EndPaint(hWnd, &ps);
		break;
	}
	case WM_CLOSE: {
		break;
	}
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

BOOL CwRegisterWindowClass(HINSTANCE instance) {
	WNDCLASSEX wc = { 0 };

	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hInstance = instance;
	wc.lpfnWndProc = CwMainWndProc;
	wc.lpszClassName = CW_WNDCLASS_MAINWINDOW;

	return (RegisterClassEx(&wc) != 0);
}


int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int show) {
	WSADATA wsadata = { 0 };
	WSAStartup(MAKEWORD(2, 2), &wsadata);

	if (CwRegisterWindowClass(instance)) {
		appState.instance = instance;

		CreateWindowEx(0, CW_WNDCLASS_MAINWINDOW, L"CyberWeb", WS_OVERLAPPEDWINDOW,
			0, 0, 640, 480, NULL, NULL, instance, NULL);
		ShowWindow(appState.hWndMain, SW_SHOW);

		MSG msg = { 0 };
		while (GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	WSACleanup();

	return 0;
}