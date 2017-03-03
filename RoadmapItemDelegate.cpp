#include "RoadmapItemDelegate.hpp"

#include <QPainter>

#include <KDGanttStyleOptionGanttItem>
#include "RoadmapModel.hpp"
#include <QColorDialog>
#include <QApplication>
#include "Utility.hpp"

using namespace ModelUtility;

RoadmapItemDelegate::RoadmapItemDelegate(QObject * parent) : ItemDelegate(parent) {
	
}

RoadmapItemDelegate::~RoadmapItemDelegate() {
	
}

bool RoadmapItemDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{	
    //Nel caso la colonna cliccata sia quella del colore, e l'indice indichi un elemento valido
	if(index.column()  == Color && index.isValid())
    {
        // Ottengo il tipo dell'elemento
        KDGantt::ItemType typ = static_cast<KDGantt::ItemType>(model->data(index, KDGantt::ItemTypeRole).toInt());
        if (typ == KDGantt::TypeSummary) { // Se l'elmento è un summary (un summary in KDGantt è un progetto per Roadmap)
            const QColor color = index.model()->data(index, Qt::BackgroundRole).value<QColor>(); // Ottengo il colore
            QColor newcolor = QColorDialog::getColor(color); // Ottengo un nuovo colore
            if (newcolor != color && newcolor.value() != QColor().black()) // Se è diverso dal colore ottenuto e diverso da nero
			{
                return model->setData(index, newcolor, Qt::BackgroundRole); // imposto il colore
			}
			else
                return false; // Niente da fare
		}
	}

    // In tutti gli altri casi richiamo l'implementazione standard
    return QItemDelegate::editorEvent(event, model, option, index);
}

KDGantt::Span RoadmapItemDelegate::itemBoundingSpan(const KDGantt::StyleOptionGanttItem& opt, const QModelIndex& idx) const
{
	const QString txt = idx.model()->data(idx, Qt::DisplayRole).toString();
	const int typ = idx.model()->data(idx, KDGantt::ItemTypeRole).toInt();
	QRectF itemRect = opt.itemRect;


	if (typ == KDGantt::TypeEvent) {
		itemRect = QRectF(itemRect.left() - itemRect.height() / 2.,
			itemRect.top(),
			itemRect.height(),
			itemRect.height());
	}

	int tw = opt.fontMetrics.width(txt) * 1.2;
	tw += static_cast<int>(itemRect.height() / 2.);
	KDGantt::Span s;
	switch (opt.displayPosition) {
	case KDGantt::StyleOptionGanttItem::Left:
		s = KDGantt::Span(itemRect.left() - tw, itemRect.width() + tw); break;
	case KDGantt::StyleOptionGanttItem::Right:
		s = KDGantt::Span(itemRect.left(), itemRect.width() + tw); break;
	case KDGantt::StyleOptionGanttItem::Hidden: // fall through
	case KDGantt::StyleOptionGanttItem::Center:
		s = KDGantt::Span(itemRect.left(), itemRect.width()); break;
	}
	return s;
}

void RoadmapItemDelegate::paintGanttItem(QPainter* painter, const KDGantt::StyleOptionGanttItem& opt, const QModelIndex& idx)
{
	if (!idx.isValid()) return;
	const KDGantt::ItemType typ = static_cast<KDGantt::ItemType>(idx.model()->data(idx, KDGantt::ItemTypeRole).toInt());

	const QString& txt = opt.text;
	QRectF itemRect = opt.itemRect;
	QRectF boundingRect = opt.boundingRect;

	boundingRect.setY(itemRect.y());
	boundingRect.setHeight(itemRect.height());

	painter->save();

	QFont font = painter->font();
	font.setBold(true);
	painter->setFont(font);
	

	QLinearGradient taskgrad(0., 0., 0., QApplication::fontMetrics().height());
	QColor bgcolor = idx.model()->data(idx, Qt::BackgroundRole).value<QColor>();
	taskgrad.setColorAt(0., bgcolor);
	taskgrad.setColorAt(1., getLighter(bgcolor, 1.20));
	painter->setBrush(taskgrad);

	QPen pen;
	if (opt.state & QStyle::State_Selected) pen.setWidth(2 * pen.width());
	painter->setPen(pen);

	bool drawText = true;
	qreal pw = painter->pen().width() / 2.;
	switch (typ) {
	case KDGantt::TypeTask:
		if (itemRect.isValid()) {
			qreal pw = painter->pen().width() / 2.;
			pw -= 1;
			QRectF r = itemRect;
			r.translate(0., r.height() / 6.);
			r.setHeight(2.*r.height() / 3.);
			painter->setBrushOrigin(itemRect.topLeft());
			painter->save();
			painter->translate(0.5, 0.5);
			painter->drawRect(r);
			painter->restore();
		}
		break;
	case KDGantt::TypeSummary:
		if (opt.itemRect.isValid()) {
			taskgrad.setColorAt(0., bgcolor);
			taskgrad.setColorAt(1., getLighter(bgcolor, 0.8));
			painter->setBrush(taskgrad);

			pw -= 1;
			const QRectF r = QRectF(opt.itemRect).adjusted(-pw, -pw, pw, pw);
			QPainterPath path;
			const qreal deltaY = r.height();
			const qreal deltaXBezierControl = .25*qMin(r.width(), r.height());
			const qreal deltaX = qMin(r.width(), r.height());
			path.moveTo(r.topLeft());
			path.lineTo(r.topRight());
			path.lineTo(QPointF(r.right(), r.top() + 2.*deltaY));
			//path.lineTo( QPointF( r.right()-3./2.*delta, r.top() + delta ) );
			path.quadTo(QPointF(r.right() - deltaXBezierControl, r.top() + deltaY), QPointF(r.right() - deltaX, r.top() + deltaY));
			//path.lineTo( QPointF( r.left()+3./2.*delta, r.top() + delta ) );
			path.lineTo(QPointF(r.left() + deltaX, r.top() + deltaY));
			path.quadTo(QPointF(r.left() + deltaXBezierControl, r.top() + deltaY), QPointF(r.left(), r.top() + 2.*deltaY));
			path.closeSubpath();
			painter->setBrushOrigin(itemRect.topLeft());
			painter->save();
			painter->translate(0.5, 0.5);
			painter->drawPath(path);
			painter->restore();
		}
		break;
	case KDGantt::TypeEvent: /* TODO */
		if (opt.boundingRect.isValid()) {
			const qreal pw = painter->pen().width() / 2. - 1;
			const QRectF r = QRectF(opt.itemRect).adjusted(-pw, -pw, pw, pw).translated(-opt.itemRect.height() / 2, 0);
			QPainterPath path;
			const qreal delta = static_cast< int >(r.height() / 2);
			path.moveTo(delta, 0.);
			path.lineTo(2.*delta, delta);
			path.lineTo(delta, 2.*delta);
			path.lineTo(0., delta);
			path.closeSubpath();
			painter->save();
			painter->translate(r.topLeft());
			painter->translate(0, 0.5);
			painter->drawPath(path);
			painter->restore();
		}
		break;
	}

	if(typ != KDGantt::TypeEvent)
	{
		pen.setColor(getIdealTextColor(bgcolor));
		painter->setPen(pen);
		painter->drawText(boundingRect, Qt::AlignCenter | Qt::AlignVCenter, txt);
	}
	else {
		//boundingRect.translate(9, 0);
		painter->drawText(boundingRect, Qt::AlignRight | Qt::AlignVCenter, txt);
	}

	painter->restore();
}

void RoadmapItemDelegate::paintConstraintItem(QPainter* p, const QStyleOptionGraphicsItem& opt, const QPointF& start, const QPointF& end, const KDGantt::Constraint& constraint)
{
	p->setRenderHints(QPainter::Antialiasing);
	qreal midx = (end.x() - start.x()) / 2. + start.x();
	qreal midy = (end.y() - start.y()) / 2. + start.y();

	if (start.x() <= end.x()) {
		p->setPen(Qt::black);
		p->setBrush(Qt::black);
	}
	else {
		p->setPen(Qt::red);
		p->setBrush(Qt::red);
	}
	if (start.x() > end.x() - TURN) {
		QPainterPath path(start);
		path.quadTo(QPointF(start.x() + TURN * 2., (start.y() + midy) / 2.),
			QPointF(midx, midy));
		path.quadTo(QPointF(end.x() - TURN * 2., (end.y() + midy) / 2.),
			QPointF(end.x() - TURN / 2., end.y()));
		QBrush brush = p->brush();
		p->setBrush(QBrush());
		p->drawPath(path);
		p->setBrush(brush);
		QPolygonF arrow;
		arrow << end
			<< QPointF(end.x() - TURN / 2., end.y() - TURN / 2.)
			<< QPointF(end.x() - TURN / 2., end.y() + TURN / 2.);
		p->drawPolygon(arrow);
	}
	else {
		p->setBrush(QBrush());
		QPainterPath path(start);
		path.cubicTo(QPointF(midx, start.y()),
			QPointF(midx, end.y()),
			QPointF(end.x() - TURN / 2., end.y()));
		p->drawPath(path);
		p->setBrush(Qt::black);
		QPolygonF arrow;
		arrow << end
			<< QPointF(end.x() - TURN / 2., end.y() - TURN / 2.)
			<< QPointF(end.x() - TURN / 2., end.y() + TURN / 2.);
		p->drawPolygon(arrow);
	}
}

