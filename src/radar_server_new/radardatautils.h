#ifndef RADARDATAUTILS_H
#define RADARDATAUTILS_H

#include <QString>

enum RadarObjType{
    RadarPointUndef = 0,        //未知
    RadarPointNormal,
    RadarPointBarrier,          //障碍物
    RadarPointBuyo,             //浮标
    RadarPointFishRaft,         //渔排
    RadarPointSpecialShip,      //特定船舶
    RadarPointPerson,           //人
    RadarPointCar,              //车
    RadarPointShip,             //一般船舶
};


struct UserSpecifiedObj
{
    int     mTrackNum;
    int     mType;
    QString mName;
};



#endif // RADARDATAUTILS_H
