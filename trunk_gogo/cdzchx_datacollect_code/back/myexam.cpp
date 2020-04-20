#include "myexam.h"
#include <string.h>
#include <stdlib.h>
#include <QDebug>

AdeSetting::AdeSetting(const char *libName, const char *cellName)
{
    //m_libName
    if(libName == NULL)
    {
        m_libName = '\0';
    }
    else
    {
        int uLibNum = strlen(libName);
        if(m_libName)
        {
            delete []m_libName;
            m_libName = NULL;
        }
        m_libName = new char[uLibNum+1];
        strcpy(m_libName,libName);
    }
    //m_cellName
    if(cellName == NULL)
    {
        m_cellName = '\0';
    }
    else
    {
        int uCellNum = strlen(cellName);
        if(m_cellName)
        {
            delete []m_cellName;
            m_cellName = NULL;
        }
        m_cellName = new char[uCellNum+1];
        strcpy(m_cellName,cellName);
    }
}

AdeSetting::AdeSetting(const AdeSetting &other)
{
    //m_libName
    int uLibNum = strlen(other.m_libName);
    if(m_libName)
    {
        delete []m_libName;
        m_libName = NULL;
    }
    m_libName = new char[uLibNum+1];
    strcpy(m_libName,other.m_libName);
    //m_cellName
    int uCellNum = strlen(other.m_cellName);
    if(m_cellName)
    {
        delete []m_cellName;
        m_cellName = NULL;
    }
    m_cellName = new char[uCellNum+1];
    strcpy(m_cellName,other.m_cellName);
}

AdeSetting::~AdeSetting()
{
    //m_libName
    if(m_libName)
    {
        delete []m_libName;
        m_libName = NULL;
    }
    //m_cellName
    if(m_cellName)
    {
        delete []m_cellName;
        m_cellName = NULL;
    }
}

AdeSetting &AdeSetting::operator =(const AdeSetting &other)
{
    if(this == &other)
    {
        return *this;
    }
    //m_libName
    if(m_libName)
    {
        delete []m_libName;
        m_libName = NULL;
    }
    int uLibNum = strlen(other.m_libName);
    m_libName = new char[uLibNum+1];
    strcpy(m_libName,other.m_libName);

    //m_cellName
    if(m_cellName)
    {
        delete []m_cellName;
        m_cellName = NULL;
    }
    int uCellNum = strlen(other.m_cellName);
    m_cellName = new char[uCellNum+1];
    strcpy(m_cellName,other.m_cellName);
    //返回
    return *this;
}

//返回值为0两字符串相等，1表明str1>str2，2表明str1<str2.(字符串中只有数字和字母)
int strCmpForNumber(const char *str1,const char *str2)
{
    if(str1 == NULL&&str2 == NULL)
    {
        return 0;
    }
    else if(str1 != NULL&&str2 == NULL)
    {
        return 1;
    }
    else if(str1 == NULL&&str2 != NULL)
    {
        return 2;
    }
    else
    {

        while(1)
        {
            char cell1 = *str1++;
            char cell2 = *str2++;
            if(cell1 == '\0'&&cell2 == '\0')//比较完毕，相等
            {
                return 0;
            }
            else if(cell1 != '\0'&&cell2 == '\0')//str1比str2长
            {
                return 1;
            }
            else if(cell1 == '\0'&&cell2 != '\0')//str1比str2短
            {
                return 2;
            }
            else
            {
                if(cell1>='A'&&cell1<='Z')//将大写字母转为小写字母
                {
                    cell1 += ('a'-'A');
                }
                if(cell2>='A'&&cell2<='Z')//将大写字母转为小写字母
                {
                    cell2 += ('a'-'A');
                }
                if((cell1>='a'&&cell1<='z')&&(cell2>='0'&&cell2<='9'))//cell1是字母，cell2是数字
                {
                    return 1;
                }
                if((cell1>='0'&&cell1<='9')&&(cell2>='a'&&cell2<='z'))//cell1是数字，cell2是字母
                {
                    return 2;
                }
                if((cell1>='a'&&cell1<='z')&&(cell2>='a'&&cell2<='z'))//都是字母
                {
                    if(cell1>cell2)
                    {
                        return 1;
                    }
                    else if(cell1<cell2)
                    {
                        return 2;
                    }
                    else
                    {
                        continue;
                    }
                }
                if((cell1>='0'&&cell1<='9')&&(cell2>='0'&&cell2<='9'))//都是数字
                {
                    //str1的连续数字
                    int sum1 = (cell1 - '0');
                    char nextCell1 = *str1++;
                    while(nextCell1>='0'&&nextCell1<='9')
                    {
                        sum1 =sum1*10+(nextCell1  - '0');
                        nextCell1 = *str1++;
                    }
                    if(!(nextCell1>='0'&&nextCell1<='9'))//如果不是数字字符串指针往前移动
                    {
                        str1--;
                    }
                    //str2的连续数字
                    int sum2 = (cell2  - '0');
                    char nextCell2 = *str2++;
                    while(nextCell2>='0'&&nextCell2<='9')
                    {
                        sum2 =sum2*10+(nextCell2  - '0');
                        nextCell2 = *str2++;
                    }
                    if(!(nextCell2>='0'&&nextCell2<='9'))//如果不是数字字符串指针往前移动
                    {
                        str2--;
                    }
                    //进行比较数字大小
                    if(sum1>sum2)
                    {
                        return 1;
                    }
                    else if(sum1<sum2)
                    {
                        return 2;
                    }
                    else
                    {
                        continue;
                    }
                }
            }
        }
    }
}

//清除字符串前面的空格
void dumpFrontSpace( char *&str)
{
    if(str == NULL)
    {
        return;
    }
    while(1)//去掉字符串前面的空格
    {
        char cell = *str++;
        if(cell == ' ')
        {
            continue;
        }
        else
        {
            str--;//清除空格后指针往前移动
            return;
        }
    }
}
string getContent(char *&str)
{
    string obj;
    if(str == NULL)
    {
        return obj;
    }
    dumpFrontSpace(str);
    bool bContainMarks = false;//是否有双引号
    char cell = *str++;
    while(cell != '\n')
    {
        if((cell == '"')&&(!bContainMarks))//第一个双引号
        {
            bContainMarks = true;
            cell = *str++;
            continue;
        }
        if((cell == '"')&&(bContainMarks))//第二个双引号返回
        {
            return obj;
        }
        if((cell == ' ')&&(!bContainMarks))//没有双引号遇到空格返回
        {
            return obj;
        }
        obj.push_back(cell);
        cell = *str++;
    }
    return obj;
}

bool analysisCmdItem(char *str,cmdItem &objItem)
{
    if(str == NULL)
    {
        return false;
    }
    char *temp = str;
    string label = getContent(temp);
    string type = getContent(temp);
    string dummy = getContent(temp);
    string tclCmd = getContent(temp);
    objItem.label = label;
    objItem.type = type;
    objItem.dummy = dummy;
    objItem.tclCmd = tclCmd;

    return true;
}

bool analysisCmdItemFromFile(const char *fileName, int startLineIndex, vector<cmdItem> &cmdItemVec)
{
    FILE *pFile;
    pFile = fopen(fileName,"r");
    if(pFile == NULL)
    {
        return false;
    }
    cmdItemVec.clear();
    char str[100];
    int uIndex = 1;
    cmdItem objItem;
    while(!feof(pFile))
    {

        fgets(str, sizeof(str), pFile);
        if(uIndex++<startLineIndex)
        {
            continue;
        }

        if(str)
        {
            if(analysisCmdItem(str,objItem))
            {
                cmdItemVec.push_back(objItem);
            }
        }

    }

    fclose(pFile); // 操作完毕后关闭文件
    return true;
}
