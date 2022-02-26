#ifdef AVRGR
//������� 3
//��� ������� � ����� ��������� ������� �������� �������.
// �� ������ ������� ��� �� �������� ����� ����������� �� GitHub.
// �� �������� ��������� ������ �������.

//������ ��������� ������� ������������ ����������� ���� ���������.
// ������ � ��� ����� ������.
// ��������� � ������������� ������ string �� string_view ���, ��� ��� ��������, � ��� ����� �������� ���������.

//��������� ������ ������ ������ ��������� ������� string_view ������ ������:
//- �����������;
//- AddDocument;
//- FindTopDocuments;
//- MatchDocument.
//��� ������ ������ ���������� string_view ������ �����:
//- MatchDocument;
//- GetWordFrequencies.

//�����������
//� ����� MatchDocument ����� ���� ������� ������������ ��������� ������ � � ���� ������ ������ ���� ��������� ���������� std::invalid_argument.
//���� ������� �������������� document_id, ��������� ���������� std::out_of_range.

//��� ���������� �� ��������
//������������ ����� � ����� � �����������, ���������� ����� SearchServer � ���������� ����� ��������������� �������.
//������� main ����� ���������������.

//��� ����� ������������� ��� ���
//������������ ������ ������ ����� ��������� ����-�������.
//������������������ ���� ������ ������ ����� ��������� � ����� �������� ��������.
// ���� ��������� ������� � ����������������:
//- 10 000 ����������, �� ����� 70 ���� � ������;
//- �� ����� 500 ���� � ��������� �������, ������� �����-�����;
//- ��� ����� � �� �������, ���������� �� 1 000 ���� ������ �� ����� 10 ����.

//��� ��������� ������, ��������� �� �� ����� 500 ����, ������� �����-�����.
//��������������� ��� ���� ���������� ���������� MatchDocument � ���� ��������.
// ���������� ����� ����� ������ ����� �������.

//����� ������ ����� ������������� ������ ������ ���� �� ������� ���� ����� ������, ��� ����� ������ ����� ������������ ������ � ����� ��������� ������.
//����� ������ ����� ������������ ������ �� ������ ��������� ����� ������ ������������ ������ ���������� ������� ������, ��� � ������� ����.
//����� ������ ����� ������������� ������ �� ������ ��������� ����� ������ ������������� ������ ���������� ������� ������, ��� � ������� ����.

//���������
//�������: string_view �� ������� �������.
// ������� ���, ��� �����, ����������� string ��� ���������� �� ����� ���������� � ����� ������.
//�� ������ ������ string_view �� ��������� ����� set<string> ��� �������� ��������� ������,
//�� ��� ����� ��� ����� ���� � ����� ���� ��������� ������� ����������� ����������: set<string, less<>>.
#endif // AVRGR

#include "search_server.h"
#include "request_queue.h"
#include "process_queries.h"
#include "paginator.h"
#include "log_duration.h"
#include "remove_duplicates.h"
#include "utility.h"

#include <iostream>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include <execution>
#include <random>

#include "task_RemoveDuplicates.h"
#include "task_ProcessQueries.h"
#include "benchmark_ProcessQueries.h"
#include "task_ProcessQueriesJoined.h"
#include "task_1_of_3_RemoveDocument.h"
#include "task_2_of_3_MatchDocument.h"
#include "benchmark_MatchDocument.h"



using namespace std::literals::string_literals;
using namespace std;

int main() {

	benchmark_MatchDocument();
	task_2_of_3_MatchDocument();

	//task_1_of_3_RemoveDocument();

	//task_ProcessQueriesJoined();

	//benchmark_ProcessQueries();
	//task_ProcessQueries();

	//task_RemoveDuplicates();
}
