#pragma once
#include <QMainWindow>
#include <qcoreapplication.h>
#include <KDGanttView>
#include <KDGanttDateTimeGrid>
#include "RoadmapModel.hpp"

class QTreeView;

/*
 * Finestra che gestisce l'interop programm
 */
class RoadmapMainWnd : public QMainWindow
{
    /*
     * Il programma è diviso in 2 sezioni:
     *  - Toolbar
     *  - TreeView + Gantt
     */
	QToolBar* m_toolbar;

    /* Comandi nella toolbar */
    QAction* m_addProject; // Aggiungi progetto
    QAction* m_addMilestone; // Aggiungi milestone
    QAction* m_addTask; // Aggiungi Task
    QAction* m_delete; // Elimina elemento

    /* Gestione dello Zoom */
    QAction* m_zoomOut; // Zoom In
    QAction* m_zoomIn; // zoom out

    QAction* m_moveUp; // Sposta l'elemento inalto
    QAction* m_moveDown; // Sposta l'elemento in basso

    QAction* m_print; // Stampa il gantt

    QAction* m_new; // Crea una nuova roadmap
    QAction* m_open; // Apri una roadmap
    QAction* m_save; // Salva la roadmap
    QAction* m_saveas; // Salva la roadmap in una nuova posizione


    RoadmapModel* m_model = nullptr; // Modello visualizzato attualmente
    KDGantt::View* m_gantt = nullptr; // Gantt

    QString m_filepath; // Percorso del file aperto
    bool m_pendingchanges = false; // Flag che indica se ci sono modifiche non salvate
	
public:
	explicit RoadmapMainWnd(QWidget *parent = nullptr);
	~RoadmapMainWnd();

protected:
    void closeEvent(QCloseEvent*) override; // Viene eseguita al momento della chiusura

private:
    void initToolbar(); // Inizializza tutti i tasti della toolbar

    void setupGantt(); // Inizializza il body con il gantt, setappa il model e inizializza i bottoni

    void notifyChanged(); // Imposta flag di changed e aggiorna il title se cambia lo stato
    void refreshTitle(); // Reimposta il title a seconda della situazione

    /*
     * Se nell'editor non c'è niente in editing ritorna true direttamente
     * Se nell'editor c'è qualcosa in editing ma non è stato modificato ritorna true
     * Se nell'editor c'è qualcosa in editing di modificato ma non salvato tenta di salvarlo
     * se il salvataggio va a buon fine torna true, altrimenti false
     */
    bool ensureClose();

    /*
     * Ensure save tenta di salvare, se non è mai stato selezionato un path di destinazione
     * chiede all'utente, se fallisce ritorna false, altrimenti true
     */
	bool ensureSave();

    /*
     * Tenta di salvare fisicamente nell'ultimo percorso impostato in m_filepath
     */
	bool trySave();

    /*
     * Tenta di caricare e inizializzare la finestra corrente dal percorso m_filepath
     */
	bool tryLoad();

    /*
     * Algoritmo che genera la data di partenza del nuovo elemento figlio di un progetto
     */
	QDate newItemStartDate(QModelIndex parent, int position);

    /*
     * Disposa il Gantt e resetta il layout della finestra corrente
     */
	void clearLayout();

    /*
     * Ottiene l'indice di inserimento per un nuovo progetto
     */
	int getProjectInsertRow();

    /*
     * Ottiene l'indice di inserimento per un elemento
     */
	int getProjectElementInsertRow();

    /*
     * Cerca di ottenere il progetto selezionato attualmente,
     * Se nel momento in cui viene chiamata c'è selezionato un elemento
     * anziché un progetto ritorna il progetto padre
     */
	QModelIndex getCurrentProject();

    /*
     * Funzione che chiede all'utente di aprire un file
     */
	void open();

    /* Utility che riseleziona una riga dato un certo indice riga e un padre */
	void selectRow(QModelIndex parent, int row = -1);

    /* Unboxa la treeView() interna al Gantt */
	QTreeView* treeView() const;

    /* Unbox la grid() internal al Gantt */
	KDGantt::DateTimeGrid* grid() const;

    /* Unbox il selection modeò */
	QItemSelectionModel* selectionModel() const;

    /* Crea una dialog interrogativa Sì\No */
	bool Ask(QString title, QString msg);

    /* Crea una dialog affermativa */
	void Say(QString title, QString msg);
};

