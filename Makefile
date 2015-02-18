build:
	msbuild dll\ToastNotification.sln /p:configuration=Release /p:Platform=win32
	copy dll\Release\ToastNotification.dll ToastNotification.dll
	msbuild dll\ToastNotification.sln /p:configuration=Release /p:Platform=x64
	copy dll\x64\Release\ToastNotification.dll ToastNotification64.dll

clean:
	msbuild dll\ToastNotification.sln /p:configuration=Release /p:Platform=win32 /t:Clean
	msbuild dll\ToastNotification.sln /p:configuration=Release /p:Platform=x64 /t:Clean

all: build
