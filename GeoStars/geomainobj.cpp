#include "geoStars.h"

GeoStarsFunc* GeoStarsFunc::m_instance = 0;

GeoStarsFunc::~GeoStarsFunc()
{
    if(m_instance)
    {
        delete m_instance;
        m_instance = NULL;
    }
}

GeoStarsFunc::GeoStarsFunc()
{

}

GeoStarsFunc* GeoStarsFunc::instance()
{
    if(!m_instance) m_instance = new GeoStarsFunc;
    return m_instance;
}
