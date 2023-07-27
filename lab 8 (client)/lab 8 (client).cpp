#include <iostream>
#include <windows.h>
using namespace std;

LPWSTR CharToLPWSTR(LPCSTR char_string) //указатель на строку символов
{
	LPWSTR res;
	DWORD res_len = MultiByteToWideChar(1251, 0, char_string, -1, NULL, 0);
	/*int MultiByteToWideChar(
    UINT CodePage,                                     // Кодовая страница для выполнения преобразования
    DWORD dwFlags,                                     // Флаги, указывающие тип преобразования
    _In_NLS_string_(cbMultiByte)LPCCH lpMultiByteStr,  // Указатель на преобразуемую строку символов.
    int cbMultiByte,                                   // Размер строки в байтах, указанной параметром lpMultiByteStr .
	                                                   // значение -1,если строка завершается нулем. если cbMultiByte равно 0, функция завершается ошибкой.
	LPWSTR lpWideCharStr,                              // Указатель на буфер, получающий преобразованную строку.
    int cchWideChar                                    // Размер буфера в символах, указанного lpWideCharStr
    );
	*/
	res = (LPWSTR)GlobalAlloc(GPTR, (res_len + 1) * sizeof(WCHAR));
	MultiByteToWideChar(1251, 0, char_string, -1, res, res_len);
	return res;
}

// поток для чтения сообщений с сервера
DWORD WINAPI readThread(PVOID param) { //функция - точка входа в поток
	HANDLE* hName = (HANDLE*)param; //hName - указатель
	DWORD readed;
	char message[128] = { 0 };
	while (true) {
		// читаем сообщение
		if (!ReadFile(hName, message, 128, &readed, NULL)) {
			/*BOOL ReadFile(
            HANDLE hFile,                // дескриптор файла
            LPVOID lpBuffer,             // буфер данных
            DWORD nNumberOfBytesToRead,  // число байтов для чтения
            LPDWORD lpNumberOfBytesRead, // число прочитанных байтов
            LPOVERLAPPED lpOverlapped    // асинхронный буфер
            );
			*/
			continue;
			cout << "Client disconnected" << endl; //Клиент отключен
		}
		// выводим его
		cout << "Server: " << message << "\n";
		// если сообщение == exit, то завершаем цикл и выходим из потока
		if (strcmp(message, "exit") == 0)
		{
			break;}
	} return NULL; }

// поток для отправки сообщений серверу
DWORD WINAPI writeThread(PVOID param) {
	HANDLE* hName = (HANDLE*)param;
	DWORD writed; //DWORD — 32-битное беззнаковое целое
	cout << endl;
	char message[128] = { 0 };
	while (true) {
		// читаем сообщение с клавиатуры
		cin.getline(&message[0], 128);
		if (!WriteFile(hName, message, 128, &writed, NULL)) {
			//BOOL WriteFile(
			//HANDLE hFile,                    // дескриптор файла
			//LPCVOID lpBuffer,                // буфер данных
			//DWORD nNumberOfBytesToWrite,     // число байтов для записи
			//LPDWORD lpNumberOfBytesWritten,  // число записанных байтов
			//LPOVERLAPPED lpOverlapped        // асинхронный буфер
			//);
			cout << "Connection not established" << endl; //Соединение не установлено
		}
		if (strcmp(message, "exit") == 0)
		{
			break;}
	}return NULL; }

int main()
{
	SetConsoleOutputCP(1251);
	SetConsoleCP(1251);
	char ServerName[MAX_PATH];

	cout << "Enter server name: ";
	cin >> ServerName;
	 
	char pipeNameClientToServer[MAX_PATH]; //переменная для имени канала
	sprintf_s(pipeNameClientToServer, "\\\\%s\\pipe\\Lab8_ClientToServer", ServerName); //путь
	char pipeNameServerToClient[MAX_PATH]; //переменная для имени канала
	sprintf_s(pipeNameServerToClient, "\\\\%s\\pipe\\Lab8_ServerToClient", ServerName); //путь
	
	if (!WaitNamedPipe(CharToLPWSTR(pipeNameClientToServer), 1000000) || !WaitNamedPipe(CharToLPWSTR(pipeNameServerToClient), 100000)) { //попытка соединения
		/*BOOL WaitNamedPipeA(
		LPCSTR lpNamedPipeName,     //Имя именованного канала. Строка должна содержать имя компьютера, на котором выполняется процесс сервера.
		DWORD  nTimeOut             //Количество миллисекундах, которое функция ожидает доступности экземпляра именованного канала.
		);*/
		cout << "Connection not established: " << GetLastError() << endl; //Соединение не установлено:
		system("pause");
		return 0; }
	cout << "Connection established\n"; //Соединение установлено

	HANDLE hPipeClientToServer; //дескриптор
	HANDLE hPipeServerToClient; //дескриптор

	//создаем именованный канал с доступом для всех
	hPipeClientToServer = CreateFile(CharToLPWSTR(pipeNameClientToServer), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	hPipeServerToClient = CreateFile(CharToLPWSTR(pipeNameServerToClient), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);;
	if (hPipeClientToServer == INVALID_HANDLE_VALUE || hPipeServerToClient == INVALID_HANDLE_VALUE) {
		cout << "Can't open pipe:" << GetLastError() << endl; // не может открыть pipe
		system("pause");
		return 0;
	}

	HANDLE readTh = NULL; //HANDLE - дескриптор, т.е. число, 
	//с помощью которого можно идентифицировать ресурс.
	HANDLE writeTh = NULL; // дескриптор

	writeTh = CreateThread(NULL, 0, writeThread, hPipeClientToServer, 0, NULL);
	//HANDLE CreateThread(
	//	LPSECURITY_ATTRIBUTES lpThreadAttributes, // дескриптор защиты
	//	SIZE_T dwStackSize,                       // начальный размер стека
	//	LPTHREAD_START_ROUTINE lpStartAddress,    // функция потока
	//	LPVOID lpParameter,                       // параметр потока
	//	DWORD dwCreationFlags,                    // опции создания
	//	LPDWORD lpThreadId                        // идентификатор потока
	//	);
	readTh = CreateThread(NULL, 0, readThread, hPipeServerToClient, 0, NULL);
	WaitForSingleObject(writeTh, INFINITE);
	/*DWORD WaitForSingleObject(
	 HANDLE hHandle,           //Дескриптор объекта. 
	 DWORD  dwMilliseconds     //Интервал ожидания в миллисекундах
	);*/

	system("pause");
	return 0;
}
