#pragma once
#include <QGraphicsView>
#include <QAbstractItemView>
#include <kdganttgraphicsview.h>

/*
 * Overrido la GraphicsView di KDGantt per forzare le constraint solo tra gli elementi dei progetti
 */
class RoadmapView : public KDGantt::GraphicsView {

public:
	explicit RoadmapView(QWidget* parent = nullptr);

    // Override necessario
	void addConstraint(const QModelIndex& from, const QModelIndex& to, Qt::KeyboardModifiers modifiers) override;
};
