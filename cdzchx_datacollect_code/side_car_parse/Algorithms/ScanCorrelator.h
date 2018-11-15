#ifndef ZCHX_ALGORITHMS_SCAN_CORRELATOR_H // -*- C++ -*-
#define ZCHX_ALGORITHMS_SCAN_CORRELATOR_H

#include "Messages/Extraction.h"
#include "Messages/Video.h"
#include <cmath>
#include <list>
#include <vector>
#include "Messages/RadarConfig.h"
#include <QObject>

using namespace ZCHX::Messages;
namespace ZCHX {
namespace Algorithms {

/**
   \ingroup Algorithms
*/
class ScanCorrelator:public QObject
{
    Q_OBJECT
public:
    // Algorithm interface
    //
    /** Constructor.

        \param controller object that controls us

        \param log device used for log messages
    */
    explicit ScanCorrelator(RadarConfig* cfg, QObject* parent = Q_NULLPTR);

    /** Register runtime parameters with the controller.

        \return true if successful, false otherwise
    */
    void startup();

    void reset();
    bool process(const Messages::Extractions::Ref& msg, Messages::Extractions::Ref& result);

private:
    void init();

    /**

       \param mgr object containing the encoded or native data to process

       \return true if successful, false otherwise
    */


    using time_t = long;
    using Extraction = ZCHX::Messages::Extraction;

    struct Data {
        time_t time;
        ZCHX::Messages::Extraction extraction;
    };

    using Entry = std::list<Data>;
    std::vector<std::vector<Entry>> buffer; // [x][y]
    size_t indexOffset;

    void corrCell(Data&, Entry& candidates);
    void corr(Extraction& ext, time_t t);

    double rMax;
    size_t numBins;

    double param_searchRadius;

    void on_searchRadius_changed(const double&);

    int param_scanTime;

    void on_scanTime_changed(const int&);

    int param_numScans;

    float searchRadius;  // the maximum gating size for correlation to occur
    float searchRadius2; // = searchRadius^2
    time_t t_new, t_old, t_veryold;
    RadarConfig * cfg_;
};

} // namespace Algorithms
} // namespace ZCHX

/** \file
 */

#endif
