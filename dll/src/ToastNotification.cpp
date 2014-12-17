/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <windows.ui.notifications.h>
#include <windows.data.xml.dom.h>
#include <wrl.h>
#include <wchar.h>

#pragma comment(lib, "delayimp")
#pragma comment(lib, "runtimeobject")

using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Data::Xml::Dom;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

wchar_t* sAppId = nullptr;

typedef bool (*EventCallback)();

class ToastNotificationHandler {
	typedef ABI::Windows::UI::Notifications::IToastNotification IToastNotification;
	typedef ABI::Windows::UI::Notifications::IToastDismissedEventArgs IToastDismissedEventArgs;
	typedef ABI::Windows::UI::Notifications::IToastNotificationManagerStatics IToastNotificationManagerStatics;
	typedef ABI::Windows::UI::Notifications::ToastTemplateType ToastTemplateType;
	typedef ABI::Windows::Data::Xml::Dom::IXmlNode IXmlNode;
	typedef ABI::Windows::Data::Xml::Dom::IXmlDocument IXmlDocument;

	typedef __FITypedEventHandler_2_Windows__CUI__CNotifications__CToastNotification_IInspectable_t ToastActivationHandler;
	typedef __FITypedEventHandler_2_Windows__CUI__CNotifications__CToastNotification_Windows__CUI__CNotifications__CToastDismissedEventArgs ToastDismissHandler;

public:
	ToastNotificationHandler() : mTitle(nullptr), mMessage(nullptr), mName(nullptr), mActivateCallback(nullptr), mDismissCallback(nullptr)
	{
	}

	~ToastNotificationHandler();

	bool Init(const wchar_t* aTitle, const wchar_t* aMessage, const wchar_t* aName);
	bool DisplayNotification(EventCallback aActivate, EventCallback aDismiss);


	ComPtr<IXmlDocument> ToastNotificationHandler::InitializeXmlForTemplate(ToastTemplateType templateType);
	bool CreateWindowsNotificationFromXml(IXmlDocument* aXml);

	HRESULT OnActivate(IToastNotification* aNotification, IInspectable* aInspectable);
	HRESULT OnDismiss(IToastNotification* aNotification, IToastDismissedEventArgs* aInspectable);

	ComPtr<IToastNotificationManagerStatics> mToastNotificationManagerStatics;
	HSTRING mTitle;
	HSTRING mMessage;
	wchar_t* mName;

	EventCallback mActivateCallback;
	EventCallback mDismissCallback;
};

ToastNotificationHandler::~ToastNotificationHandler()
{
	if (mTitle) {
		WindowsDeleteString(mTitle);
	}
	if (mMessage) {
		WindowsDeleteString(mMessage);
	}
	if (mName) {
		free(mName);
	}
}

bool
ToastNotificationHandler::Init(const wchar_t* aTitle, const wchar_t* aMessage, const wchar_t* aName)
{
	HRESULT hr;
	hr = WindowsCreateString(aTitle, wcslen(aTitle), &mTitle);
	if (FAILED(hr)) {
		return false;
	}
	hr = WindowsCreateString(aMessage, wcslen(aMessage), &mMessage);
	if (FAILED(hr)) {
		return false;
	}
	if (aName) {
		mName = _wcsdup(aName);
		if (!mName) {
			return false;
		}
	}

	return true;
}

static bool
SetNodeValueString(HSTRING inputString, ComPtr<IXmlNode> node, ComPtr<IXmlDocument> xml)
{
	ComPtr<IXmlText> inputText;
	ComPtr<IXmlNode> inputTextNode;
	ComPtr<IXmlNode> appendedChild;
	HRESULT hr;

	hr = xml->CreateTextNode(inputString, &inputText);
	if (FAILED(hr)) {
		return false;
	}
	hr = inputText.As(&inputTextNode);
	if (FAILED(hr)) {
		return false;
	}
	hr = node->AppendChild(inputTextNode.Get(), &appendedChild);
	if (FAILED(hr)) {
		return false;
	}

	return true;
}

ComPtr<IXmlDocument>
ToastNotificationHandler::InitializeXmlForTemplate(ToastTemplateType templateType)
{
	HRESULT hr;
	hr = GetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(), mToastNotificationManagerStatics.GetAddressOf());
	if (FAILED(hr)) {
		return nullptr;
	}

	ComPtr<IXmlDocument> toastXml;
	mToastNotificationManagerStatics->GetTemplateContent(templateType, &toastXml);
	return toastXml;
}

bool
ToastNotificationHandler::CreateWindowsNotificationFromXml(IXmlDocument* aXml)
{
	ComPtr<IToastNotificationFactory> factory;
	HRESULT hr;
	hr = GetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(), factory.GetAddressOf());
	if (FAILED(hr)) {
		OutputDebugStringW(L"Cannot create IToastNotification\n");
		return false;
	}

	ComPtr<IToastNotification> notification;
	hr = factory->CreateToastNotification(aXml, &notification);
	if (FAILED(hr)) {
		OutputDebugStringW(L"Cannot create IToastNotification\n");
		return false;
	}

	EventRegistrationToken activatedToken;
	hr = notification->add_Activated(Callback<ToastActivationHandler>(this, &ToastNotificationHandler::OnActivate).Get(), &activatedToken);
	if (FAILED(hr)) {
		return false;
	}
	EventRegistrationToken dismissedToken;
	hr = notification->add_Dismissed(Callback<ToastDismissHandler>(this, &ToastNotificationHandler::OnDismiss).Get(), &dismissedToken);
	if (FAILED(hr)) {
		return false;
	}

	ComPtr<IToastNotifier> notifier;
	hr = mToastNotificationManagerStatics->CreateToastNotifierWithId(HStringReference(sAppId).Get(), &notifier);
	if (FAILED(hr)) {
		return false;
	}
	hr = notifier->Show(notification.Get());
	if (FAILED(hr)) {
		return false;
	}

	return true;
}

bool
ToastNotificationHandler::DisplayNotification(EventCallback aActivate, EventCallback aDismiss)
{
	ComPtr<IXmlDocument> toastXml = InitializeXmlForTemplate(ToastTemplateType::ToastTemplateType_ToastText03);
	HRESULT hr;

	ComPtr<IXmlNodeList> toastTextElements;
	hr = toastXml->GetElementsByTagName(HStringReference(L"text").Get(), &toastTextElements);
	if (FAILED(hr)) {
		return false;
	}

	ComPtr<IXmlNode> titleTextNodeRoot;
	hr = toastTextElements->Item(0, &titleTextNodeRoot);
	if (FAILED(hr)) {
		return false;
	}
	ComPtr<IXmlNode> msgTextNodeRoot;
	hr = toastTextElements->Item(1, &msgTextNodeRoot);
	if (FAILED(hr)) {
		return false;
	}

	SetNodeValueString(mTitle, titleTextNodeRoot.Get(), toastXml.Get());
	SetNodeValueString(mMessage, msgTextNodeRoot.Get(), toastXml.Get());

	mActivateCallback = aActivate;
	mDismissCallback = aDismiss;

	return CreateWindowsNotificationFromXml(toastXml.Get());
}

HRESULT
ToastNotificationHandler::OnActivate(IToastNotification* aNotification, IInspectable* aInspectable)
{
	if (mActivateCallback) {
		mActivateCallback();
	}
	return S_OK;
}

HRESULT
ToastNotificationHandler::OnDismiss(IToastNotification* aNotification, IToastDismissedEventArgs* aArgs)
{
	if (mDismissCallback) {
		mDismissCallback();
	}
	delete this;
	return S_OK;
}

extern "C"
__declspec(dllexport)
bool WINAPI
SetAppId()
{
	if (!sAppId) {
		WCHAR wszFilename[MAX_PATH];
		GetModuleFileNameW(NULL, wszFilename, MAX_PATH);
		wchar_t* slash = wcsrchr(wszFilename, '\\');
		if (slash) {
			*slash = '\0';
		}

		HKEY key;
		if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Mozilla\\Firefox\\TaskBarIDs", 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS) {
			WCHAR value[MAX_PATH];
			DWORD type = REG_SZ;
			DWORD len = sizeof(value);
			if (RegQueryValueExW(key, wszFilename, NULL, &type, (LPBYTE) value, &len) == ERROR_SUCCESS) {
				sAppId = _wcsdup(value);
			}
			RegCloseKey(key);
		}
	}

	return !!sAppId;
}

// external API to show toast notification via JS-CTYPES
extern "C"
__declspec(dllexport)
bool WINAPI
DisplayToastNotification(const wchar_t* aTitle, const wchar_t* aMessage, const wchar_t* aName, void* aCallbackActive, void* aCallbackDismiss)
{
	if (!sAppId) {
		return false;
	}

	ToastNotificationHandler* handler = new ToastNotificationHandler();

	if (!handler->Init(aTitle, aMessage, aName)) {
		delete handler;
		return false;
	}

	if (!handler->DisplayNotification((EventCallback) aCallbackActive, (EventCallback) aCallbackDismiss)) {
		delete handler;
		return false;
	}

	return true;
}

extern "C"
__declspec(dllexport)
bool WINAPI
CloseToastNotification(const wchar_t* aName)
{
	return false;
}
