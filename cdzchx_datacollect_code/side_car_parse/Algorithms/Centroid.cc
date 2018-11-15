#include "Centroid.h"

//#define CENTROID_DEBUG

#ifdef CENTROID_DEBUG
#include <fstream>
#endif

Centroid::Centroid(BinaryTargetImagePtr mask, VideoTargetImagePtr video) :
    m_mask(mask), m_video(video), m_useOriginalTarget(false)
{
}

void
Centroid::Process()
{
    // calculate the x and y gradients (first derivatives)
    VideoTargetImagePtr x_grad;
    VideoTargetImagePtr y_grad;
    m_video->MaskGradient(m_mask, x_grad, y_grad);

    // determine the positive-to-negative slope transitions that define
    //  the peaks in the x and y directions
    BinaryTargetImagePtr peaks_x = x_grad->IdentifyPeaksX();
    BinaryTargetImagePtr peaks_y = y_grad->IdentifyPeaksY();

    // the places where peaks occur in both x and y simultaneously are
    //  the target centers
    BinaryTargetImagePtr targetPeaks = (*peaks_x) && (*peaks_y);

    // the peaks may be more than one pixel (e.g., as a result of saturation)
    //  so blob detect (again) to find the center of the peak Targets
    int rows = targetPeaks->GetRows();
    int cols = targetPeaks->GetCols();
    BINARYDATA* data = targetPeaks->GetDataRef();
    AzimuthDataPtr az = m_mask->GetAzimuthData();

    // process each line of the detected target peaks
    for (int r = 0; r < rows; r++) { m_is.AppendScanLine(BinaryScanLineArray(data + r * cols, cols), (*az)[r]); }

    // close any Targets that may still be incomplete
    m_is.CloseAllOpenTargets();

    // use the original target if no targets were found (i.e., there were no
    //  local maximum in the data)
    m_useOriginalTarget = IsClosedTargetsEmpty();

#ifdef CENTROID_DEBUG
    ofstream out;
    char filename[256];
    static int targetCount = 0;
    targetCount++;

    sprintf(filename, "target%i_video.csv", targetCount);
    out.open(filename, ios::trunc | ios::binary);
    if (!out) {
        cout << "unable to open file " << filename << endl;
        abort();
    }
    m_video->DumpCSV(out);
    out.close();

    sprintf(filename, "target%i_mask.csv", targetCount);
    out.open(filename, ios::trunc | ios::binary);
    if (!out) {
        cout << "unable to open file " << filename << endl;
        abort();
    }
    m_mask->DumpCSV(out);
    out.close();

    sprintf(filename, "target%i_xgrad.csv", targetCount);
    out.open(filename, ios::trunc | ios::binary);
    if (!out) {
        cout << "unable to open file " << filename << endl;
        abort();
    }
    x_grad->DumpCSV(out);
    out.close();

    sprintf(filename, "target%i_ygrad.csv", targetCount);
    out.open(filename, ios::trunc | ios::binary);
    if (!out) {
        cout << "unable to open file " << filename << endl;
        abort();
    }
    y_grad->DumpCSV(out);
    out.close();

    sprintf(filename, "target%i_xpeaks.csv", targetCount);
    out.open(filename, ios::trunc | ios::binary);
    if (!out) {
        cout << "unable to open file " << filename << endl;
        abort();
    }
    peaks_x->DumpCSV(out);
    out.close();

    sprintf(filename, "target%i_ypeaks.csv", targetCount);
    out.open(filename, ios::trunc | ios::binary);
    if (!out) {
        cout << "unable to open file " << filename << endl;
        abort();
    }
    peaks_y->DumpCSV(out);
    out.close();

    sprintf(filename, "target%i_targetPeaks.csv", targetCount);
    out.open(filename, ios::trunc | ios::binary);
    if (!out) {
        cout << "unable to open file " << filename << endl;
        abort();
    }
    targetPeaks->DumpCSV(out);
    out.close();

    sprintf(filename, "target%i_subtargs.csv", targetCount);
    out.open(filename, ios::trunc | ios::binary);
    if (!out) {
        cout << "unable to open file " << filename << endl;
        abort();
    }
    while (!IsClosedTargetsEmpty()) {
        TargetPosition tppos = PopClosedTargetPosition();
        PRI_COUNT p = 0;
        for (int a = 0; a < m_mask->m_az->size(); a++)
            if ((*(m_mask->m_az))[a] == tppos.az) {
                p = a;
                break;
            }
        out << tppos.range - m_mask->GetSize().minRange << "," << p << std::endl;
    }
    out.close();

    if (targetCount > 10) {
        cout << "exiting because CENTROID_DEBUG is defined" << endl;
        abort();
    }
#endif
}

bool
Centroid::IsClosedTargetsEmpty()
{
    if (m_useOriginalTarget)
        return false;
    else
        return m_is.IsClosedTargetsEmpty();
}

TargetPosition
Centroid::PopClosedTargetPosition()
{
    if (m_useOriginalTarget) {
        m_useOriginalTarget = false;
        return m_mask->GetSize().Center();
    } else {
        SegmentedTargetImagePtr peakOfTarget = m_is.PopClosedTarget();
        TargetPosition pos = peakOfTarget->GetSize().Center();
        pos.range += m_mask->GetSize().minRange;
        return pos;
    }
}
