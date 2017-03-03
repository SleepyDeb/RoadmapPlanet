#include "RoadmapGrid.hpp"
#include <QBrush>
#include <QStyleOptionHeader>
#include <QApplication>

RoadmapGrid::RoadmapGrid(QObject * parent) : DateTimeGrid() {
    setParent(parent);
	this->setUserDefinedLowerScale(new KDGantt::DateTimeScaleFormatter(KDGantt::DateTimeScaleFormatter::Day, QString::fromLatin1("ddd dd")));
	this->setScale(KDGantt::DateTimeGrid::ScaleUserDefined);
}

RoadmapGrid::~RoadmapGrid() {
	
}

void RoadmapGrid::paintUserDefinedHeader(QPainter* painter, const QRectF& headerRect, const QRectF& exposedRect, qreal offset, const KDGantt::DateTimeScaleFormatter* formatter, QWidget* widget)
{
	const QStyle* const style = widget ? widget->style() : QApplication::style();

	QDateTime dt = formatter->currentRangeBegin(mapToDateTime(offset + exposedRect.left())).toUTC();
	qreal x = mapFromDateTime(dt);

	while (x < exposedRect.right() + offset) {
		const QDateTime next = formatter->nextRangeBegin(dt);
		const qreal nextx = mapFromDateTime(next);

		QStyleOptionHeader opt;
		if (widget) opt.init(widget);
		opt.rect = QRectF(x - offset + 1, headerRect.top(), qMax(1., nextx - x - 1), headerRect.height()).toAlignedRect();

		opt.textAlignment = formatter->alignment();

		if (formatter->range() == KDGantt::DateTimeScaleFormatter::Week) {
			int delta = Qt::Monday - dt.date().dayOfWeek();
			QDate firstDayOfWeek = dt.date().addDays(delta);
			QDate lastDayOfWeek = dt.date().addDays(7 - delta);

			if(firstDayOfWeek.month() != lastDayOfWeek.month())
			{
				opt.text = QString("%1 / %2").arg(firstDayOfWeek.toString("MMMM yy")).arg(lastDayOfWeek.toString("MMMM yy"));
			} else
			{
				QDate firstday = QDate(dt.date().year(), dt.date().month(), 1);
				int weekinmonth = firstday.daysTo(firstDayOfWeek) / 7 + 1;
				opt.text = QString("%1 settimana di %2").arg(weekinmonth).arg(firstDayOfWeek.toString("MMMM yy"));
            }
		}
		else {
			opt.text = formatter->text(dt);
		}
		// use white text on black background
		opt.palette.setColor(QPalette::Window, QColor("black"));
		opt.palette.setColor(QPalette::ButtonText, QColor("black"));

		style->drawControl(QStyle::CE_Header, &opt, painter, widget);

		dt = next;
		x = nextx;
	}
}
