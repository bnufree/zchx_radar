#ifndef ZCHXFILTERAREAFILE_H
#define ZCHXFILTERAREAFILE_H

#include "zchxmsgcommon.h"

class zchxFilterAreaFile
{
public:
    explicit zchxFilterAreaFile(const QString& fileName);
    bool    removeArea(qint64 id);
    bool    addArea(const zchxMsg::filterArea& area);
    QList<zchxMsg::filterArea>  getFilterAreaList() const;
    QByteArray getFilterAreaByteArray() const;


private:
    void    init();
    bool    updateFile();

private:
    QString mFileName;
    QMap<qint64, zchxMsg::filterArea> mAreaMap;
};

#endif // ZCHXFILTERAREAFILE_H
