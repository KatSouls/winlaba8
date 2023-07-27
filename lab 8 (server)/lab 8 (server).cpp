#include <iostream>
#include <windows.h>
using namespace std;

// поток чтения сообщений от клиента
DWORD WINAPI readThread(PVOID param) { //функция - точка входа в поток
	HANDLE* hName = (HANDLE*)param;//hName - указатель
	DWORD readed;
	char message[128] = { 0 };
	while (true) {
		// читаем сообщение
		if (!ReadFile(hName, message, 128, &readed, NULL)) {
			/*
			BOOL ReadFile(
			HANDLE hFile,                // дескриптор файла
			LPVOID lpBuffer,             // буфер данных
			DWORD nNumberOfBytesToRead,  // число байтов для чтения
			LPDWORD lpNumberOfBytesRead, // число прочитанных байтов
			LPOVERLAPPED lpOverlapped    // асинхронный буфер
			);*/
			cout << "Client disconnected\n"; //Клиент отключен
			return NULL;
		}
		// выводим его
		cout << "Client: " << message << "\n";
		// если сообщение == exit, то завершаем цикл и выходим из потока
		if (strcmp(message, "exit") == 0)
		{
			break;
		}
	}
	return NULL;
}

// поток записи сообщений клиенту
DWORD WINAPI writeThread(PVOID param) {
	HANDLE* hName = (HANDLE*)param;
	DWORD writed;
	char message[128] = { 0 };
	while (true) {
		// читаем сообщение с клавиатуры
		cin.getline(&message[0], 128);
		// отправляем сообщение клиенту
		if (!WriteFile(hName, message, 128, &writed, NULL)) {
			/*
			BOOL WriteFile(
			HANDLE hFile,                    // дескриптор файла
			LPCVOID lpBuffer,                // буфер данных
			DWORD nNumberOfBytesToWrite,     // число байтов для записи
			LPDWORD lpNumberOfBytesWritten,  // число записанных байтов
			LPOVERLAPPED lpOverlapped        // асинхронный буфер
			);*/
			cout << "Connection not established" << endl; //Соединение не установлено
			return NULL;
		}
		// если сообщение == exit, то завершаем цикл и выходим из потока
		if (strcmp(message, "exit") == 0)
		{
			break;
		}
	}
	return NULL;
}


int main()
{
	SetConsoleOutputCP(1251);
	SetConsoleCP(1251);

	//дескрипторы именованных каналов
	HANDLE hPipeClientToServer;
	HANDLE hPipeServerToClient;
	//атрибуты защиты
	SECURITY_ATTRIBUTES sa;
	//дескриптор безопасности
	SECURITY_DESCRIPTOR sd;

	//заполняем атрибуты безопасности, отключаем наследование дескрипторов
	sa.bInheritHandle = FALSE;
	sa.nLength = sizeof(&sa);

	// инициализируем дескриптор безопасности
	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	/*BOOL InitializeSecurityDescriptor(
	[out] PSECURITY_DESCRIPTOR pSecurityDescriptor, //Указатель на структуру SECURITY_DESCRIPTOR, которую инициализирует функция.
	[in]  DWORD                dwRevision           //Уровень ревизии, назначаемый дескриптору безопасности. Этот параметр должен быть SECURITY_DESCRIPTOR_REVISION.
		); 
	*/

	//доступ разрешен всем
	SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
	/*BOOL SetSecurityDescriptorDacl(
	PSECURITY_DESCRIPTOR pSecurityDescriptor, // Указатель на структуру SECURITY_DESCRIPTOR , к которой функция добавляет DACL
	BOOL bDaclPresent,						  // Флаг, указывающий на наличие DACL в дескрипторе безопасности.
	PACL pDacl,								  // Указатель на структуру ACL , указывающую DACL для дескриптора безопасности. 
	BOOL bDaclDefaulted						  // Флаг, указывающий источник DACL.
	);
	*/

	//созданный дескриптор используется в SECURITY_ATTRIBUTES
	sa.lpSecurityDescriptor = &sd;
	
	//создаем именованный канал с доступом для всех
	hPipeClientToServer = CreateNamedPipe(L"\\\\.\\pipe\\Lab8_ClientToServer", PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_MESSAGE | PIPE_WAIT | PIPE_ACCEPT_REMOTE_CLIENTS, 255, 0, 0, INFINITE, &sa);
	if (hPipeClientToServer == INVALID_HANDLE_VALUE) {
		cerr << "Error while client to server pipe creating: " << GetLastError() << endl;
		cin.get();
		return 0;
	}

	hPipeServerToClient = CreateNamedPipe(L"\\\\.\\pipe\\Lab8_ServerToClient", PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_MESSAGE | PIPE_WAIT | PIPE_ACCEPT_REMOTE_CLIENTS, 255, 0, 0, INFINITE, &sa);
	if (hPipeServerToClient == INVALID_HANDLE_VALUE) {
		cerr << "Error while server to client pipe creating: " << GetLastError() << endl;
		cin.get();
		return 0;
	}


	char ComputerName[MAX_PATH]; // переменная для имени сервера
	DWORD size = 256;
	GetComputerNameA(ComputerName, &size); // получаем имя компьютера
	cout << "Server name: " << ComputerName << endl;

	// ожидаем подключения клиента к pipe
	if (!ConnectNamedPipe(hPipeClientToServer, NULL)) {
		cout << "Connection error: " << GetLastError() << endl; //Ошибка соединения:
		return 0;
	}
	cout << "Connection established" << endl; //Соединение установлено

	HANDLE hWrite;
	HANDLE hRead; //дескриптор
	// создаём потоки для чтения и записи
	hWrite = CreateThread(NULL, 0, writeThread, hPipeServerToClient, 0, NULL);
  /*HANDLE CreateThread(
	LPSECURITY_ATTRIBUTES lpThreadAttributes, // дескриптор защиты
	SIZE_T dwStackSize,                       // начальный размер стека
	LPTHREAD_START_ROUTINE lpStartAddress,    // функция потока
	LPVOID lpParameter,                       // параметр потока
	DWORD dwCreationFlags,                    // опции создания
	LPDWORD lpThreadId                        // идентификатор потока
	);*/
	hRead = CreateThread(NULL, 0, readThread, hPipeClientToServer, 0, NULL);
	WaitForSingleObject(hWrite, INFINITE); // ожидаем завершения потока

	system("pause");
	return 0;
}
