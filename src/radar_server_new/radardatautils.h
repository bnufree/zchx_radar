#ifndef RADARDATAUTILS_H
#define RADARDATAUTILS_H

#include <QString>

enum RadarObjType{
    Obj_Normal = 0,      //未定义
    Obj_Barrier = 1,       //障碍物
    Obj_Buoy,              //浮标
    Obj_FishRaft,      //鱼排
    Obj_SpecialShip,      //监控船
    Obj_Other,
};


struct UserSpecifiedObj
{
    int     mTrackNum;
    int     mType;
    QString mName;
};



#endif // RADARDATAUTILS_H
