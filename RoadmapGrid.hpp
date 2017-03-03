#pragma once
#include <kdganttdatetimegrid.h>

/*
 * KDGantt::DateTimeGrid è la classe che si occupa di definire
 * e disegnare l'header della grid
 */
class RoadmapGrid : public KDGantt::DateTimeGrid {

public:
	RoadmapGrid(QObject * parent = nullptr);
	~RoadmapGrid();

protected:
    /*
     * Dell'header l'unica cosa che ridefinisco è il drawing
     */
	void paintUserDefinedHeader(QPainter* painter, const QRectF& headerRect, const QRectF& exposedRect, qreal offset, const KDGantt::DateTimeScaleFormatter* formatter, QWidget* widget) override;
	
};
