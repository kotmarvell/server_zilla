﻿// Сервер, читающий присланный клиентом текст и выдирающий оттуда 2 числа (координаты)
#pragma warning(disable: 4996)
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <locale.h>
#include <iostream>
#pragma comment(lib, "ws2_32.lib")

// Прототип функции, обслуживающий подключившихся пользователей
DWORD WINAPI SexToClient(LPVOID client_socket);

// Количество активных пользователей
int nclients = 0;

void printUsers() {
	if (nclients != 0)
		printf("%d user on-line\n", nclients);
	else
		printf("No User on line\n");
}

int main()
{
	//FreeConsole();
	setlocale(0, "");
	char buff[1024]; // Буфер для инициализации работы с сокетами (подгрузка DLL?)

	printf("TCP SERVER \n");

	// Я так поняла тут какие-то метаданные, просто проигнорируй и вставляй везде где работаешь с сокетами.

		/*
		* Шаг 1 - Инициализация Библиотеки Сокетов
		* т.к. возвращенная функцией информация не используется
		* ей передается указатель на рабочий буфер, преобразуемый к указателю
		* на структуру WSADATA.
		* Такой прием позволяет сэкономить одну переменную, однако, буфер
		* должен быть не менее полкилобайта размером (структура WSADATA
		* занимает 400 байт)
		*/

	if (WSAStartup(0x0202, (WSADATA*)& buff[0]))
	{
		// Ошибка!
		printf("Error WSAStartup %d\n", WSAGetLastError());
		return -1;
	}

	// Шаг 2 - создание сокета
	// Что представляет собой этот объект - не важно. Думаю это просто целое число. 
	//Важно то, в какие функции мы можем его скармливать.

	// AF_INET - сокет IPv4
	// SOCK_STREAM - потоковый сокет (с установкой соединения)
	// 0 - по умолчанию выбирается TCP протокол (с SOCK_STREAM мы UDP и не сможем выбрать_.

	SOCKET mysocket = socket(AF_INET, SOCK_STREAM, 0);
	if (mysocket < 0) {
		// Ошибка!
		printf("Error socket %d\n", WSAGetLastError());
		WSACleanup(); // Деиницилизация библиотеки Winsock
		return -1;
	}


	/*
	* Мы хотим использовать данный сокет, чтобы повесить сервер на заданный порт (MY_PORT == 666).
	* sockaddr_in описывает адрес на который мы вешаем. Я так поняла, нам интересен здесь только порт, остальное дефолтное.
	* К этому серверу можно подсоединиться через localhost:порт, 127.0.0.1:порт с данной машины, из локальной сети - 192.168.Локальный.адрес:порт,
	* из Интернета - (айпи адрес компьютера):порт, но обычно айпишник относится к роутеру и нужно в роутере настраивать прокидывание сообщений,
	* приходящих на порт, на конкретный компьютер.
	*/

	const int MY_PORT = 666; // Константа(номер порта)
	sockaddr_in local_addr;
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(MY_PORT); // (не забываем о сетевом порядке!!!) Это такая мерзкая вещь, называется byte ordering. Я не разбираюсь какой тут принят вариант.
	local_addr.sin_addr.s_addr = 0; // Сервер принимает подключения на все свои IP-адреса


	/*
	* bind собственно вешает сервер на адрес с портом.
	* Т.е. теперь всё, что приходит на данный порт, операционка  будет направлять этой программе (после вызова listen).
	* 0 означает успешный bind, иначе ошибка(например другая программа заняла порт).
	*/

	if (bind(mysocket, (sockaddr*)& local_addr, sizeof(local_addr)) != 0)
	{
		// Ошибка
		printf("Error bind %d\n", WSAGetLastError());
		closesocket(mysocket); // Закрываем сокет
		// Учитывая что программа завершается, не факт что это нужно. 
		// В случае tcp это означает прервать все соединения и перестать слушать порт.
		WSACleanup();
		return -1;
	}

	// Шаг 4 - ожидание подключений. Т.е. теперь сервер начинает принимать приходящие на порт сообщения.
	// размер очереди - 0x100
	if (listen(mysocket, 0x100) != 0)
	{
		// Ошибка
		printf("Error listen %d\n", WSAGetLastError());
		closesocket(mysocket);
		WSACleanup();
		return -1;
	}

	printf("Ожидание подключений...\n");

	/*
	* Теперь мы будем принимать соединения.
	* client_socket используется для общения с клиентом,
	* в client_addr записывается информация об адресе клиента (хотя тут она особо не нужна).
	*/

	// Шаг 5 - извлекаем сообщение из очереди

	SOCKET client_socket; // Cокет для клиента
	sockaddr_in client_addr; // Адрес клиента (заполняется системой)

	// Функции accept необходимо передать размер структуры
	int client_addr_size = sizeof(client_addr);


	// Цикл извлечения запросов на подключение из очереди

	/*
	* accept блокируется, пока не подсоединится новый клиент.
	* Блокируется - в смысле, как например блокируется cin >> пока мы что-нибудь не введем.
	* Т.е. код не исполняется дальше.
	*/
	while ((client_socket = accept(mysocket, (sockaddr*)& client_addr, \
		& client_addr_size)))
	{
		// Теперь в client_socket хранится сокет, через который мы можем читать что клиент нам шлет, 
		// и слать ему сообщения, пока есть соединение.
		nclients++; // Увеличиваем счетчик подключившихся клиентов

		// Получаем имя хоста
		HOSTENT* hst;
		hst = gethostbyaddr((char*)& client_addr.sin_addr.s_addr, 4, AF_INET);

		// Вывод сведений о клиенте
		printf("+%s [%s] new connect!\n",
			(hst) ? hst->h_name : "", inet_ntoa(client_addr.sin_addr));
		printUsers();

		/*
		* Вызов нового потока для обслужвания клиента
		* Да, для этого рекомендуется использовать _beginthreadex
		* но, поскольку никаких вызовов функций стандартной Си библиотеки
		* поток не делает, можно обойтись и CreateThread
		*
		*
		* Каждое новое соединение будет обрабатываться в новом потоке.
		* CreateThread принимает третьим аргументом указатель на функцию, откуда он начинает выполняться.
		* 4м можно передать адрес её одного аргумента. Вроде поток завершится после функции (надеюсь).
		* Итого, создастся новый поток, после чего исполнение этого цикла продолжится в текущем потоке,
		* а новый поток будет паралллельно или конкурентно обрабатывать клиента.
		*/

		DWORD thID;
		CreateThread(NULL, NULL, SexToClient, &client_socket, NULL, &thID);
	}
	return 0;
}


// Эта функция создается в отдельном потоке
// и обсуживает очередного подключившегося клиента независимо от остальных

DWORD WINAPI SexToClient(LPVOID client_socket)
{
	// Мы получили указатель на client_socket, со стертым типом (LPVOID вероятно это void*)
	SOCKET my_sock;
	my_sock = ((SOCKET*)client_socket)[0];
	// можно переписать как SOCKET my_sock = *(((SOCKET*)client_socket)

	char buff[10]; // В этот буфер мы будем считывать сообщения от клиента.
	//10 - Максимальный размер сообщения
	//const char sHELLO[] = "Hello world\r\n";
	/*
	* Отправляем клиенту приветствие
	* send принимает сокет, адрес буфера, размер (сколько байт из буфера отправить).
	* sizeof здесь работает потому что размер массива sHELLO известен при компиляции.
	* Если будут динамические массивы, sizeof будет возвращать размер указателя.
	*/
	/*send(my_sock, sHELLO, sizeof(sHELLO), 0);*/

	int x = 0;
	int y = 0;
	while (1) {

		/*
		* recv читает, что в настоящий момент пришло на сокет.
		* Первый аргумент сокет, второй адрес буфера, куда записывать пришедшие сообщения, третий - размер буфера.
		* Вроде пока данных не пришло, recv блокируется.
		* В буфер будет прочитано min(число доступных байт, размер буфера).
		* Если соединение закрыто нормально, вернет 0, если ненормально - вернет что-то меньше нуля.
		* В противном случае возращает количество прочитанных байт.
		*/

		int bytes_recv = recv(my_sock, &buff[0], sizeof(buff), 0);

		if (bytes_recv > 0)
		{

			if (buff[0] == 'C') //Курсор
			{
				char xm[4];
				char ym[4];

				for (int i = 2; i < 6; i++) {
					xm[i - 2] = buff[i];
				}

				for (int i = 6; i < 10; i++) {
					ym[i - 6] = buff[i];
				}


				x = atoi(xm); //Строка в целое число
				y = atoi(ym);

				SetCursorPos(x, y);
				std::cout << x << " " << y << std::endl;
				/*if (x == 0 && y == 0)
					mouse_event(MOUSEEVENTF_LEFTDOWN, x, y, 0, 0);*/

			}
			else if (buff[0] == 'M') { //Кнопки на мыши
				for (int i = 0; i < 10; i++)
					std::cout << buff[i];
				std::cout << " " << x << " " << y << std::endl;
				if (buff[8] == '2') {
					mouse_event(MOUSEEVENTF_RIGHTDOWN, x, y, 0, 0);
					mouse_event(MOUSEEVENTF_RIGHTUP, x, y, 0, 0);
				}
				else if (buff[8] == '1') {
					mouse_event(MOUSEEVENTF_LEFTDOWN, x, y, 0, 0);
					mouse_event(MOUSEEVENTF_LEFTUP, x, y, 0, 0);
				}
				else if (buff[8] == '4') {
					mouse_event(MOUSEEVENTF_MIDDLEDOWN, x, y, 0, 0);
					mouse_event(MOUSEEVENTF_MIDDLEUP, x, y, 0, 0);

				}

			}
			else if (buff[0] == 'K') {
				for (int i = 0; i < 10; i++)
					std::cout << buff[i];
				char codeKey[5];
				int codeKeyInt = 0;
				for (int i = 5; i < 10; i++)
					codeKey[i - 5] = buff[i];
				codeKeyInt = atoi(codeKey);
				//std::cout << codeKeyInt << std::endl;
				keybd_event(codeKeyInt, 0, 0, 0);
				keybd_event(codeKeyInt, 0, KEYEVENTF_KEYUP, 0);
			}

		}
	}

	/*
	* Если мы здесь, то произошел выход из цикла по причине
	* возращения функцией recv ошибки - соединение с клиентом разорвано.
	* Или может даже нормального разрыва.
	*/
	nclients--; // Уменьшаем счетчик активных клиентов
	printf("-disconnect\n");
	printUsers();

	closesocket(my_sock); // Закрываем сокет. Да, это си, тут все надо ручками закрывать.
	return 0;
}