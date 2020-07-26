#include <conio.h>
#include <Windows.h>
#include <mmsystem.h>
#include <iostream>

#pragma comment (lib, "Winmm.lib")

//Nстуд. билета = 530732
//Размер блока = 5307320
//Число блоков = 189
//Динамический TLS

const unsigned int N = 1000000000;
const unsigned int BlockSize = 5307320;
const unsigned int BlockNumber = 189;
const unsigned int TLSCellCount = 1088;
const unsigned int ThreadCount = 1;

//Массив индексов ячеек памяти, связанных с каждой операцией суммирования
DWORD dwTlsIndex[ThreadCount] = { 0 };

//Дескрипторы потоков
HANDLE ThreadHandles[ThreadCount];

//События, необходимые для сигнализирования об окончании операции суммирования
HANDLE EventHandles[ThreadCount];

//Флаг, по которому происходит вывод результатов всеми потоками
bool Flush = false;

//Массив результатов выполнения каждого потока
unsigned int Results[ThreadCount] = { 0 };

//Функция, выполняемая потоком
DWORD WINAPI BlockFunc(LPVOID lpParameter)
{
	//unsigned int Pointer = *(unsigned int*)lpParameter;
	while (true)
	{
		LPVOID lpValue = TlsGetValue(dwTlsIndex[0]);

		if (!lpValue && (GetLastError() != ERROR_SUCCESS))
		{
			std::cout << "Ошибка при чтении содержимого TLS-ячейки." << std::endl;
			getch();
			return NULL;
		};

		if (!Flush)
		{
			//Вычисленная блочная сумма
			float BlockSum = 0;

			for (unsigned int i = 0; i < BlockSize; i++)
			{
				float x = (i + *(unsigned int*)lpParameter + 0.5) / (float)N;
				BlockSum += 4. / (float)(1 + x*x);
			};

			if (!TlsSetValue(dwTlsIndex[0], (LPVOID)((unsigned int)BlockSum + (unsigned int)lpValue)))
			{
				std::cout << "Ошибка при изменении содержимого TLS-ячейки." << std::endl;
				getch();
				return NULL;
			};
		}
		else
		{
			Results[0] = (unsigned int)lpValue;
			std::cout << "Вычисленная потоком сумма: " << Results[0] << std::endl;
		};

		//Сигнализируем о завершении операции
		if (!SetEvent(EventHandles[0]))
		{
			std::cout << "Ошибка при сигнализировании о завершении операции." << std::endl;
			getch();
			return NULL;
		};

		//Приостанавливаем поток
		if (SuspendThread(ThreadHandles[0]) == (DWORD)-1)
		{
			std::cout << "Ошибка при приостановлении потока." << std::endl;
			getch();
			return NULL;
		};
	};
};

void main(void)
{
	setlocale(LC_ALL, "Russian");

	//Создаём один поток
	unsigned short ThreadNumber = 0;

	//Передаём в каждый поток только один параметр - адрес начала следующего блока
	unsigned int Position = 0;

	//Создаём события
	EventHandles[0] = CreateEvent(NULL, FALSE, FALSE, "Event for the first thread.");

	if (!EventHandles[0])
	{
		std::cout << "Ошибка при создании события потока №" << ThreadNumber << '.' << std::endl;
		getch();
		return;
	};

	ThreadNumber++;
	if ((ThreadHandles[0] = CreateThread(NULL, NULL, BlockFunc, &Position, CREATE_SUSPENDED, NULL)) == NULL)
	{
		std::cout << "Ошибка при создании потока №" << ThreadNumber << '.' << std::endl;
		getch();
		return;
	};

	//Выделяем TLS-ячейки памяти для каждого потока
	dwTlsIndex[0] = TlsAlloc();

	if (dwTlsIndex[0] == TLS_OUT_OF_INDEXES)
	{
		std::cout << "Ошибка при выделении TLS-ячейки." << std::endl;
		getch();
		return;
	};

	//Начальное значение счётчика
	unsigned long Time = timeGetTime();

	//Цикл по числу блоков
	for (unsigned int i = 0; i < BlockNumber; i++, Position += BlockSize)
	{
		ThreadNumber = 1;

		//Для единственного потока
		Position = i*BlockSize;

		if (ResumeThread(ThreadHandles[0]) == (DWORD)-1)
		{
			std::cout << "Ошибка при возобновлении потока №" << ThreadNumber << '.' << std::endl;
			getch();
			return;
		};

		//Ждём завершения обработки какого-либо блока
		if (WaitForSingleObject(EventHandles[0], INFINITE) == WAIT_FAILED)
		{
			std::cout << "Ошибка при ожидании события потока №" << ThreadNumber << '.' << std::endl;
			getch();
			return;
		};

		//Сбрасываем сигнальное состояние события
		if (!ResetEvent(EventHandles[0]))
		{
			std::cout << "Ошибка при сбросе сигнального состояния события." << std::endl;
			getch();
			return;
		};

		std::cout << "Блок №" << i << " обработан." << std::endl;
	};

	//Общее время обработки
	Time = timeGetTime() - Time;
	std::cout << "Время обработки: " << Time << " мс." << std::endl;

	//Выводим промежуточные результаты в глобальные переменные
	Flush = true;

	if (ResumeThread(ThreadHandles[0]) == (DWORD)-1)
	{
		std::cout << "Ошибка при возобновлении потока №" << ThreadNumber << '.' << std::endl;
		getch();
		return;
	};

	//Ждём завершения обработки какого-либо блока
	if (WaitForSingleObject(EventHandles[0], INFINITE) == WAIT_FAILED)
	{
		std::cout << "Ошибка при ожидании события потока №" << ThreadNumber << '.' << std::endl;
		getch();
		return;
	};

	//Сбрасываем сигнальное состояние события
	if (!ResetEvent(EventHandles[0]))
	{
		std::cout << "Ошибка при сбросе сигнального состояния события." << std::endl;
		getch();
		return;
	};

	//Вычисленное значение суммы
	float pi = (float)Results[0] / N;

	//Полученное значение pi
	std::cout << "pi = " << pi << std::endl;

	getch();
}