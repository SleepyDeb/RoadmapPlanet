#include "RoadmapView.hpp"
#include <QGraphicsItem>
#include "RoadmapModel.hpp"

RoadmapView::RoadmapView(QWidget * parent) : KDGantt::GraphicsView(parent) {

}

void RoadmapView::addConstraint(const QModelIndex& from, const QModelIndex& to, Qt::KeyboardModifiers modifiers)
{
    // Se uno degli indici from e to puntano a un progetto anziché un elemento, non effettuo il link
    if (from.data(KDGantt::ItemTypeRole) == KDGantt::TypeSummary || to.data(KDGantt::ItemTypeRole) == KDGantt::TypeSummary)
		return;

    // Ottengo il constraint model
	RoadmapConstraintModel* cmodel =  reinterpret_cast<RoadmapConstraintModel*>(constraintModel());
    if (cmodel->hasConstraint(from, to)) // Controllo se avesse già questo link
	{
        // Poiché la funzione si deve comportare come un toggle, se l'elemento già esiste lo rimuovo
		cmodel->removeConstraint(KDGantt::Constraint(from, to));
		return;
	}

    //Stessa cosa invertendo to e from
	if(cmodel->hasConstraint(to, from))
	{
		cmodel->removeConstraint(KDGantt::Constraint(to, from));
		return;
	}

    //Altrimenti creo effettivamnte una nuova constraint
	cmodel->addConstraint(KDGantt::Constraint(from, to));
}
