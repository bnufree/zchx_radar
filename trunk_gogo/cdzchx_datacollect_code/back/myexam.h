#ifndef MYEXAM_H
#define MYEXAM_H
#include <string>
#include <vector>
using namespace std;
struct cmdItem
{
    string label;
    string type;
    string dummy;
    string tclCmd;
};

int strCmpForNumber(const char* str1,const char* str2);

bool analysisCmdItemFromFile(const char* fileName, int startLineIndex, vector<cmdItem> &cmdItemVec);

class AdeSetting
{
public:
    AdeSetting (const char *libName,const char *cellName); // 普通构造函数
    AdeSetting (const AdeSetting &other); // 拷贝构造函数
    ~AdeSetting (void); // 析构函数
    AdeSetting& operator =(const AdeSetting &other); // 赋值函数
private:
    char *m_libName; // 用于保存lib name
    char *m_cellName; //save cell name
};

#endif // MYEXAM_H
