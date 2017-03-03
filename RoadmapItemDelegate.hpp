#pragma once
#include <kdganttitemdelegate.h>

/*
 * KDGantt:ItemDelegate permette di overridare alcuni behavior
 * degli Item all'interno della grid
 */
class RoadmapItemDelegate : public KDGantt::ItemDelegate {
    const qreal TURN = 10.; // Fattore minimo di raggio per le curve delle frecce

public:
	explicit RoadmapItemDelegate(QObject * parent = nullptr);
	~RoadmapItemDelegate();


protected:
    /*
     * Permette di overridare il comportamento sul tentativo di modifica dei valori nel modello
     */
	bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

public:
    /*
     * Ottiene uno span relativo alla linea temporale a seconda dell'elemento da disegnare
     * In RoadmapPlanet viene utilizzato poiché per scrivere le scritte in grassetto abbiamo bisogno
     * di un po' più di spazio nel caso l'elemento sia una milestone
     */
	KDGantt::Span itemBoundingSpan(const KDGantt::StyleOptionGanttItem& opt, const QModelIndex& idx) const override;

    /*
     * Routine che disegna i vari tipi di "Item" della Roadmap
     */
	void paintGanttItem(QPainter* p, const KDGantt::StyleOptionGanttItem& opt, const QModelIndex& idx) override;

    /*
     * Routine che disegna le "frecce" dei links tra gli oggetti
     */
	void paintConstraintItem(QPainter* p, const QStyleOptionGraphicsItem& opt, const QPointF& start, const QPointF& end, const KDGantt::Constraint& constraint) override;
private:
	
};
