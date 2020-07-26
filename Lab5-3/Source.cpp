#include <conio.h>
#include <Windows.h>
#include <mmsystem.h>
#include <iostream>

#pragma comment (lib, "Winmm.lib")

//N����. ������ = 530732
//������ ����� = 5307320
//����� ������ = 189
//������������ TLS

const unsigned int N = 1000000000;
const unsigned int BlockSize = 5307320;
const unsigned int BlockNumber = 189;
const unsigned int TLSCellCount = 1088;
const unsigned int ThreadCount = 1;

//������ �������� ����� ������, ��������� � ������ ��������� ������������
DWORD dwTlsIndex[ThreadCount] = { 0 };

//����������� �������
HANDLE ThreadHandles[ThreadCount];

//�������, ����������� ��� ���������������� �� ��������� �������� ������������
HANDLE EventHandles[ThreadCount];

//����, �� �������� ���������� ����� ����������� ����� ��������
bool Flush = false;

//������ ����������� ���������� ������� ������
unsigned int Results[ThreadCount] = { 0 };

//�������, ����������� �������
DWORD WINAPI BlockFunc(LPVOID lpParameter)
{
	//unsigned int Pointer = *(unsigned int*)lpParameter;
	while (true)
	{
		LPVOID lpValue = TlsGetValue(dwTlsIndex[0]);

		if (!lpValue && (GetLastError() != ERROR_SUCCESS))
		{
			std::cout << "������ ��� ������ ����������� TLS-������." << std::endl;
			getch();
			return NULL;
		};

		if (!Flush)
		{
			//����������� ������� �����
			float BlockSum = 0;

			for (unsigned int i = 0; i < BlockSize; i++)
			{
				float x = (i + *(unsigned int*)lpParameter + 0.5) / (float)N;
				BlockSum += 4. / (float)(1 + x*x);
			};

			if (!TlsSetValue(dwTlsIndex[0], (LPVOID)((unsigned int)BlockSum + (unsigned int)lpValue)))
			{
				std::cout << "������ ��� ��������� ����������� TLS-������." << std::endl;
				getch();
				return NULL;
			};
		}
		else
		{
			Results[0] = (unsigned int)lpValue;
			std::cout << "����������� ������� �����: " << Results[0] << std::endl;
		};

		//������������� � ���������� ��������
		if (!SetEvent(EventHandles[0]))
		{
			std::cout << "������ ��� ���������������� � ���������� ��������." << std::endl;
			getch();
			return NULL;
		};

		//���������������� �����
		if (SuspendThread(ThreadHandles[0]) == (DWORD)-1)
		{
			std::cout << "������ ��� ��������������� ������." << std::endl;
			getch();
			return NULL;
		};
	};
};

void main(void)
{
	setlocale(LC_ALL, "Russian");

	//������ ���� �����
	unsigned short ThreadNumber = 0;

	//������� � ������ ����� ������ ���� �������� - ����� ������ ���������� �����
	unsigned int Position = 0;

	//������ �������
	EventHandles[0] = CreateEvent(NULL, FALSE, FALSE, "Event for the first thread.");

	if (!EventHandles[0])
	{
		std::cout << "������ ��� �������� ������� ������ �" << ThreadNumber << '.' << std::endl;
		getch();
		return;
	};

	ThreadNumber++;
	if ((ThreadHandles[0] = CreateThread(NULL, NULL, BlockFunc, &Position, CREATE_SUSPENDED, NULL)) == NULL)
	{
		std::cout << "������ ��� �������� ������ �" << ThreadNumber << '.' << std::endl;
		getch();
		return;
	};

	//�������� TLS-������ ������ ��� ������� ������
	dwTlsIndex[0] = TlsAlloc();

	if (dwTlsIndex[0] == TLS_OUT_OF_INDEXES)
	{
		std::cout << "������ ��� ��������� TLS-������." << std::endl;
		getch();
		return;
	};

	//��������� �������� ��������
	unsigned long Time = timeGetTime();

	//���� �� ����� ������
	for (unsigned int i = 0; i < BlockNumber; i++, Position += BlockSize)
	{
		ThreadNumber = 1;

		//��� ������������� ������
		Position = i*BlockSize;

		if (ResumeThread(ThreadHandles[0]) == (DWORD)-1)
		{
			std::cout << "������ ��� ������������� ������ �" << ThreadNumber << '.' << std::endl;
			getch();
			return;
		};

		//��� ���������� ��������� ������-���� �����
		if (WaitForSingleObject(EventHandles[0], INFINITE) == WAIT_FAILED)
		{
			std::cout << "������ ��� �������� ������� ������ �" << ThreadNumber << '.' << std::endl;
			getch();
			return;
		};

		//���������� ���������� ��������� �������
		if (!ResetEvent(EventHandles[0]))
		{
			std::cout << "������ ��� ������ ����������� ��������� �������." << std::endl;
			getch();
			return;
		};

		std::cout << "���� �" << i << " ���������." << std::endl;
	};

	//����� ����� ���������
	Time = timeGetTime() - Time;
	std::cout << "����� ���������: " << Time << " ��." << std::endl;

	//������� ������������� ���������� � ���������� ����������
	Flush = true;

	if (ResumeThread(ThreadHandles[0]) == (DWORD)-1)
	{
		std::cout << "������ ��� ������������� ������ �" << ThreadNumber << '.' << std::endl;
		getch();
		return;
	};

	//��� ���������� ��������� ������-���� �����
	if (WaitForSingleObject(EventHandles[0], INFINITE) == WAIT_FAILED)
	{
		std::cout << "������ ��� �������� ������� ������ �" << ThreadNumber << '.' << std::endl;
		getch();
		return;
	};

	//���������� ���������� ��������� �������
	if (!ResetEvent(EventHandles[0]))
	{
		std::cout << "������ ��� ������ ����������� ��������� �������." << std::endl;
		getch();
		return;
	};

	//����������� �������� �����
	float pi = (float)Results[0] / N;

	//���������� �������� pi
	std::cout << "pi = " << pi << std::endl;

	getch();
}