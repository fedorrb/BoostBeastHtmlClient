#define Visible 1
#define NoVisible 0

short SetItemText(StayWindow * Wnd,WORD ID, char* Text);

short SetWndText(StayWindow * Wnd, char* Text);

// ��������������� ���������� ����
//Wnd - ������������� ����  ID - ��-�� ����������
short ItemShow(StayWindow * Wnd,WORD ID);
//���� = 1 - ���������� 0 - ������
short ItemVisible(StayWindow * Wnd,WORD ID, short Flag);
//e��������� ������
short SetItemFocus(StayWindow * Wnd,WORD ID);


BYTE	Find_Context(StayDataset *Base, StayEvent s, StayFD* FindName,char *Str);