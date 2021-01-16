#ifndef NORMALIZE_H
#define NORMALIZE_H
#include <cstdlib>
#include <string>
#include <list>
#include <fstream>
#include <iostream>
#include <sstream>
#include <locale>
//#include <codecvt>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;
/*
����� ��� �������������� ������ �������� �����
�������� �� ������ ������ ������ ����� �����
*/

class Normalize
{
public:
	Normalize(void);
	void SetFileName(string fileName);
	void CreateNewFileName(string fileName = "");
	bool LoadFile();
	void SaveFile();
	void SplitStringLine(std::string& str, std::vector<std::string> & cont); //������� �������� ����� ������ �� ��������� �� �����������
	void ModifyLengthCol(std::vector<std::string> & cont); //�������������� �������
public:
	~Normalize(void);
private:
	string fileName;
	std::vector<std::string> splitContents; //������ � ������� ����������� �������� ���� � ����������� �� ������
	void GetMasLengthCurStr(std::string str, std::vector<int> &strLengthMas); //��������� ������ �������
	void MergeMas(std::vector<int> &blockLengthMas, std::vector<int> &strLengthMas); //�� ��������� ������ ������� � ������� �������� ����������
	void ModifyLengthBlockCol(std::vector<std::string> &cont, std::vector<int> &blockLengthMas, int cur, int amount); //�������� ������ �������
};
#endif