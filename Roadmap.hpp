#pragma once
/*
 * Questo file contiene le definizioni delle classi :
 *  - Roadmap : QObject()
 *      -> questa classe è la Root della struttura dati, gestisce la Serializzazione
 *         la Deserializzazione tramite gli operatori >> e << su QDataStream,
 *         Roadmap inoltre eredita QObject per permettere al framework Qt di poter
 *         disposare l'oggetto in maniera automatica.
 *         Roadmap è anche la factory dei sui progetti figli
 *
 *  - RoadmapElement
 *      -> è la classe base di tutti gli elementi definiti nella Roadmap,
 *         questa classe implementa un solo metodo type(), per permettere
 *         di capire il tipo dell'elemento la dove si ha solo il puntatore,
 *         i tipi possibili sono definiti dalla enumerazione RoadmapElementType
 *
 *  - RoadmapProject : RoadmapElement()
 *      -> è la classe che rappresenta un progetto, un progetto contiene una
 *         lista di ProjectElement, i project element definiti sono RoadmapTask e RoadmapMilestone
 *         RoadmapProject fa anche da factory di ProjectElements
 *
 *  - RoadmapProjectElement : RoadmapElement()
 *      -> è la classe base di tutti gli elementi contenuti in un progetto,
 *         un ProjectElement ha una lista di ProjectElement child gestiti tramite un Id
 *         assegnato dal progetto, i child possono essere anche cross progetti.
 *         inoltre un project element definisce sempre una data di partenza e un Nome
 *
 *  - RoadmapTask : RoadmapProjectElement()
 *      -> Un task è un project element che ha come parametro in più ha una durata in giorni
 *
 *  - RoadmapMilestone : RoadmapProjectElement()
 *      -> Una milestone è un project element che ha come parametro in più un valore booleano "delivered"
 *
 * NB: Tutti gli elementi della Roadmap ereditano RoadampElement,
 *     Esempio: RoadampMilestone -> RoadmapProjectElement -> RoadmapElement
 */
#include <QObject>
#include <QDate>
#include <QColor>

class Roadmap;
class RoadmapProjectElement;
class RoadmapTask;
class RoadmapMilestone;

/*
 * La enumerazione che definisce tutti i tipi di Elementi implementati
 */
enum RoadmapElementType
{
    PROJECT = 1 << 0, // Un progetto
    PROJECT_ELEMENT = 1 << 2, // Un elemento del progetto
    PROJECT_TASK = 1 << 3 | PROJECT_ELEMENT, // Un task è anche un ProjectElement
    PROJECT_MILESTONE = 1 << 4 | PROJECT_ELEMENT // Una milestone è anche un ProjectElement
};


/*
 * Classe base di tutti gli elementi che compongono la Roadmap,
 * Tranne la Roadmap stessa
 */
class RoadmapElement {
    /*
     * Campo Tipo, questo campo è privato e non potrà cambiare
     * durante la vita dell'oggetto
     */
    RoadmapElementType Type;

protected:
    /*
     * Questo costruttore costringe i genitori a passare un tipo
     */
    RoadmapElement(RoadmapElementType type);

public:
    RoadmapElementType type() const; //get del tipo


    /*
     * Gli operatori di Serializzazione\Deserializzazione hanno bisogno
     * di accedere ai field privati della struttura di oggeti,
     * più avanti tutte le classi dichiarate in questo file saranno friend
     * di questi operatori
     */
    friend QDataStream& operator << (QDataStream &out, Roadmap &project);
    friend QDataStream& operator >> (QDataStream &in, Roadmap &project);
};

class RoadmapProject : public RoadmapElement
{
    Roadmap* rmap; // Puntatore alla Roadmap padre
    QString Name; // Nome del progetto
    QColor Color; // Colore del progetto
    QList<RoadmapProjectElement*> Elements; // Lista degli elementi figli

    /*
     * Questo metodo serve a liberare tutte le referenze di un ProjectElement
     * Un ProjectElement è referenziato in primis dal suo progetto padre,
     * In più da tutti i project element che lo "puntano" come child,
     * I riferimenti a child possono essere anche cross project, per cui
     * per rimuovere tutte le referenze al progetto è necessario andare a cercare
     * tutti gli altri elementi in tutti gli altri progetti presenti nella Roadmap
     */
	void clearReferenceToElement(RoadmapProjectElement* element);

public:
    /*
     * Questo costruttore verrà chiamato solo in modo privato dalla Roadmap
     */
	explicit RoadmapProject(Roadmap* parent);
	~RoadmapProject();

    /*
     * Ottiene la Roadmap padre
     */
	Roadmap* roadmap() const;

    /*
     * Funzioni factory, generano l'elemento di tipo richiesto passandogli
     * il progetto corrente come padre, e aggiungendolo alla lista Elements
     */
	RoadmapTask* addTask();
	RoadmapMilestone* addMilestone();

    /*
     * Ottiene la posizione del progetto corrente all'interno della Roadmap
     */
	int position();

    /*
     * Elimina l'elemento selezionato dal progetto e lo libera da tutte
     * le possibili referenze
     */
	void delElement(RoadmapProjectElement* element);

    /*
     * Get degli elementi figli del progetto
     */
	QList<RoadmapProjectElement*> elements() const;

    /*
     * Get\Set del nome
     */
	QString name() const;
	void setName(const QString name);

    /*
     * Get\Set del colore
     */
	QColor color() const;
	void setColor(const QColor color);

    /*
     * Spostano l'elemento nella lista per cambiarlo di posizione
     */
	void movNext(RoadmapProjectElement* element);
	void movPrev(RoadmapProjectElement* element);

    /*
     * Calcolano la data di inizio e di fine
     */
	QDate startDate() const;
	QDate endDate() const;

    friend QDataStream& operator << (QDataStream &out, Roadmap &project);
    friend QDataStream& operator >> (QDataStream &in, Roadmap &project);
};

class RoadmapProjectElement : public RoadmapElement
{
    RoadmapProject* Project; //Ogni ProjectElement deve conoscere il progetto padre

    /*
     * L'id del ProjectElement è univoco per tutta la Roadmap
     * e permette ai ProjectElement di poter serializzare i link in fase di
     * salvataggio e caricamento.
     * I link nel file di salvataggio vengono salvati come:
     *   Id Roadmap padre => lista di id degli elementi figli
     *   Es: Element(12) => Element(13), Element(08), Element(09)
     */
    int Id;

    /*
     * Rappresenta la data di partenza
     */
	QDate Date;

    /*
     * Rappresenta la data di partenza
     */
	QString Name;

    /*
     * Lista dei figli del RoadmapProjectElement corrente
     */
	QList<RoadmapProjectElement*> Childs;

protected:
    /*
     * Il Costruttore di un ProjectElement ha bisogno di
     * - Un progetto padre
     * - Un id univoco (per la serializzazione dei link)
     * - Di un valore tipo
     */
	RoadmapProjectElement(RoadmapProject* parent, int id, RoadmapElementType type);

public:
	~RoadmapProjectElement();

    /*
     * Getters id, progetto padre
     */
	int id() const;
	RoadmapProject* project() const;

    /*
     * Get\Set data di inizio
     */
	QDate date() const;
	void setDate(const QDate& date);

    /*
     * Get\Set nome
     */
	QString name() const;
	void setName(const QString name);

    /*
     * Utility per collegare un altro ProjectElement a quello corrente
     */
	void addChild(RoadmapProjectElement* element);

    /*
     * Rimuove un link
     */
	void remChild(RoadmapProjectElement* element);

    /*
     * Ottiene la posizione dell'elemento a secondo della posizione
     * all'interno del progetto padre
     */
	int position();

    /*
     * Ottiene la lista dei figli
     */
	QList<RoadmapProjectElement*> childs() const;

    friend QDataStream& operator << (QDataStream &out, Roadmap &project);
    friend QDataStream& operator >> (QDataStream &in, Roadmap &project);
};

/*
 * Un Task è un particolare elemento del progetto che oltre
 * alle proprietà base di RoadmapProjectElement ha anche una
 * durata espressa in giorni
 */
class RoadmapTask : public RoadmapProjectElement
{
    int Days; // Durata del task in giorni

public:
    /*
     * Progetto padre, Id univoco
     */
    explicit RoadmapTask(RoadmapProject* parent, int id);

    /*
     * Get dei giorni
     */
	int days() const;

    /*
     * Set dei giorni
     */
	void setDays(const int days);

    /*
     * Data di fine calcolata come date() (Proprietà di tutti i ProjectElement)
     * più la durata (days)
     */
	QDate endDate() const;

    friend QDataStream& operator << (QDataStream &out, Roadmap &project);
    friend QDataStream& operator >> (QDataStream &in, Roadmap &project);
};

/*
 * Una milestone è un particolare elemento del progetto che oltre
 * alle proprietà base di RoadmapProjectElement un flag chiamato
 * delivered
 */
class RoadmapMilestone : public RoadmapProjectElement
{
    bool Delivered; // Flag delivered della Milestone

public:
    /*
     * Progetto padre, Id univoco
     */
	explicit RoadmapMilestone(RoadmapProject* parent, int id);

    /*
     * Get\Set dello stato della milestone
     */
	bool delivered() const;
	void setDelivered(const bool delivered);

    friend QDataStream& operator << (QDataStream &out, Roadmap &project);
    friend QDataStream& operator >> (QDataStream &in, Roadmap &project);
};

/*
 * Entrypoint della struttura di oggetti
 *
 * Roadmap eredita QObject per usufruire della
 * "distruzione" automatica fornita da Qt,
 * I QObject quando vengono istanziati richiedono un parent,
 * così che nel caso il parent venga eliminato il distruttore
 * dell'oggeto implementato venga chiamato in automatico,
 * questo meccanismo rende l'oggetto molto più sicuro
 */
class Roadmap : QObject
{
    /*
     * Lista dei progetti figli della Roadmap
     */
    QList<RoadmapProject*> Projects;

public:
    /*
     * Il parent dell'oggetto per assicurare il destroy
     */
    explicit Roadmap(QObject* parent = nullptr);

    /*
     * Nel distruttore mi occupo di fare il delete
     * di tutti gli oggetti alloccati dinamicamente nell'Heap
     */
	~Roadmap();

    /*
     * Produce un Id univoco per i project elements
     */
	int nextId() const;

    /*
     * Elenca i progetti della Roadmap
     */
	QList<RoadmapProject*> projects() const;

    /*
     * Crea un progetto e lo aggancia alla Roadmap corrente
     */
	RoadmapProject* addProject();

    /*
     * Elimina un progetto
     */
	void delProject(RoadmapProject* project);

    /*
     * Muove di 1 in avanti la posizione un progetto
     */
	void movNextPosition(RoadmapProject* project);

    /*
     * Muove di 1 indietro la posizione un progetto
     */
	void movPrevPosition(RoadmapProject* project);

    /*
     * Trova un project element a partire dal suo Id
     */
	RoadmapProjectElement* findElementById(int id) const;

	friend QDataStream& operator << (QDataStream &out, Roadmap &project);
	friend QDataStream& operator >> (QDataStream &in, Roadmap &project);
};

/*
 * Dichiarazione del Serializzatore
 */
QDataStream& operator << (QDataStream &out, Roadmap &project);

/*
 * Dichiarazione di un Deserializzatore
 */
QDataStream& operator >> (QDataStream &in, Roadmap &project);
