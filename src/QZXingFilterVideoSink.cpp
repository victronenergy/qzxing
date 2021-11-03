#include "QZXingFilterVideoSink.h"
#include <QDebug>
#include <QtConcurrent/QtConcurrent>
#include "QZXingImageProvider.h"

QZXingFilter::QZXingFilter(QObject *parent)
    : QObject(parent)
    , decoder(QZXing::DecoderFormat_QR_CODE)
    , decoding(false)
{
    /// Connecting signals to handlers that will send signals to QML
    connect(&decoder, &QZXing::decodingStarted,
            this, &QZXingFilter::handleDecodingStarted);
    connect(&decoder, &QZXing::decodingFinished,
            this, &QZXingFilter::handleDecodingFinished);
}

QZXingFilter::~QZXingFilter()
{
    if(!processThread.isFinished()) {
        processThread.cancel();
        processThread.waitForFinished();
    }
}

void QZXingFilter::handleDecodingStarted()
{
    decoding = true;
    emit decodingStarted();
    emit isDecodingChanged();
}

void QZXingFilter::handleDecodingFinished(bool succeeded)
{
    decoding = false;
    emit decodingFinished(succeeded, decoder.getProcessTimeOfLastDecoding());
    emit isDecodingChanged();
}

void QZXingFilter::setVideoSink(QObject *videoSink){
    m_videoSink = qobject_cast<QVideoSink*>(videoSink);

    connect(m_videoSink, &QVideoSink::videoFrameChanged, this, &QZXingFilter::processFrame);
}

void QZXingFilter::processFrame(const QVideoFrame &frame) {
    if(isDecoding() || !processThread.isFinished()) return;

    decoding = true;
    QImage image = frame.toImage();
    processThread = QtConcurrent::run([=](){
        if(image.isNull())
        {
            qDebug() << "QZXingFilter error: Cant create image file to process.";
            decoding = false;
            return;
        }

        QImage frameToProcess(image);
        const QRect& rect = captureRect.toRect();

        if (captureRect.isValid() && frameToProcess.size() != rect.size()) {
            frameToProcess = image.copy(rect);
        }

        static int i=0;
        qDebug() << "image.size()" << frameToProcess.size();
        qDebug() << "image.format()" << frameToProcess.format();
        const QString path = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/qrtest/test_" + QString::number(i++ % 100) + ".png";
        qDebug() << "saving image" << i << "at:" << path << frameToProcess.save(path);

        decoder.decodeImage(frameToProcess, frameToProcess.width(), frameToProcess.height());
    });
}