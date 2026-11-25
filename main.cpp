
#include <windows.h>
#include <iostream>
#include <string>
#include <cwchar>

extern DWORD CHECK_INTERVAL = 300;

// 弹窗信息
struct DialogInfo {
	DialogInfo() {
		dialog = 0;
		button = 0;
		matched = false;
	}
	HWND dialog;
	HWND button;
	bool matched;
};

// 枚举弹窗子控件
BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam)
{
	DialogInfo* info = reinterpret_cast<DialogInfo*>(lParam);

	char className[128] = {};
	char text[512] = {};

	GetClassName(hwnd, className, 128);
	GetWindowText(hwnd, text, 512);

	std::string controlText(text);

	// 检查弹窗提示内容
	if (controlText.find("IDM 主程序文件已损坏")
		!= std::string::npos)
	{
		info->matched = true;
	}

	// 查找继续下载按钮
	if (strcmp(className, "Button") == 0)
	{
		if (controlText.find("继续下载") != std::wstring::npos)
		{
			info->button = hwnd;
		}
	}

	return TRUE;
}

// 枚举顶层窗口
BOOL CALLBACK EnumWindowProc(HWND hwnd, LPARAM lParam)
{
	DialogInfo* result = reinterpret_cast<DialogInfo*>(lParam);

	char title[256] = {};
	char className[128] = {};

	GetWindowText(hwnd, title, 256);
	GetClassName(hwnd, className, 128);

	// 判断窗口标题
	if (strcmp(title, "Internet Download Manager") != 0)
		return TRUE;

	// 判断窗口类型
	if (strcmp(className, "#32770") != 0)
		return TRUE;

	DialogInfo info;
	info.dialog = hwnd;

	EnumChildWindows(
		hwnd,
		EnumChildProc,
		reinterpret_cast<LPARAM>(&info)
		);

	// 两个条件都满足才认为是目标弹窗
	if (info.matched && info.button != 0)
	{
		*result = info;
		return FALSE;
	}

	return TRUE;
}

// 查找目标弹窗
bool FindIDMDialog(DialogInfo& info)
{
	EnumWindows(
		EnumWindowProc,
		reinterpret_cast<LPARAM>(&info)
		);

	return info.dialog != 0;
}

// 强制启用并点击按钮
bool ForceClick(HWND button)
{
	if (!IsWindow(button))
		return false;

	char text[256] = {};
	GetWindowText(button, text, 256);

	std::cout << "[发现按钮] " << text << std::endl;

	// 强制启用按钮
	EnableWindow(button, TRUE);

	if (!IsWindowEnabled(button))
	{
		std::cout << "[失败] 无法启用按钮" << std::endl;
		return false;
	}

	std::cout << "[成功] 按钮已强制启用" << std::endl;

	// 立即执行点击
	DWORD_PTR result = 0;

	LRESULT ret = SendMessageTimeout(
		button,
		BM_CLICK,
		0,
		0,
		SMTO_ABORTIFHUNG | SMTO_BLOCK,
		2000,
		&result
		);

	if (ret != 0)
	{
		std::cout << "[成功] 已发送点击消息" << std::endl;
		return true;
	}

	std::cout << "[失败] 点击消息发送失败" << std::endl;

	return false;
}

int main()
{
	//SetConsoleOutputCP(CP_UTF8);

	std::cout << "IDM 自动监控程序已启动" << std::endl;

	// 记录已经处理的弹窗
	HWND lastDialog = 0;

	while (true)
	{
		DialogInfo info;

		if (FindIDMDialog(info))
		{
			// 避免同一个弹窗重复点击
			if (info.dialog != lastDialog)
			{
				std::cout << "\n[发现 IDM 损坏提示弹窗]" << std::endl;

				lastDialog = info.dialog;

				ForceClick(info.button);
			}
		}
		else
		{
			// 弹窗消失，允许下一次处理
			lastDialog = 0;
		}

		Sleep(CHECK_INTERVAL);
	}

	return 0;
}
