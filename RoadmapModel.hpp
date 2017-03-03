#pragma once
#include <QAbstractItemModel>
#include <QAbstractProxyModel>
#include <KDGanttConstraintModel>
#include <KDGanttGlobal>
#include "Roadmap.hpp"
#include <QTimer>

// Enumerazione che descrive le collonne utilizzate dal modello
enum RoadmapModelColumns
{
    Name = 0, // Nome dell'elemento
    Type = 1, // Tipo // Qui KDGantt chiederà il tipo dell'elemento
    StartDate = 2, // Data di inizio
    EndDate = 3, // Data di fine
    Color = 4, // Colore
    Delivered = 5, // stato
    KDGanttType = 6 // Questa colonna è una colonna "fantasma" interrogata da KDGantt (Colonna max + 1)
};
class RoadmapConstraintModel;

/*
 * Dichiaro il modello
 * Il modello fa da "wrapper" all'oggetto Roadmap,
 *
 * Questa applicazione al di là delle routine di Salvataggio, Loading,
 * si basa totalmente sul modello.
 *
 * Le Qt offrono una gestione MVC già pronta da utilizzare,
 * KDGantt è compatibile con questa struttura
 *
 * In Qt l'MVC è composto in Model + View, il controller
 * è embeddato in parte nel modello e in parte nella View.
 * Il controller è suddiviso questo modo:
 *  - QAbstractItemModel espone gli eventi di layout changed, insertrow, move, delete ecc...
 *  - QAbstractItemView (Nel nostro caso implementato da KDGantt::View) si aggancia agli eventi esposti
 *    dal modello
 *
 * In questo file è definito inoltre RoadmapConstraintModel che implementa un KDGantt::ConstraintModel,
 * ConstraintModel non è un modello standard Qt ma è offerto dalla libreria, nel nostro caso lo esponiamo
 * come proprietà del modello base, RoadmapConstraintModel si occuperà di rappresentare le constriant dei
 * nostri project element in modo coerente con il modello sottostante e la Roadmap
 *
 * Tutto l'MVC in Qt gira intorno a QModelIndex.
 * Un QModelIndex è una "coordinata" che permette alla view di interoperare con il
 * modello, un QModelIndex è composto da una Riga, una Colonna, e un puntatore opzionale,
 * e espone dei metodi che permettono di estrarre informazioni da questo "indice",
 * un QModelIndex valido conosce sempre il modello di origine, la validità di un QModelIndex
 * è verificabile attraverso QModelIndex().isValid()
 *
 * QModelIndex(Row, Column, Pointer)
 *
 * QModelIndex può essere interrogata per il get e il set dei dati, generalmente QModelIndex non rappresenta mai
 * l'intera row, ma di solito si fa riferire ad un solo field, quindi ad esempio per rappresentare un nostro progetto
 * che è coposto da 5 Colonne, esisterà un QModelIndex per ogni colonna (Nome, Tipo, Data Inizio, Data Fine, KDGantt values)
 *
 * Ogni indice può essere interrogato per più tipi di ruoli
 *
 * La particolarità di QAbstractItemModel è che può rappresentare vari tipi di strutture dati,
 * Liste, Tabelle, Treeview
 *
 * Nel nostro caso il modello rappresenterà un tree di 3 generazioni, Root, Progetti ed elementi
 *
 * !QModelIndex().isValid() // Root, una Root in un modello Qt è definita tramite un index non valido
 *  |    * Generazione 1, Progetti
 *  | -> QMoldeIndex(0, 0) [Nome], QMoldeIndex(0, 1) [Type], QMoldeIndex(0, 2) [StartDate], QMoldeIndex(0, 3) [EndDate] ecc..
 *  |       |  * Generazione 2, Elementi
 *  |       |_> QMoldeIndex(0, 0) [Nome], QMoldeIndex(0, 1) [Type], QMoldeIndex(0, 2) [StartDate], QMoldeIndex(0, 3) [EndDate] ecc..
 *          |_> QMoldeIndex(1, 0) [Nome], QMoldeIndex(1, 1) [Type], QMoldeIndex(1, 2) [StartDate], QMoldeIndex(1, 3) [EndDate] ecc..
 *
 * NB: ogni record si identifica di una posizione relativa al proprio parent,
 *     ed il parent non è un parametro preimpostato all'interno dell'indice ma è valutato tramite la funzione parent
 *     del modello di origine, è compito del modello capire il "tipo" del QModelIndex, motivo per cui la nostra struttura dati
 *     implementa di default un modo per riconoscere il tipo direttamente a partire dal pointer.
 */
class RoadmapModel : public QAbstractItemModel {
    Roadmap* m_rmap; // L'oggetto roadmap "wrappato"
    RoadmapConstraintModel* m_cmodel; // Il constraint model
    QTimer* m_layoutchanger = nullptr; // Un timer per gestire un isteresi sui refresh

public:
	explicit RoadmapModel(Roadmap* rmap = nullptr, QObject * parent = nullptr);
	~RoadmapModel();

	Roadmap* roadmap() const;

    /*
     * Index interroga il modello a partire da un certo parent, e chiede un altro indice a seconda di Row, Column
     *
     * Noi abbiamo 2 casi corretti a cui rispondere
     *  - QModelIndex parent è invalid
     *      -> row si riferisce alla posizione del progetto
     *  - QModelIndex parent è valid
     *      -> dal parent ricavo il progetto base e da lì l'elemento da ritornare
     *
     * In tutti i casi in cui non sia possibile tornare un indice valido, si ritorna QModelIndex()
     */
	QModelIndex index(int row, int column, const QModelIndex& parent) const override;

    /*
     * Parent è una funzione che tenta di ritornare il padre di un indice
     *
     * I padri dei progetti sono indici Root quindi non validi
     * I padri degli elementi sono i relativi progetti
     */
	QModelIndex parent(const QModelIndex& child) const override;

    /*
     * Row count è una funzione che le view chiamano per sapere quante righe potranno interrogare
     * a partire da un certo parent
     *
     * Nel caso parent sia Root (invalid) torniamo il numero di progetti
     * Altrimenti recuperiamo il relativo progetto e torniamo il numero di elementi figli
     */
	int rowCount(const QModelIndex& parent) const override;

    /*
     * La treeview ci costringe a non poter avere un numero di colonne variabile a seconda
     * del tipo di elemento, quindi ritornerà sempre 6
     */
	int columnCount(const QModelIndex& parent) const override;

    /*
     * Dati di descrizione relativi alle intestazioni delle varie colonne, noi ritorneremo solo i titoli
     */
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    /*
     * Funzione che interroga un certo indice, in un certo ruolo, i ruoli servono per indicare
     * lo scopo per cui verrà utilizzato il dato, ad esempio, per le colonne StartDate, nel ruolo
     * di visualizzazione tornerò una stringa, nel ruolo di editing una data, nel ruolo di background
     * un colore, (i colore uguale per tutte le altre colonne)
     */
	QVariant data(const QModelIndex& index, int role) const override;

    /*
     * Funzione che invece imposta i valori all'interno delle colonne
     */
	bool setData(const QModelIndex& index, const QVariant& value, int role) override;

    /*
     * Funzione di inserimento delle righe
     */
	bool insertRows(int row, int count, const QModelIndex& parent) override;

    /*
     * Funzione di rimozione
     */
	bool removeRows(int row, int count, const QModelIndex& parent) override;

    /*
     * Non usata, return false
     */
	bool removeColumns(int column, int count, const QModelIndex& parent) override;

    /* Funzione di move (up\down) */
	bool moveRows(const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild) override;

    /*
     * innesca un isteresi a 50ms, se viene chiamata con una frequenza più alta di 20 volte\sec si resetta
     * si occupa anche di evitare un bug sulle constraints
     */
	void emitChanged();

    /*
     * Ottiene un cosntraint model che agisce direttamente sui link della Roadmap
     */
    RoadmapConstraintModel* constraintModel() const;

    /*
     * Ottiene dei modificatori che indicano alla View il behavior che può assumere su un certo indice
     * ad esempio se il campo è editabile o meno
     */
	Qt::ItemFlags flags(const QModelIndex& index) const override;

private:

    /*
     * Indica se c'è un refresh che attende di essere eseguito
     */
    bool isChanging();
};

class RoadmapConstraintModel : public KDGantt::ConstraintModel
{
    bool m_readonly; // Flag per hack di un bug della lib
    RoadmapModel* m_model; // Modello di riferimento da cui leggere Roadmap

public:
	explicit RoadmapConstraintModel(RoadmapModel* rmodel, QObject* parent = nullptr);
	~RoadmapConstraintModel() override;

	void addConstraint(const KDGantt::Constraint& c) override;
	bool removeConstraint(const KDGantt::Constraint& c) override;

	RoadmapModel* roadmapModel() const;

    /* Hack */
    void rebuildConstraints(); // Legge i link dalla Roadmap e li riaggiunge al modello
    void clearConstraints(); // Pulisce i link dal modello ma non li rimuove dalla Roadmap
	Roadmap* roadmap() const;
};

/* Definisce routine di utility utilizzate nell'implementazione del modello */
namespace ModelUtility
{

    inline RoadmapElement* unbox(const QModelIndex& index)
    {
        return dynamic_cast<RoadmapElement*>((RoadmapElement*)index.internalPointer());
    }

    inline bool isProjectElement(RoadmapElementType type)
    {
        return type & PROJECT_ELEMENT;
    }

    inline bool isProject(RoadmapElementType pro)
    {
        return pro == PROJECT;
    }

    inline RoadmapProject* unboxProject(const QModelIndex& index)
    {
        return static_cast<RoadmapProject*>(index.internalPointer());
    }

    inline RoadmapProjectElement* unboxPElement(const QModelIndex& index)
    {
        return static_cast<RoadmapProjectElement*>(index.internalPointer());
    }

    inline RoadmapMilestone* unboxMilestone(const QModelIndex& index)
    {
        return static_cast<RoadmapMilestone*>(index.internalPointer());
    }

    inline RoadmapTask* unboxTask(const QModelIndex& index)
    {
        return static_cast<RoadmapTask*>(index.internalPointer());
    }
}
