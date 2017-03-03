#include "RoadmapMainWnd.hpp"
#include "RoadmapView.hpp"
#include "RoadmapModel.hpp"
#include "RoadmapItemDelegate.hpp"
#include "RoadmapGrid.hpp"

#define SoftwareName "Roadmap Planet"
#define FormatExtension ".ropl"
#define MaxZoom 100
#define MinZoom 20

#include <QCloseEvent>
#include <QPushButton>
#include <QTreeView>
#include <QSplitter>
#include <QToolBar>
#include <KDGanttDateTimeGrid>
#include <QLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <KDGantt>
#include <KDGanttGlobal>
#include <QLineEdit>

RoadmapMainWnd::RoadmapMainWnd(QWidget *parent)
	: QMainWindow(parent)
{
    initToolbar(); // Creo la toolbar con tutti i bottoni
    clearLayout(); // Pulisco il layout (Qua serve solo per impostare i bottoni disabilitati)
}

RoadmapMainWnd::~RoadmapMainWnd()
{

}

void RoadmapMainWnd::closeEvent(QCloseEvent* e)
{
	if(m_pendingchanges)
		if(!Ask("Warning", "Exit without save?"))
            if (!ensureClose())
            {
				e->ignore();
                return;
            }

    clearLayout();
}

void RoadmapMainWnd::initToolbar()
{
	m_toolbar = new QToolBar(this);

	m_toolbar->setMovable(false);

    m_toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon); // Imposto lo stile
    m_toolbar->setIconSize(QSize(48, 48)); // la dimensione

    addToolBar(m_toolbar); // la imposto nella finestra corrente

    m_new = m_toolbar->addAction(QIcon(":/Icons/document-new-4.png"), "New.."); // Creo il bottone New (Con relativa icona)
    QObject::connect(m_new, &QAction::triggered, this, [=]() // Sull'evento di attivazione dell'evento attacco una lambda
	{
        if(ensureClose()) // Mi assicuro non ci sia niente di aperto
            setupGantt(); // Creo un nuovo Gantt vuoto
	});

    m_open = m_toolbar->addAction(QIcon(":/Icons/folder-open-3.png"), "Open.."); // Creo il bottone Open
    QObject::connect(m_open, &QAction::triggered, this, [=]()
	{
        open(); // Chiedo all'utente di aprire qualcosa
	});

    m_save = m_toolbar->addAction(QIcon(":/Icons/document-save-5.png"), "Save.."); // Cre
	QObject::connect(m_save, &QAction::triggered, this, [=]()
	{
        if (m_gantt == nullptr) return; //Check per gantt non inizializzato

        ensureSave(); // Tento di salvare
	});

    m_saveas = m_toolbar->addAction(QIcon(":/Icons/document-save-as-6.png"), "Save as..");
	QObject::connect(m_saveas, &QAction::triggered, this, [=]()
	{
        if (m_gantt == nullptr) return; //Check per gantt non inizializzato

        QString fpath = m_filepath; // Mi salvo ilvecchio
        m_filepath = ""; // Pulisco il percorso di riferimento così che ensureSave() provi a generare un nuovo percorso
		if (!ensureSave())
            m_filepath = fpath; // se ensureSave() non è riuscita a salvare reimposto il percorso precedente
	});

    m_zoomIn = m_toolbar->addAction(QIcon(":/Icons/zoom-in.png"), "Zoom In"); // Creo il bottone Zoom In
    QObject::connect(m_zoomIn, &QAction::triggered, this, [=]()
	{
        if (m_gantt == nullptr) return; // Check

        qreal dayWidth = grid()->dayWidth() + 10; // Ingrandisco la colonna del giorno di 10 pixel
        if (dayWidth > MaxZoom) { // Se sono andato oltre il massimo
            dayWidth = MaxZoom; // Reimposto sul massimo
            m_zoomIn->setEnabled(false); // Disabilito lo zoom In
		}
		else
            m_zoomOut->setEnabled(true); // nel caso fosse andato ok, abilito lo zoom out

        if (dayWidth >= 50) // Sopra una certa soglia faccio vedere una descrizione più estesa su settimane\giorni (Vedi RoadmapGrid)
			grid()->setScale(KDGantt::DateTimeGrid::ScaleUserDefined);

        grid()->setDayWidth(dayWidth); // Imposto la grandezza
	});

    m_zoomOut = m_toolbar->addAction(QIcon(":/Icons/zoom-out.png"), "Zoom Out"); // Creo il bottone Zoom out
	QObject::connect(m_zoomOut, &QAction::triggered, this, [=]()
	{
        // Stessa logica di ZoomIn al contrario
		if (m_gantt == nullptr) return;

		qreal dayWidth = grid()->dayWidth() - 10;
		if (dayWidth < MinZoom) {
			dayWidth = MinZoom;
			m_zoomOut->setEnabled(false);
		}
		else
			m_zoomIn->setEnabled(true);

		if (dayWidth <= 50)
			grid()->setScale(KDGantt::DateTimeGrid::ScaleDay);

		grid()->setDayWidth(dayWidth);
	});

    m_moveUp = m_toolbar->addAction(QIcon(":/Icons/go-up-4.png"), "Move Up"); // Creo il bottone Move Up
	QObject::connect(m_moveUp, &QAction::triggered, this, [=]()
	{
        if (m_gantt == nullptr) return;

        if (selectionModel()->hasSelection()) { // Se c'è qualcosa di selezionato
            QModelIndex idx = selectionModel()->currentIndex( ); // Ottengo l'indice selezionato
            if(m_model->moveRows(idx.parent(), idx.row(), 1, idx.parent(), idx.row() - 1)) // Yento di muoverlo
                selectRow(idx.parent(), idx.row() - 1); // Se si è mosso lo riseleziono
		}
	});

    m_moveDown = m_toolbar->addAction(QIcon(":/Icons/go-down-4.png"), "Move Down");
	QObject::connect(m_moveDown, &QAction::triggered, this, [=]()
	{
        //Stessa logica di MoveDown
		if (m_gantt == nullptr) return;

		if (selectionModel()->hasSelection()) {
			QModelIndex idx = selectionModel()->currentIndex();
			m_model->moveRows(idx.parent(), idx.row(), 1, idx.parent(), idx.row() + 1);
			selectRow(idx.parent(), idx.row() + 1);
		}
	});

    m_print = m_toolbar->addAction(QIcon(":/Icons/printer.png"), "Print"); // Crea il pulsante Print
	QObject::connect(m_print, &QAction::triggered, this, [=]()
	{
		if (m_gantt->model() == nullptr)
			return;

        QString filter = "PDF File (*.pdf)"; // Chiedo una desetinazione .pdf
        QString file = QFileDialog::getSaveFileName(this, "Select file", "", filter, &filter);
        if(file.isEmpty()) // Se ritorna un valore valido
            return;

        // Renderizzo in una QPrinter appositamente creata

        //Creazione della stampante
		QPrinter printer(QPrinter::HighResolution);
		printer.setOrientation(QPrinter::Landscape);
		printer.setColorMode(QPrinter::Color);
		printer.setPageMargins(0.2, 0.2, 0.2, 0.2, QPrinter::Point);
        printer.setOutputFormat(QPrinter::PdfFormat); // Impostazione Pdf
        printer.setOutputFileName(file); // File di destinazione

        m_gantt->print(&printer, false, true); // Commit sulla stampante
	});

    m_addProject = m_toolbar->addAction(QIcon(":/Icons/list.png"), "Add Project"); // Pulsante aggiungi progetto
	QObject::connect(m_addProject, &QAction::triggered, this, [=]()
	{
		if (m_gantt == nullptr) return;

        int prjRow = getProjectInsertRow(); // Ottengo la riga di inserimento
        if (m_model->insertRows(prjRow, 1, QModelIndex())) { // Tento di inserirlo

            // Se l'inserimento aviene con successo, ottengo l'indice della colonna Nome
            QModelIndex prjIndexName = m_model->index(prjRow, Name, QModelIndex());

            // Conto quanti progetti ci sono passando la Root a rowCount
            int pcount = m_model->rowCount(QModelIndex());

            // Imposto il nuovo nome a seconda del numero di progetti
            m_model->setData(prjIndexName, "New project " + QString::number(pcount), Qt::EditRole);

            // ottengo poi l'indice della colonna colore
            QModelIndex prjIndexColor = m_model->index(prjRow, Color, QModelIndex());

            // Imposto il colore prescato da una paletta di 20 colori
			m_model->setData(prjIndexColor, getColors(20).value(20 - pcount% 20), Qt::EditRole);

            // Seleziono la riga appena inserita
            selectRow(QModelIndex(), prjRow);
		}
	});

    m_addTask = m_toolbar->addAction(QIcon(":/Icons/edit-add-4.png"), "Add Task"); // Pulsante aggiungi Task
	QObject::connect(m_addTask, &QAction::triggered, [=]()
	{
		if (m_gantt == nullptr) return;

        QModelIndex project = getCurrentProject(); // Ottengo il progetto padre
        if(!project.isValid()) return; // Se non ci sono riuscito early exit

        int row = getProjectElementInsertRow(); // Ottengo la riga di inserimento
        if (m_model->insertRows(row, 1, project)) { // Tento di inserire la riga
            //Imposto il nome a seconda del numero degli elementi
            QModelIndex taskIndexName = m_model->index(row, Name, project);
			int pcount = m_model->rowCount(project);
            m_model->setData(taskIndexName, "New Task " + QString::number(pcount), Qt::EditRole);

            //Imposto la data di partenza a seconda della data dell'elemento precedente
            //Questom comportamento è definito dalla funzione newItemStartDate
			QModelIndex taskIndexDate = m_model->index(row, StartDate, project);
			m_model->setData(taskIndexDate, newItemStartDate(project, row), Qt::EditRole);

			selectRow(project, row);
		}
	});

    m_addMilestone = m_toolbar->addAction(QIcon(":/Icons/milestone.png"), "Add Milestone");
	QObject::connect(m_addMilestone, &QAction::triggered, [=]()
	{
		if (m_gantt == nullptr) return;

        QModelIndex project = getCurrentProject();
        if(!project.isValid()) return;

		int row = getProjectElementInsertRow();
		if (m_model->insertRows(row, 1, project)) {
			QModelIndex milestone = m_model->index(row, Type, project);
			m_model->setData(milestone, KDGantt::TypeEvent, KDGantt::ItemTypeRole);
			milestone = m_model->index(row, Type, project);

			QModelIndex milestoneName = m_model->index(milestone.row(), Name, project);
			int pcount = m_model->rowCount(project);
			m_model->setData(milestoneName, "Milestone " + QString::number(pcount), Qt::EditRole);

			QModelIndex milestoneDate = m_model->index(milestone.row(), StartDate, project);
			QDate date = newItemStartDate(project, row);
			QDate prjd = m_model->index(project.row(), EndDate, QModelIndex()).data(KDGantt::EndTimeRole).toDate();
			if (date > prjd && prjd != QDate::fromJulianDay(minJd()))
				date = prjd;
			m_model->setData(milestoneDate, date, Qt::EditRole);

			selectRow(project, row);
		}
	});

    m_delete = m_toolbar->addAction(QIcon(":/Icons/edit-delete-6.png"), "Delete");
	QObject::connect(m_delete, &QAction::triggered, [=]()
	{
		if (m_gantt == nullptr) return;

		if (selectionModel()->hasSelection()) {
            QModelIndex todelete = selectionModel()->currentIndex();
			int row = todelete.row();
			QModelIndex parent = todelete.parent();
			selectionModel()->clear();
			m_model->removeRows(row, 1, parent);
			int rc = m_model->rowCount(parent);
			if(rc > 0)
			{
				if (row >= rc)
					row--;

                selectRow(parent, row);
			} else
			{
				selectRow(parent);
			}
		}
	});
}

void RoadmapMainWnd::setupGantt()
{
    if (m_gantt != nullptr) return; // Se c'è già early exit

	m_gantt = new KDGantt::View();
	auto view = new RoadmapView();
	
    m_gantt->setGraphicsView(view); // imposto la view
	
    m_gantt->splitter()->setSizes( // Imposto la grandezza dei pannelli treeview\gantt
        QList<int>()
        << this->width() * 0.30 // 30%
		<< this->width() * 0.70 // 70%
	);
    m_gantt->setItemDelegate(new RoadmapItemDelegate(this)); // Imposto il behavior degli item

    RoadmapGrid* grid = new RoadmapGrid(m_gantt); // Imposto la custom grid dell'intestazione
    grid->setFreeDays(QSet<Qt::DayOfWeek>() << Qt::Saturday << Qt::Sunday); // Imposto sabato e domenica come giorni "festivi"
    m_gantt->setGrid(grid); //Imposto la griglia

    setCentralWidget(m_gantt); // Aggancio il gantt all finestra

	//auto rmap = createSampleRoadmap(this);
    auto rmap = new Roadmap(this); // Creo la Roadmap
    m_model = new RoadmapModel(rmap); // Creo il modello e glie la passo al modello

    m_gantt->setModel(m_model); // Imposto il modello sul gantt
    m_gantt->setConstraintModel(m_model->constraintModel()); // Imposto il ConstraintModel sul Gantt
    view->setSelectionModel(selectionModel()); // Imposto il selection model sulla view

    /*
     * Questi eventi mi indicano la dove il modello abbia committato qualche modifica
     * per sporcare il flag di changed
     */
    connect(m_model, &QAbstractItemModel::dataChanged, this, &RoadmapMainWnd::notifyChanged);
	connect(m_model, &QAbstractItemModel::rowsRemoved, this, &RoadmapMainWnd::notifyChanged);
	connect(m_model, &QAbstractItemModel::rowsInserted, this, &RoadmapMainWnd::notifyChanged);
	connect(m_model, &QAbstractItemModel::rowsAboutToBeMoved, this, &RoadmapMainWnd::notifyChanged);

    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, [=]() // Sul cambio di selezione
    {
        // Attivo e disattivo i bottoni
		QModelIndex index = selectionModel()->currentIndex();
		if (selectionModel()->hasSelection())
		{
			int r = index.row();
			int rc = m_model->rowCount(index.parent());

			m_moveUp->setEnabled(r > 0);
			m_moveDown->setEnabled(r < rc - 1);
			m_addProject->setEnabled(true);
			m_addMilestone->setEnabled(true);
			m_addTask->setEnabled(true);
            m_delete->setEnabled(true);
        }
		else
		{
			m_addProject->setEnabled(true);
			m_addMilestone->setEnabled(false);
			m_addTask->setEnabled(false);
			m_moveUp->setEnabled(false);
			m_moveDown->setEnabled(false);
			m_delete->setEnabled(false);
		}
	});


    // Imposto la grandezza delle colonne
    treeView()->hideColumn(Type); // Nascondo la colonna del tipo
	treeView()->setColumnWidth(Name, 140);
	treeView()->setColumnWidth(StartDate, 80);
	treeView()->setColumnWidth(EndDate, 80);
	treeView()->setColumnWidth(Color, 60);
	treeView()->setColumnWidth(Delivered, 60);
	treeView()->expandAll();

    m_model->emitChanged(); // Emitto un change per reimpostare correttamente il layout

	m_save->setEnabled(true);
	m_saveas->setEnabled(true);
	m_addProject->setEnabled(true);
	m_zoomIn->setEnabled(true);
	m_zoomOut->setEnabled(true);
	m_print->setEnabled(true);
	m_pendingchanges = false;

	refreshTitle();
}

void RoadmapMainWnd::notifyChanged()
{
	if (m_pendingchanges != true) {
		m_pendingchanges = true;
		refreshTitle();
	}
}

void RoadmapMainWnd::refreshTitle()
{
    // A seconda dello stato imposto il titolo in modo differente

	if (m_gantt == nullptr)
		setWindowTitle(SoftwareName);

	else
		if (m_filepath.isEmpty())
			if(m_pendingchanges)
				setWindowTitle(QString(SoftwareName) + " - edits in pending");
			else
				setWindowTitle(QString(SoftwareName));
		else
			if (m_pendingchanges)
                setWindowTitle(QString(SoftwareName) + " " + m_filepath + " - edits in pending");
			else
                setWindowTitle(QString(SoftwareName) + " " + " - " + m_filepath);
}

bool RoadmapMainWnd::ensureClose()
{
	if (m_model == nullptr)
		return true;

	if (m_pendingchanges)
	{
		if (!ensureSave())
			return false;
	}

	clearLayout();
	m_filepath = "";

	return true;

}

bool RoadmapMainWnd::ensureSave()
{
	if (m_model == nullptr)
		return false;

	if (m_filepath.isEmpty())
	{
        QString filter = QString(SoftwareName) + " Files (" + "*" + QString(FormatExtension) + ")"; // SW Name Files (*.ext)
        QString result = QFileDialog::getSaveFileName(this, "Select save destination", "", filter, &filter);
		if (result.isEmpty())
            return false;

		if (QFile::exists(result)) {
			if (!Ask("Warning", "Are you shure to overwrite?"))
				return false;

			if (!QFile::remove(result)) {
				Say("Error", "Can't write on this file.");
				return false;
			}
		}

		m_filepath = result;

		return trySave();
	}
	else
	{
		if(m_pendingchanges)
			if (Ask("Save?", "Do you want to save changes?"))
			{
				return trySave();
			}

		return false;
	}
}

bool RoadmapMainWnd::trySave()
{
	try {
		QFile file(m_filepath);

        if (!file.open(QIODevice::WriteOnly)) // Tento di aprire il file in scrittura
			return false;

        QDataStream stream(&file); // inizizlizzo lo stream in scrittura
        stream << *m_model->roadmap(); // tento la Serializzazione
		file.close();

        // Rinfresco il titolo e il flag per i cambiamenti in pending
        m_pendingchanges = false;
        refreshTitle();
		return true;
	}
	catch (std::exception e)
	{
		Say("Error", "An error occured saving file.");
		return false;
	}
}

bool RoadmapMainWnd::tryLoad()
{
	if (!QFile::exists(m_filepath))
		return false;

	try {		
		QFile file(m_filepath);

        if (!file.open(QIODevice::ReadOnly)) // se non è stato possibile aprire il file
			return false;

        setupGantt(); // inizializzo la finestra con il Gantt
	
        QDataStream stream(&file); // inizializzo lo stream di lettura
        stream >> *m_model->roadmap(); // tento la deserializzazione
        file.close();

		return true;
	}
	catch (std::exception e)
	{
		Say("Error", "An error occured loading file.");
		return false;
	}
}

QDate RoadmapMainWnd::newItemStartDate(QModelIndex parent, int position)
{
	if (position > 0 && parent.isValid())
	{
		QVariant vDate = parent.child(position - 1, EndDate).data(Qt::EditRole);
		if (vDate.isValid())
		{
			return vDate.toDate();
		}
		else
		{
			vDate = parent.child(position - 1, StartDate).data(Qt::EditRole);
			return vDate.toDate().addDays(1);
		}
	}
	else
		return QDate::currentDate();
}

void RoadmapMainWnd::clearLayout()
{
	setCentralWidget(nullptr);

	if (m_gantt != nullptr) {
		delete m_gantt;
		m_gantt = nullptr;
	}

	if (m_model != nullptr) {
		delete m_model;
		m_model = nullptr;
	}
	
	m_addProject->setEnabled(false);
	m_addMilestone->setEnabled(false);
	m_addTask->setEnabled(false);
    m_delete->setEnabled(false);

	m_zoomOut->setEnabled(false);
	m_zoomIn->setEnabled(false);

	m_moveUp->setEnabled(false);
	m_moveDown->setEnabled(false);

	m_print->setEnabled(false);

	m_new->setEnabled(true);
	m_open->setEnabled(true);
	m_save->setEnabled(false);
	m_saveas->setEnabled(false);

	refreshTitle();
}

int RoadmapMainWnd::getProjectInsertRow()
{
	if (m_gantt == nullptr)
        return -1;

	if (selectionModel()->hasSelection()) {
		QModelIndex sindex = selectionModel()->currentIndex();
		if (sindex.isValid())
		{
			switch (sindex.data(KDGantt::ItemTypeRole).toInt())
			{
			case KDGantt::TypeSummary:
				return sindex.row() + 1;
			case KDGantt::TypeEvent:
			case KDGantt::TypeTask:
				return sindex.parent().row();
			}
		}

		return m_model->rowCount(QModelIndex());
	}

	return 0;
}

int RoadmapMainWnd::getProjectElementInsertRow()
{
	if (m_gantt == nullptr)
		return -1;

    QModelIndex sindex = selectionModel()->currentIndex();
	if (sindex.isValid())
	{
		switch (sindex.data(KDGantt::ItemTypeRole).toInt())
		{
		case KDGantt::TypeSummary:
			return m_model->rowCount(sindex);
		case KDGantt::TypeEvent:
		case KDGantt::TypeTask:
			return sindex.row() + 1;
		}
	}

	return -1;
}

QModelIndex RoadmapMainWnd::getCurrentProject()
{
	if (m_gantt == nullptr)
		return QModelIndex();

    QModelIndex sindex = selectionModel()->currentIndex();
	switch (sindex.data(KDGantt::ItemTypeRole).toInt())
	{
	case KDGantt::TypeSummary:
		return sindex;
	case KDGantt::TypeEvent:
	case KDGantt::TypeTask:
		return sindex.parent();
	}

	return QModelIndex();
}

void RoadmapMainWnd::open()
{
    if (!ensureClose()) return; // Mi assicuro sia tutto chiuso

    // Creo il filtro per selezionare il file
    QString filter = QString(SoftwareName) + " Files (" + "*" + QString(FormatExtension) + ")";

    // Tento di selezionare il file
    QString result = QFileDialog::getOpenFileName(this, "Select file", "", filter, &filter);

    m_filepath = result; // Imposto il risultato
    if (!tryLoad()) // Se ho fallito nel caricamento
        m_filepath = ""; // Ri svuoto il percorso
}

void RoadmapMainWnd::selectRow(QModelIndex parent, int row)
{
	if(row > -1)
		selectionModel()->setCurrentIndex(m_model->index(row, 0, parent), QItemSelectionModel::SelectCurrent);
	else
		selectionModel()->setCurrentIndex(parent, QItemSelectionModel::SelectCurrent);	
}

QTreeView* RoadmapMainWnd::treeView() const
{
	return reinterpret_cast<QTreeView*>(m_gantt->leftView());
}

KDGantt::DateTimeGrid* RoadmapMainWnd::grid() const
{
	return static_cast<KDGantt::DateTimeGrid*>(m_gantt->grid());
}

QItemSelectionModel* RoadmapMainWnd::selectionModel() const
{
	return m_gantt->selectionModel();
}

bool RoadmapMainWnd::Ask(QString title, QString msg)
{
	QMessageBox::StandardButton reply = QMessageBox::question(this, title, msg, QMessageBox::Yes | QMessageBox::No);

	return reply == QMessageBox::Yes;
}

void RoadmapMainWnd::Say(QString title, QString msg)
{
	QMessageBox sayMsgBox;

	sayMsgBox.setText(title);
	sayMsgBox.setInformativeText(msg);
	sayMsgBox.setStandardButtons(QMessageBox::Ok);
	sayMsgBox.setDefaultButton(QMessageBox::Ok);
	sayMsgBox.show();
}
