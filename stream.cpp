#include "stream.h"
#include <QDebug>
#include <QApplication>


Stream::Stream(QString svID, QString sourceMAC, QObject *parent) : QObject(parent)
{
    this->alive = true;
    this->checkAlive = false;
    this->analysed = false;
    this->svID = svID;
    this->sourceMAC = sourceMAC;
    this->sampleRate = SampleRate();

    connect(&watcher, SIGNAL(finished()), this, SLOT(handleAnalysisFinished()));
    connect(this, SIGNAL(stopWatcher()), &watcher, SLOT(cancel()));
}

Stream::~Stream()
{
    qDebug() << "destroying Stream";

    //emit removeView();

    //future.cancel();
    //watcher.cancel();

    //future.cancel();
    if (future.isRunning()) {
        future.cancel();
    }

    if (watcher.isRunning()) {
        emit stopWatcher();
    }

    if (timer != NULL && timer->isActive()) {
        //timer->stop();
        //timer->disconnect();
        emit stopTimer();
    }
}

void Stream::addSample(LE_IED_MUnn_PhsMeas1 *dataset, quint16 smpCnt)
{
    if (smpCnt >= 0 && smpCnt < MAX_SAMPLES) {
        checkAlive = true;
        //qDebug() << smpCnt  << capturedSamples << samplesPerSecond;

        samples[smpCnt].voltage[0] = dataset->MUnn_TVTR_1_Vol_instMag.i;
        samples[smpCnt].voltageQuality[0] = dataset->MUnn_TVTR_1_Vol_q;
        samples[smpCnt].voltage[1] = dataset->MUnn_TVTR_2_Vol_instMag.i;
        samples[smpCnt].voltageQuality[1] = dataset->MUnn_TVTR_2_Vol_q;
        samples[smpCnt].voltage[2] = dataset->MUnn_TVTR_3_Vol_instMag.i;
        samples[smpCnt].voltageQuality[2] = dataset->MUnn_TVTR_3_Vol_q;
        samples[smpCnt].voltage[3] = dataset->MUnn_TVTR_4_Vol_instMag.i;
        samples[smpCnt].voltageQuality[3] = dataset->MUnn_TVTR_4_Vol_q;

        samples[smpCnt].current[0] = dataset->MUnn_TCTR_1_Amp_instMag.i;
        samples[smpCnt].currentQuality[0] = dataset->MUnn_TCTR_1_Amp_q;
        samples[smpCnt].current[1] = dataset->MUnn_TCTR_2_Amp_instMag.i;
        samples[smpCnt].currentQuality[1] = dataset->MUnn_TCTR_2_Amp_q;
        samples[smpCnt].current[2] = dataset->MUnn_TCTR_3_Amp_instMag.i;
        samples[smpCnt].currentQuality[2] = dataset->MUnn_TCTR_3_Amp_q;
        samples[smpCnt].current[3] = dataset->MUnn_TCTR_4_Amp_instMag.i;
        samples[smpCnt].currentQuality[3] = dataset->MUnn_TCTR_4_Amp_q;

        // determine sampling rate and nominal frequency
        if (smpCnt == 0 && !sampleRate.isKnown()) {
            if (capturedSamples == SAMPLES_50HZ_80_PER_CYCLE) {
                sampleRate.setSampleRate(SampleRate::NominalFreq50Hz, SampleRate::Samples80);
                analysisInstance.setBlockParameters(&measure_P_50Hz_80_samples_per_cycle);
            }
            else if (capturedSamples == SAMPLES_60HZ_80_PER_CYCLE) {
                sampleRate.setSampleRate(SampleRate::NominalFreq60Hz, SampleRate::Samples80);
                analysisInstance.setBlockParameters(&measure_P_60Hz_80_samples_per_cycle);
            }
            else if (capturedSamples == SAMPLES_50HZ_256_PER_CYCLE) {
                sampleRate.setSampleRate(SampleRate::NominalFreq50Hz, SampleRate::Samples256);
                analysisInstance.setBlockParameters(&measure_P_50Hz_256_samples_per_cycle);
            }
            else if (capturedSamples == SAMPLES_60HZ_256_PER_CYCLE) {
                sampleRate.setSampleRate(SampleRate::NominalFreq60Hz, SampleRate::Samples256);
                analysisInstance.setBlockParameters(&measure_P_60Hz_256_samples_per_cycle);
            }

            if (sampleRate.isKnown()) {
                samplesPerSecond = sampleRate.getSamplesPerSecond();

                if (disabled == false) {
                    updateStreamTableModel();

                    //emit updateModel(true);
                    emit scheduleAnalysis();  // TODO: do analysis scheduling in StreamManager? Or, set a flag to: block UI updates; wait for all timers to stop, then block them; delete itself later.
                }
            }

            // TODO: detect invalid sample rate values, and count valid packets recv'd?
        }

        if (smpCnt == 0) {
            capturedSamples = 1;
        }
        else {
            capturedSamples++;
        }
    }
}

void Stream::updateStreamTableModel() {
    StreamTableRow *row = new StreamTableRow(this);

    row->moveToThread(QApplication::instance()->thread());  // move to UI thread

    emit setStreamTableRow(row);
}

QString Stream::getSvID()
{
    return this->svID;
}

QString Stream::getSourceMAC()
{
    return this->sourceMAC;
}

//QString Stream::getFreq()
//{
//    if (analysed) {
//        return QString("%1 Hz").arg(analysisInstance.measure_Y.Frequency, 0, 'g', SIGNIFICANT_DIGITS_DIPLAYED);
//    }
//    else {
//        return QString("--");
//    }
//}

//QString Stream::getVoltage()
//{
//    if (analysed) {
//        return QString("%1 kV").arg(sqrt(3) * (analysisInstance.measure_Y.Voltage[0] + analysisInstance.measure_Y.Voltage[1] + analysisInstance.measure_Y.Voltage[2]) / 3000.0, 0, 'g', SIGNIFICANT_DIGITS_DIPLAYED);
//    }
//    else {
//        return QString("--");
//    }
//}

//QString Stream::getPower()
//{
//    if (analysed) {
//        return QString("%1 kVA, p.f. %2").arg(analysisInstance.measure_Y.Power[0] / 1000.0, 0, 'g', SIGNIFICANT_DIGITS_DIPLAYED).arg(analysisInstance.measure_Y.Power[3], 0, 'f', SIGNIFICANT_DIGITS_DIPLAYED);
//    }
//    else {
//        return QString("--");
//    }
//}

//QString Stream::getCurrent()
//{
//    if (analysed) {
//        return QString("%1 kA").arg(sqrt(3) * (analysisInstance.measure_Y.Current[0] + analysisInstance.measure_Y.Current[1] + analysisInstance.measure_Y.Current[2]) / 3000.0, 0, 'g', SIGNIFICANT_DIGITS_DIPLAYED);
//    }
//    else {
//        return QString("--");
//    }
//}

//QString Stream::getSamplesPerCycle()
//{
//    if (sampleRate.isKnown()) {
//        return QString("%1").arg(sampleRate.getSamplesPerCycle());
//    }
//    else {
//        return QString("--");
//    }
//}

// TODO: put this processing in analysis thread?
QPainterPath *Stream::getPainterPath(QPainterPath *path, PowerSystemQuantity powerSystemQuantity, int phase)
{
    quint32 iterations = sampleRate.getSamplesPerCycle() * NUMBER_OF_CYCLES_TO_ANALYSE;
    qreal Ts = sampleRate.getTimestep();

    for (quint32 t = 0; t < iterations; t++) {
        qreal tMilliseconds = (qreal) t * Ts * 1000.0;

        if (powerSystemQuantity == Stream::Current) {
            if (t == 0) {
                path->setElementPositionAt(t, tMilliseconds, (qreal) -1.0 *samples[t].current[phase] * LE_IED.S1.MUnn.IEC_61850_9_2LETCTR_1.Amp.sVC.scaleFactor);
            }
            else {
                path->setElementPositionAt(t, tMilliseconds, (qreal) -1.0 *samples[t].current[phase] * LE_IED.S1.MUnn.IEC_61850_9_2LETCTR_1.Amp.sVC.scaleFactor);
            }
        }
        else if (powerSystemQuantity == Stream::Voltage) {
            if (t == 0) {
                path->setElementPositionAt(t, tMilliseconds, (qreal) -1.0 *samples[t].voltage[phase] * LE_IED.S1.MUnn.IEC_61850_9_2LETVTR_1.Vol.sVC.scaleFactor);
            }
            else {
                path->setElementPositionAt(t, tMilliseconds, (qreal) -1.0 *samples[t].voltage[phase] * LE_IED.S1.MUnn.IEC_61850_9_2LETVTR_1.Vol.sVC.scaleFactor);
            }
        }

        //qDebug() << (qreal) t * Ts << (qreal) samples[t].current[phase] * 0.01;
    }

    return path;
}

quint32 Stream::getNumberOfSamplesCaptured()
{
    return this->capturedSamples;
}

bool Stream::isAnalysed()
{
    return analysed;
}

bool Stream::isAlive()
{
    return this->alive;
}

void Stream::setAnalysed(bool analysed)
{
    this->analysed = analysed;
}

ExternalOutputs_measure *Stream::getStreamData()
{
    return &analysisInstance.measure_Y;
}

qreal Stream::getMaxInstantaneous(Stream::PowerSystemQuantity powerSystemQuantity)
{
    if (powerSystemQuantity == Stream::Voltage) {
        return maxInstantaneousVoltage;
    }
    else if (powerSystemQuantity == Stream::Current) {
        return maxInstantaneousCurrent;
    }
    return 0.0;
}

SampleRate *Stream::getSampleRate()
{
    return &sampleRate;
}

void Stream::handleAnalysisFinished()
{
    //qDebug() << "done analysis";

    //emit updateModel(false);         //TODO: updates still twitchy, need more granular update?

    if (disabled == false) {
        updateStreamTableModel();

        if (timer == NULL) {
            timer = new QTimer(this);
            connect(timer, SIGNAL(timeout()), this, SLOT(timeout()));
            connect(this, SIGNAL(stopTimer()), timer, SLOT(stop()));
            timer->start(RECALCULATE_ANALYSIS_TIME);
        }
    }

    //emit updateView();  // TODO: Safe to put this here? Could be race condition?

    //QTimer::singleShot(RECALCULATE_ANALYSIS_TIME, this, SLOT(scheduleAnalysis()));
}

void Stream::scheduleAnalysis() {
    //qDebug() << "in scheduleAnalysis";

    // TODO: better checking of watcher/future state
    if (!watcher.isRunning() && sampleRate.isKnown()) {
        future = QtConcurrent::run(this, &Stream::analyse);     // TODO: can only use this (and timers) if using exec(); explicitly create other thread?
        watcher.setFuture(future);
    }

    // TODO: invalidate samples per cycle info here too?
}

void Stream::timeout()
{
    //qDebug() << "in timeout";
    if (disabled == false) {
        bool prevAlive = alive;

        if (checkAlive == false) {
            alive = false;
        }
        else {
            alive = true;
            scheduleAnalysis();
        }

        if (alive != prevAlive) {
            //emit updateModel(false);    // TODO: only update appropriate cell?
            updateStreamTableModel();
        }

        checkAlive = false;
    }
    else {
        emit stopTimer();
        emit deleteLater();
    }
}

void Stream::analyse()
{
    //qDebug() << "in analysis";
    //QElapsedTimer timer;
    //timer.start();

    setAnalysed(false);

    analysisInstance.initialize();
    quint32 iterations = sampleRate.getSamplesPerCycle() * NUMBER_OF_CYCLES_TO_ANALYSE;

    maxInstantaneousVoltage = 0.0;
    maxInstantaneousCurrent = 0.0;

    //TODO: why the need to ignore the first sample?
    for (quint32 t = 1; t < iterations; t++) {
        analysisInstance.measure_U.Vabcpu[0] = samples[t].voltage[0] * LE_IED.S1.MUnn.IEC_61850_9_2LETVTR_1.Vol.sVC.scaleFactor;
        analysisInstance.measure_U.Vabcpu[1] = samples[t].voltage[1] * LE_IED.S1.MUnn.IEC_61850_9_2LETVTR_2.Vol.sVC.scaleFactor;
        analysisInstance.measure_U.Vabcpu[2] = samples[t].voltage[2] * LE_IED.S1.MUnn.IEC_61850_9_2LETVTR_3.Vol.sVC.scaleFactor;
        analysisInstance.measure_U.IabcAmps[0] = samples[t].current[0] * LE_IED.S1.MUnn.IEC_61850_9_2LETCTR_1.Amp.sVC.scaleFactor;
        analysisInstance.measure_U.IabcAmps[1] = samples[t].current[1] * LE_IED.S1.MUnn.IEC_61850_9_2LETCTR_2.Amp.sVC.scaleFactor;
        analysisInstance.measure_U.IabcAmps[2] = samples[t].current[2] * LE_IED.S1.MUnn.IEC_61850_9_2LETCTR_3.Amp.sVC.scaleFactor;

        qreal timestepMaxVoltage = qMax(qAbs(analysisInstance.measure_U.Vabcpu[0]), qMax(qAbs(analysisInstance.measure_U.Vabcpu[1]), qAbs(analysisInstance.measure_U.Vabcpu[2])));
        if (timestepMaxVoltage > maxInstantaneousVoltage) {
            maxInstantaneousVoltage = timestepMaxVoltage;
        }
        qreal timestepMaxCurrent = qMax(qAbs(analysisInstance.measure_U.IabcAmps[0]), qMax(qAbs(analysisInstance.measure_U.IabcAmps[1]), qAbs(analysisInstance.measure_U.IabcAmps[2])));
        if (timestepMaxCurrent > maxInstantaneousCurrent) {
            maxInstantaneousCurrent = timestepMaxCurrent;
        }

        analysisInstance.step();
    }

    setAnalysed(true);

    //qDebug() << "The analysis took" << timer.elapsed() << "milliseconds";
}
