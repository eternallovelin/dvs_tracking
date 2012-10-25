#include "camwidget.h"
#include <QPainter>
#include <QTimer>
#include <QColor>
#include <stdio.h>

CamWidget::CamWidget(QImage *i, std::vector<Cluster*> *c, QWidget *parent) : QWidget(parent)
{
    img = i;
    clusters = c;

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(20);
    setWindowTitle(tr("DVS128"));
    resize(512,512);
    counter = 0;
}

void CamWidget::paintEvent(QPaintEvent *event){
    QPainter painter(this);
    QRect rect(0,0,512,512);
    QColor color = Qt::black;
    painter.drawImage(rect,*img);

    //draw circle around cluster
    for(unsigned int i = 0; i < clusters->size(); i++){
        //if(!clusters->at(i)->candidate){
            int x = (127-clusters->at(i)->posX)*4;
            int y = (127-clusters->at(i)->posY)*4;
            painter.setPen(Qt::green);
            painter.drawEllipse(QPoint(x,y),30,30);
            //printf("x,y: %f %f                     \r",clusters->at(i)->posX,clusters->at(i)->posY);
            //printf("#clusters: %d  \r",clusters->size());
        //}
    }

    for(int x = 0; x < 128; x++){
        for(int y = 0; y < 128; y++){
            QRgb *pixel = (QRgb*)img->scanLine(y);
            pixel = &pixel[x];
            *pixel = color.rgb();
        }
    }
}

void CamWidget::setImage(QImage *i){
    img = i;
}


