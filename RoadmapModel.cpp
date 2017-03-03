#include "RoadmapModel.hpp"
#include <KDGanttGlobal>
#include <KDGanttStyleOptionGanttItem>
#include <QItemSelectionModel>
#include <Utility.hpp>

using namespace ModelUtility;

RoadmapConstraintModel::RoadmapConstraintModel(RoadmapModel* model, QObject* parent)
{
	m_readonly = false;
	m_model = model;

	if (model == nullptr)
		m_model = new RoadmapModel(nullptr, this);

	rebuildConstraints();
}

RoadmapConstraintModel::~RoadmapConstraintModel()
{
}

void RoadmapConstraintModel::addConstraint(const KDGantt::Constraint& c)
{
	if(!m_readonly)
	{
		RoadmapElement* rpe = unbox(c.startIndex());
		RoadmapElement* rce = unbox(c.endIndex());

		if (!isProjectElement(rpe->type())) { return; }
		if (!isProjectElement(rce->type())) { return; }
		 
		RoadmapProjectElement* pelement = unboxPElement(c.startIndex());
		RoadmapProjectElement* celement = unboxPElement(c.endIndex());
		pelement->addChild(celement);
	}

	KDGantt::ConstraintModel::addConstraint(c);
}

bool RoadmapConstraintModel::removeConstraint(const KDGantt::Constraint& c)
{
	if (!m_readonly) {
		RoadmapProjectElement* pelement = unboxPElement(c.startIndex());
		RoadmapProjectElement* celement = unboxPElement(c.endIndex());
		if (pelement != nullptr && celement != nullptr)
			pelement->remChild(celement);
	}

	return KDGantt::ConstraintModel::removeConstraint(c);
}

void RoadmapConstraintModel::rebuildConstraints()
{
	for (RoadmapProject* project : roadmap()->projects())
	{
		QModelIndex projectIndex = m_model->index(project->position(), 0, QModelIndex());
		for (RoadmapProjectElement* element : project->elements())
		{
			QModelIndex pelementIndex = m_model->index(element->position(), 0, projectIndex);
			for (RoadmapProjectElement* child : element->childs())
			{
				QModelIndex destProjectIndex = m_model->index(child->project()->position(), 0, QModelIndex());
				QModelIndex celementIndex = m_model->index(child->position(), 0, destProjectIndex);
				ConstraintModel::addConstraint(KDGantt::Constraint(pelementIndex, celementIndex));
			}
		}
	}
}

void RoadmapConstraintModel::clearConstraints()
{
	m_readonly = true;    
	QList<KDGantt::Constraint> cs = constraints();
	for (KDGantt::Constraint c : cs)
		removeConstraint(c);
	m_readonly = false;
}

Roadmap* RoadmapConstraintModel::roadmap() const
{
	return m_model->roadmap();
}

RoadmapModel::RoadmapModel(Roadmap* rmap, QObject* parent) : QAbstractItemModel(parent)
{
	m_rmap = rmap;

	if (m_rmap == nullptr)
		m_rmap = new Roadmap(this);

	m_cmodel = new RoadmapConstraintModel(this, this);
}

Roadmap* RoadmapModel::roadmap() const
{
	return m_rmap;
}

QModelIndex RoadmapModel::index(int row, int column, const QModelIndex& parent) const
{
	if (!parent.isValid())
	{
		QList<RoadmapProject*> projects = roadmap()->projects();

		if (row >= projects.count())
			return QModelIndex();

		return createIndex(row, column, projects.value(row));
	}

	RoadmapElement* element = unbox(parent);

	if (isProject(element->type()))
	{
		RoadmapProject* project = unboxProject(parent);

		QList<RoadmapProjectElement*> elements = project->elements();
		if (row >= elements.count())
			return QModelIndex();

		return createIndex(row, column, elements.value(row));
	}

	return QModelIndex();
}

QModelIndex RoadmapModel::parent(const QModelIndex& child) const
{
	if (!child.isValid() || child.model() != this)
		return QModelIndex();

	RoadmapElement* element = unbox(child);

	if (element == nullptr)
		return QModelIndex();

	if (isProject(element->type()))
		return QModelIndex();

	if (isProjectElement(element->type())) {
		RoadmapProjectElement* pelement = unboxPElement(child);
		RoadmapProject* project = pelement->project();
		return createIndex(project->position(), 0, project);
    }

    return QModelIndex();
}

int RoadmapModel::rowCount(const QModelIndex& parent) const
{
	if (!parent.isValid())
		return roadmap()->projects().count();

	RoadmapElement* element = unbox(parent);
	if (isProject(element->type())) {
		RoadmapProject* project = unboxProject(parent);
		return project->elements().count();
	}

	return 0;
}

int RoadmapModel::columnCount(const QModelIndex& parent) const
{
	//Name, Type, StartDate, EndDate, Color, Delivered (KDGanttType)
	return 6;
}

QVariant RoadmapModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
		return QVariant();

	switch (section) {
	case Name: return tr("Name");
	case Type: return tr("Type");
	case StartDate: return tr("Start");
	case EndDate: return tr("End");
	case Color: return tr("Color");
	case Delivered: return tr("Delivered");
	case KDGanttType: return tr("KDGanttType");
	default: return QVariant();
	}
}

QVariant RoadmapModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();

	//if (role == Qt::TextAlignmentRole)
	//	return Qt::AlignCenter;

	RoadmapElement* element = unbox(index);
	if (element == nullptr)
		return QVariant();

	if (index.column() == Type || role == KDGantt::ItemTypeRole)
	{
		switch (element->type())
		{
		case PROJECT: return static_cast<int>(KDGantt::TypeSummary);
		case PROJECT_TASK: return static_cast<int>(KDGantt::TypeTask);
		case PROJECT_MILESTONE: return static_cast<int>(KDGantt::TypeEvent);
		default: break;
		}
	}

	if(role == Qt::BackgroundRole)
	{
		switch (element->type())
		{
		case PROJECT: return unboxProject(index)->color();
		case PROJECT_TASK: 
			return getLighter(unboxPElement(index)->project()->color(), 1.10);
		case PROJECT_MILESTONE: 
			if(!unboxMilestone(index)->delivered())
				return getLighter(unboxPElement(index)->project()->color(), 1.10);
			else
				return QColor(0, 200, 30);
		default: break;
		}
	}

	if(role == Qt::TextColorRole)
	{
		switch (element->type())
		{
		case PROJECT: return getIdealTextColor(unboxProject(index)->color());
		case PROJECT_TASK:
		case PROJECT_MILESTONE:
			return getIdealTextColor(getLighter(unboxPElement(index)->project()->color(), 1.10));
		default: break;
		}
	}

	if(index.column() == Type)
	{
		switch (role)
		{
		case Qt::DisplayRole:
		case Qt::EditRole:
			switch (element->type())
			{
			case PROJECT: return tr("Project");
			case PROJECT_TASK: return tr("Task");
			case PROJECT_MILESTONE: return tr("Milestone");
			default: break;
			}
		}
	}

	switch (element->type())
	{
	case PROJECT: {
		RoadmapProject* project = unboxProject(index);

		if (role == KDGantt::StartTimeRole)
			return project->startDate();

		if (role == KDGantt::EndTimeRole)
			return project->endDate();

		if (role == KDGantt::TextPositionRole)
			return KDGantt::StyleOptionGanttItem::Center;

		switch ((RoadmapModelColumns)index.column())
		{
		case Name:
			switch (role) {
			case Qt::DisplayRole:
			case Qt::EditRole:
				return project->name();
			}
			break;

		case Color:
			switch (role)
			{
			case Qt::DisplayRole:
			case Qt::EditRole:
				return project->color();
			}
			break;

		case StartDate:
			switch (role)
			{
			case Qt::DisplayRole:
				return project->startDate().toString("dd-MM-yyyy");

			case Qt::EditRole:
				return project->startDate();
			}
			break;

		case EndDate:
			switch (role)
			{
			case Qt::DisplayRole:
				return project->endDate().toString("dd-MM-yyyy");

			case Qt::EditRole:
				return project->endDate();
			}
			break;
		}
		break;
	}
	case PROJECT_MILESTONE: {
		RoadmapMilestone* milestone = unboxMilestone(index);

		if (role == KDGantt::TextPositionRole)
			return KDGantt::StyleOptionGanttItem::Right;

		if (role == KDGantt::StartTimeRole)
			return milestone->date();

		if (role == KDGantt::EndTimeRole)
			return QDate();

		switch ((RoadmapModelColumns)index.column())
		{
		case Name:
			switch (role) {
			case Qt::DisplayRole:
			case Qt::EditRole:
				return milestone->name();
			}
			break;

		case StartDate:
			switch (role)
			{
			case Qt::DisplayRole:
				return milestone->date().toString("dd-MM-yyyy");

			case Qt::EditRole:
				return milestone->date();
			}
			break;
		case Delivered:
			switch (role)
			{
			case Qt::DisplayRole:
				return milestone->delivered() ? tr("Delivered") : tr("None");

			case Qt::EditRole:
				return milestone->delivered();
			}
		}
		break;
	}
	case PROJECT_TASK: {
		RoadmapTask* task = unboxTask(index);

		if (role == KDGantt::TextPositionRole)
			return KDGantt::StyleOptionGanttItem::Center;

		if (role == KDGantt::StartTimeRole)
			return task->date();

		if (role == KDGantt::EndTimeRole)
			return task->endDate();

		switch ((RoadmapModelColumns)index.column())
		{
		case Name:
			switch (role) {
			case Qt::DisplayRole:
			case Qt::EditRole:
				return task->name();
			}
			break;

		case StartDate:
			switch (role)
			{
			case Qt::DisplayRole:
				return task->date().toString("dd-MM-yyyy");

			case Qt::EditRole:
				return task->date();
			}
			break;

		case EndDate:
			switch (role)
			{
			case Qt::DisplayRole:
				return task->endDate().toString("dd-MM-yyyy");

			case Qt::EditRole:
				return task->endDate();
			}
			break;
		}
		break;
	}
	}

	return QVariant();
}

bool RoadmapModel::setData(const QModelIndex& idx, const QVariant& value, int role)
{
    if (!idx.isValid())
		return false;

	RoadmapElement* element = unbox(idx);

	bool result = false;
	switch (element->type())
	{
	case PROJECT: {
		RoadmapProject* project = unboxProject(idx);
			switch (idx.column())
			{
				case Name:
					project->setName(value.toString());
					result = true;
					break;

				case Color:
					project->setColor(value.value<QColor>());
					result = true;
					break;
			}
		break;
	}
	case PROJECT_TASK: {
		RoadmapTask* task = unboxTask(idx);
		switch (idx.column())
		{
		case Name:
			task->setName(value.toString());
			result = true;
			break;

		case Type:
			if(value.toInt()==KDGantt::TypeEvent)
			{
				emitChanged();
				QModelIndex pidx = parent(idx);
				beginRemoveRows(pidx, task->position(), task->position());
				QString name = task->name();
				QDate date = task->date();
				int p = task->position();

				RoadmapProject* parent = task->project();
				parent->delElement(task);
				endRemoveRows();

				beginInsertRows(pidx, p, p);
				RoadmapMilestone* miles = parent->addMilestone();
				miles->setName(name);
				miles->setDate(date);

 				while(miles->position() > p)
					parent->movPrev(miles);

				endInsertRows();
				//result = true;
				return true;
		/*		break;*/
			}

		case StartDate:
			task->setDate(value.toDate());
			result = true;
			break;

		case EndDate:
			int days = task->date().daysTo(value.toDate());
			if (days > 0) {
				task->setDays(task->date().daysTo(value.toDate()));
				result = true;
			} else
			{
				task->setDays(1);
				result = true;
			}
			break;
		}
		break;
	}
	case PROJECT_MILESTONE: {
		RoadmapMilestone* milestone = unboxMilestone(idx);
		switch (idx.column())
		{
		case Name:
			milestone->setName(value.toString());
			result = true;
			break;

		case StartDate:
			milestone->setDate(value.toDate());
			result = true;
			break;

		case Delivered:
			milestone->setDelivered(value.toBool());
			result = true;
			break;
		}
		break;
	}
	}

    if (result) {
		emitChanged();
		emit dataChanged(idx, idx);
	}

	return result;
}

bool RoadmapModel::insertRows(int row, int count, const QModelIndex& parent)
{
    if(isChanging()) return false;

	bool inserted = false;
	if(!parent.isValid())
	{
		emitChanged();
		beginInsertRows(parent, row, row + count - 1);
		while(count-- > 0)
		{
			int pcount = m_rmap->projects().count();
			RoadmapProject* pro = m_rmap->addProject();

			if (row < m_rmap->projects().count())
				while (pro->position() < row)
					m_rmap->movNextPosition(pro);
			
			if (row > -1)
				while (pro->position() > row)
					m_rmap->movPrevPosition(pro);
		}
		endInsertRows();
		inserted = true;
	} else
	{
		RoadmapProject* project = unboxProject(parent);
		emitChanged();
		beginInsertRows(parent, row, row + count - 1);
		while (count-- > 0)
		{
			int ccount = project->elements().count();
			RoadmapTask* task = project->addTask();

			if (row < project->elements().count())
				while (task->position() < row)
					project->movNext(task);

			if (row > -1)
				while (task->position() > row)
					project->movPrev(task);
		}
		endInsertRows();
		inserted = true;
	}

	return inserted;
}

bool RoadmapModel::removeRows(int row, int count, const QModelIndex& parent)
{
    if(isChanging()) return false;

	if(!parent.isValid())
	{
		if (roadmap()->projects().count() <= row + count - 1)
			return false;

		emitChanged();
		beginRemoveRows(parent, row, row + count - 1);
		while (count-- > 0) {
			roadmap()->delProject(roadmap()->projects().value(row));
		}
		endRemoveRows();
	} else
	{
		RoadmapProject* project = unboxProject(parent);
		if (project->elements().count() <= row + count - 1)
			return false;

		emitChanged();
		beginRemoveRows(parent, row, row + count - 1);
		while (count-- > 0) {
			project->delElement(project->elements().value(row));
		}
		endRemoveRows();
	}

	return true;
}

bool RoadmapModel::removeColumns(int column, int count, const QModelIndex& parent)
{
	return false;
}

bool RoadmapModel::moveRows(const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild)
{
    if(isChanging()) return false;

    m_cmodel->clearConstraints();
    layoutChanged();
	if (sourceParent != destinationParent)
		return false;

	int delta = destinationChild - sourceRow;

	if (delta == 0)
        return true;

	if(!sourceParent.isValid())
	{
		for (int p = sourceRow; p < count + sourceRow; p++) {
			RoadmapProject* project = roadmap()->projects().value(p);
			if (delta > 0)
				for (int i = 0; i < delta; i++)
					roadmap()->movNextPosition(project);
            else
				for (int i = 0; i < delta * -1; i++)
                    roadmap()->movPrevPosition(project);
		}
    } else {
		RoadmapProject* project = unboxProject(sourceParent);
		for (int p = sourceRow; p < count + sourceRow; p++) {
			RoadmapProjectElement* element = project->elements().value(p);
			if (delta > 0)
                for (int i = 0; i < delta; i++)
                    project->movNext(element);
			else
                for (int i = 0; i < delta * -1; i++)
                    project->movPrev(element);
		}
    }

    layoutChanged();
    m_cmodel->rebuildConstraints();
	return true;
}

void RoadmapModel::emitChanged()
{
	if(m_layoutchanger == nullptr)
	{
		m_layoutchanger = new QTimer();
		m_layoutchanger->setInterval(50);
		m_layoutchanger->setSingleShot(true);
		QObject::connect(m_layoutchanger, &QTimer::timeout, this, [&]()
		{
			QTimer* timer = m_layoutchanger;
			m_layoutchanger = nullptr;
			emit layoutChanged();
            m_cmodel->rebuildConstraints();
			delete timer;
		});
        emit layoutAboutToBeChanged();
		m_cmodel->clearConstraints();
		m_layoutchanger->start();
	}
	else {
		m_layoutchanger->stop();
		m_layoutchanger->start();
	}
}

RoadmapConstraintModel* RoadmapModel::constraintModel() const
{
    return m_cmodel;
}

Qt::ItemFlags RoadmapModel::flags(const QModelIndex& idx) const 
{
	if (!idx.isValid())
		return QAbstractItemModel::flags(idx);

	Qt::ItemFlags fl = Qt::NoItemFlags;

	RoadmapElement* element = unbox(idx);

	switch (element->type())
	{
	case PROJECT: {
		switch (idx.column())
		{
		case Name:
			fl = Qt::ItemIsEditable;
			break;

		case Color:
			fl = Qt::ItemIsEditable;
			break;
		}
		break;
	}
	case PROJECT_TASK: {
		RoadmapTask* task = unboxTask(idx);
		switch (idx.column())
		{
		case Name:
			fl = Qt::ItemIsEditable;
			break;

		case StartDate:
			fl = Qt::ItemIsEditable;
			break;

		case EndDate:
			fl = Qt::ItemIsEditable;
			break;
		}
		break;
	}
	case PROJECT_MILESTONE: {
		RoadmapMilestone* milestone = unboxMilestone(idx);
		switch (idx.column())
		{
		case Name:
			fl = Qt::ItemIsEditable;
			break;

		case StartDate:
			fl = Qt::ItemIsEditable;
			break;

		case Delivered:
			fl = Qt::ItemIsEditable;
			break;
		}
		break;
	}
	}

	return QAbstractItemModel::flags(idx) | fl;
}

bool RoadmapModel::isChanging()
{
    return m_layoutchanger != nullptr;
}

RoadmapModel::~RoadmapModel() {

}
