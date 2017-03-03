#include "Roadmap.hpp"

#include <QtCore>
#include <QDate>
#include "Utility.hpp"

void RoadmapProject::clearReferenceToElement(RoadmapProjectElement* element)
{
    /*
     * Per ogni progetto presente nella roadmap
     */
    for (RoadmapProject* project : rmap->projects()) {
        /*
         * Tento di rimuovere ogni possibile link all'elemento
         * Per farlo scorro tutti gli elementi di tutti i progetti
         * e chiamo remChild
         */
        for (RoadmapProjectElement* pElement : project->elements())
            pElement->remChild(element);

        /*
         * Tento di rimuovere l'elemento dal progetto
         */
        project->Elements.removeOne(element);
    }
}

/*
 * Inizializzo tutti i fields della classe
 * Passo la costante "PROJECT" alla classe base RoadmapElement
 * ad identificare il tipo del "RoadmapElement"
 */
RoadmapProject::RoadmapProject(Roadmap* parent) : RoadmapElement(PROJECT), rmap(parent), Name("New Project"), Color(QColor(255, 0, 0)), Elements()
{
}

/*
 * All'eleminazione del progetto
 */
RoadmapProject::~RoadmapProject()
{
    /*
     * Sgancio le referenze
     */
    for (RoadmapProjectElement* element : Elements)
        clearReferenceToElement(element);

    /*
     * Se avessi eliminato subito gli ogetti, la clearreference
     * avrebbe generato un access violation
     */
    for(RoadmapProjectElement* element: Elements)
        delete element; // elimino gli elementi dallo Heap

    /*
     * Pulisco la lista di elementi
     */
    Elements.clear();

    /*
     * sgancio il progetto dalla roadmap
     */
    rmap = nullptr;
}

Roadmap* RoadmapProject::roadmap() const
{
    return rmap; // Ritorno il puntatore alla Roadmap
}

RoadmapTask* RoadmapProject::addTask()
{
    /*
     * Alloco un oggetto task nello heap, generando un nuovo id
     * univoco
     */
    RoadmapTask* task = new RoadmapTask(this, rmap->nextId());

    /*
     * Lo aggiungo agli elementi del progetto corrente
     */
    Elements.append(task);

    /*
     * Lo restituisco al chiamante
     */
    return task;
}

RoadmapMilestone* RoadmapProject::addMilestone()
{
    /*
     * Alloco un oggetto milestone nello heap, generando un nuovo id
     * univoco, lo aggiungo all'istanza corrente e lo ritorno.
     */
    RoadmapMilestone* mile = new RoadmapMilestone(this, rmap->nextId());
    Elements.append(mile);
    return mile;
}

int RoadmapProject::position()
{
    /*
     * tramite riferimento alla roadmap ottengo la lista dei progetti
     * e ottengo lindice dell'istanza corrente usando il metodo
     * indexOf delle QList
     */
    return roadmap()->projects().indexOf(this);
}

void RoadmapProject::delElement(RoadmapProjectElement* element)
{
    /*
     * Controllo che l'elemento passato
     * esista nell'istanza corrente
     */
    if (!Elements.contains(element))
        return;

    /*
     * Pulisco ogni referenza all'elemento
     */
    clearReferenceToElement(element);

    /*
     * Lo rimuovo dalla lista
     */
    Elements.removeOne(element);

    delete element;
}

QList<RoadmapProjectElement*> RoadmapProject::elements() const
{
    return Elements; // Ritorno la lista di elementi
}

QString RoadmapProject::name() const
{
    return Name; // Ritorno il nome del progetto
}

void RoadmapProject::setName(const QString name)
{
    Name = name; // Imposto il nuovo nome del progetto
}

QColor RoadmapProject::color() const
{
    return Color; // Ritorno il colore del progetto
}

void RoadmapProject::setColor(const QColor color)
{
    Color = color; // Imposto il nuovo colore del progetto
}

void RoadmapProject::movNext(RoadmapProjectElement* element)
{
    // Controllo che l'elemento non sia nullo, altrimenti early exit
    if (element == nullptr) return;

    // Ottengo l'indice dell'elemento
    int ei = Elements.indexOf(element); // Element Index

    /*
     * per Element index < 0 l'elemento non è presente nell'istanza corrente
     *  Early Exit per elemento non valido
     *
     * per Element index == Ultimo indice valido
     *  Early exit perché l'elemento è già all'ultima posizione valida
     */
    if (ei < 0 || ei == Elements.count() - 1)
        return;

    Elements.removeAt(ei); // Sgancio l'elemento momentaneamente
    Elements.insert(ei + 1, element); // Lo reinserisco Element Index + 1
}

void RoadmapProject::movPrev(RoadmapProjectElement* element)
{
    /*
     * Stesso identico discorso che per MoveNext
     */
    if (element == nullptr) return;
    int ei = Elements.indexOf(element);

    /*
     * Ovviamente in questo caso controllo che sia
     * possibile spostare l'elemento indietro,
     * l'ultima posizione valida sarebbe 1
     */
    if (ei < 0 || ei < 1)  return;
    Elements.removeAt(ei);
    Elements.insert(ei - 1, element);
}

QDate RoadmapProject::startDate() const
{
    /*
     * Per calcolare la data di inizio vado a cercare l'elemento
     * che inizia alla data più piccola
     * uso la costante maxJd() (Maximum Julian Day) per
     * avere un termine di paragone per andare a cercare la data minima
     */
    QDate minDate = QDate::fromJulianDay(maxJd());
    bool changed = false; // Flag per assicurarsi che la ricerca abbia avuto successo

    /*
     * Per ogni elemento
     */
    for (RoadmapProjectElement* element : elements())
    {
        /*
         * Se l'elemento selezionato ha una data pià piccola di minDate,
         * (Questa situazione è assicurata al primo giro del loop)
         */
        if (element->date() < minDate) {
            /*
             * re imposto minDate e flaggo changed
             */
            minDate = element->date();
            changed = true;
        }
    }

    /*
     * Se non è stato trovato nulla, creo una QDate vergine
     */
    if (!changed)
        minDate = QDate();

    return minDate;
}

QDate RoadmapProject::endDate() const
{
    /*
     * Stesso discorso di start date, ovviamente al contrario cercando la data più grande
     * in questo caso prendo in considerazione solo gli elementi di tipo task
     */
    QDate maxDate = QDate::fromJulianDay(minJd());
    bool changed = false;

    for (RoadmapProjectElement* element : elements())
    {
        /*if (element->type() == PROJECT_MILESTONE)
        {
            if (element->date() > maxDate) {
                maxDate = element->date();
                changed = true;
            }
        }*/

        /*
         * Se l'elemento è un task
         */
        if (element->type() == PROJECT_TASK)
        {
            /*
             * Casto l'elemento a RoadmapTask pointer
             */
            RoadmapTask* task = static_cast<RoadmapTask*>(element);

            /*
             * Se la data di fine è più grande di maxDate
             * imposto maxDate e flaggo changed
             */
            if (task->endDate() > maxDate) {
                maxDate = task->endDate();
                changed = true;
            }
        }
    }

    if (!changed)
        maxDate = QDate();

    return maxDate;
}

RoadmapElement::RoadmapElement(RoadmapElementType type)
{
    Type = type; // Inizializzo il tipo dell'elemento intanziato
}

RoadmapElementType RoadmapElement::type() const
{
    return Type; // Ritorno il tipo dell'elemento corrente
}

/*
 * Inizializzo tutti i fields della classe
 * Un ProjectElement ha:
 *  - Un RoadmapElementType
 *  - Un id univoco
 *  - Un progetto padre
 *  - Una data di riferimento
 *  - Un nome
 *  - La lista dei figli
 */
RoadmapProjectElement::RoadmapProjectElement(RoadmapProject* parent, int id, RoadmapElementType type) : RoadmapElement(type), Project(parent), Id(id), Date(QDate::currentDate()), Childs()
{

}

RoadmapProjectElement::~RoadmapProjectElement()
{
    /*
     * Alla distruzione basta pulire la lista dei figli
     * e sganciare il progetto padre dall'istanza corrente
     */
    childs().clear();
    Project = nullptr;
}

int RoadmapProjectElement::id() const
{
    return Id; // Ritorno l'id dell'elemento
}

RoadmapProject* RoadmapProjectElement::project() const
{
    return Project; // Ritorno il progetto padre
}

QDate RoadmapProjectElement::date() const
{
    return Date; // Ritorno la data di riferimento dell'elemento
}

void RoadmapProjectElement::setDate(const QDate& date)
{
    Date = date; // Imposto la data di riferimento dell'elemento
}

QString RoadmapProjectElement::name() const
{
    return Name; // Ritorno il nome
}

void RoadmapProjectElement::setName(const QString name)
{
    Name = name; // Reimposto il nome
}

void RoadmapProjectElement::addChild(RoadmapProjectElement* element)
{
    // Se l'elemento è già presente nella lista dei child
    if (Childs.contains(element))
        return; // Early exit

     //Non posso aggiungere come figlio me stesso
    if(element == this)
        return; // Early exit

    // Aggancio il child
    Childs.append(element);
}

void RoadmapProjectElement::remChild(RoadmapProjectElement* element)
{
    // Se l'elemento non è presente nella lista dei child
    if (!Childs.contains(element))
        return; // Early exit

    // Rimuovo la prima corrispondenza nella lista dei child
    Childs.removeOne(element);
}

int RoadmapProjectElement::position()
{
    /* Ottengo la lista degli elementi dal progetto padre
     * chiamo il metodo indexOf per ottenere la posizione
     * dell'elelemto corrente
     */
    return project()->elements().indexOf(this);
}

QList<RoadmapProjectElement*> RoadmapProjectElement::childs() const
{
    return Childs; // Ritorno la lista dei childs
}

/*
 * Inizializzo tutti i fields della classe
 * Un ProjectTask ha:
 *  - Un RoadmapElementType impostato con la costante PROJECT_TASK
 *  - Un id univoco
 *  - Un progetto padre
 *  - Una data di riferimento
 *  - Un nome
 *  - La lista dei figli
 *  - Una durata indicata in giorni
 */
RoadmapTask::RoadmapTask(RoadmapProject* parent, int id) : RoadmapProjectElement(parent, id, PROJECT_TASK), Days(1)
{
}

int RoadmapTask::days() const
{
    return Days; // Ritorno la durata in giorni
}

void RoadmapTask::setDays(const int days)
{
    Days = days; // Imposto la durata in giorni
}

QDate RoadmapTask::endDate() const
{
    /*
     * La data di fine di un task è la sua data di partenza
     * più la durata del task
     */
    return date().addDays(days());
}


/*
 * Inizializzo tutti i fields della classe
 * Un ProjectMilestone ha:
 *  - Un RoadmapElementType impostato con la costante PROJECT_MILESTONE
 *  - Un id univoco
 *  - Un progetto padre
 *  - Una data di riferimento
 *  - Un nome
 *  - La lista dei figli
 *  - Un flag che ne descrive lo stato chiamato "Delivered"
 */
RoadmapMilestone::RoadmapMilestone(RoadmapProject* parent, int id) : RoadmapProjectElement(parent, id, PROJECT_MILESTONE), Delivered(false)
{
}

bool RoadmapMilestone::delivered() const
{
    return Delivered; // Ritorno lo stato
}

void RoadmapMilestone::setDelivered(const bool delivered)
{
    Delivered = delivered; // Imposto lo stato
}

int Roadmap::nextId() const
{
    // L'id di partenza è 1
    int maxId = 1;


    /*
     * Cerco l'id più grande presente nella Roadmap
     *
     * Per ogni progetto
     */
    for (RoadmapProject* pro : projects()) {
        /*
         * Per ogni elemento
         */
        for (RoadmapProjectElement* element : pro->elements())
        {
            if (element->id() > maxId)
                maxId = element->id();
        }
    }

    /*
     * Ritorno l'id più grande + 1
     */
    return maxId + 1;
}

Roadmap::Roadmap(QObject* parent) : QObject(parent), Projects()
{
}

Roadmap::~Roadmap()
{
    /*
     * Elimino ogni progetto all'interno della Roadmap
     */
    QList<RoadmapProject*> l = this->Projects;
    for (RoadmapProject* project : l) {
        Projects.removeOne(project);
        delete project;
    }
}

QList<RoadmapProject*> Roadmap::projects() const
{
    return Projects; // Ritorno la lista dei progetti
}

RoadmapProject* Roadmap::addProject()
{
    /*
     * Per aggiungere un progetto
     * Creo un Progetto
     * Lo aggiungo all'istanza corrente
     * E lo ritorno
     */
    RoadmapProject* project = new RoadmapProject(this);
    Projects.append(project);
    return  project;
}

void Roadmap::delProject(RoadmapProject* project)
{
    /*
     * Provo a rimuovere l'elemento dalla lista dei progetti
     * Se la remove ritorna true
     * Significa che era un progetto effettivamente contenuto nella lista dei progetti
     * Nel caso removeOne ritorni true elimino il progetto dall'heap
     */
    if (Projects.removeOne(project))
        delete project;
}

void Roadmap::movNextPosition(RoadmapProject* project)
{
    if (project == nullptr)
        return; // Se è null early exit

    int pi = Projects.indexOf(project); // Ottengo Project Index

    if (pi < 0 || pi == Projects.count() - 1)  // Se l'indice non è valido
        return; // Early exit

    Projects.removeAt(pi); // Lo rimuovo dalla lista
    Projects.insert(pi + 1, project); // Lo inseriesco una posizione più avanti
}

void Roadmap::movPrevPosition(RoadmapProject* project)
{
    if (project == nullptr)
        return; // Se è null early exit

    int pi = Projects.indexOf(project); // Ottengo Project Index
    if (pi < 0 || pi == 0)
        return; // Early exit

    Projects.removeAt(pi); // Lo rimuovo dalla lista
    Projects.insert(pi - 1, project); // Lo inseriesco una posizione più indietro
}

RoadmapProjectElement* Roadmap::findElementById(int id) const
{
    /*
     * Per ogni progetto
     */
    for (RoadmapProject* pro : projects())
        /*
         * Per ogni elemento
         */
        for (RoadmapProjectElement* ele : pro->elements())
            if (ele->id() == id) //Se l'id è quello giusto
                return ele; // Ritorno l'elemento

    return nullptr; // L'elemento non è stato trovato
}

QDataStream& operator<<(QDataStream& out, Roadmap& rmap)
{
    /*
     * Ogni call a << scrive nello stream,
     * è importante l'ordine con cui vengono fatte
     * le scritture, questo stabilisce il binary format
     * del file di salvataggio
     *
     * Serializzo qusta stringa come "magic string"
     */
    out << "RoadmapPlanet01";

    /*
     * Serializzo il numero di progetti
     */
    out << rmap.Projects.count();

    /*
     * Inizializzo un hash map per memorizzare
     * i link tra gli elementi (1 a N)
     */
    QMap<int, QList<int>> links;

    /*
     * Per ogni progetto
     */
    for (RoadmapProject* pro : rmap.Projects)
    {
        out << pro->Name; // Serializzo il nome
        out << pro->Color; // Serializzo il Colore

        out << pro->Elements.count(); // Serializzo il numero di elementi


        /*
         * Per ogni elemento negli elementi
         */
        for (RoadmapProjectElement* element : pro->Elements)
        {
            /*
             * A seconda del tipo
             */
            switch (RoadmapElementType(element->Type))
            {
            case PROJECT_TASK:
            {
                /* Recupero il task */
                RoadmapTask* task = static_cast<RoadmapTask*>(element);
                out << task->Id; // Serializzo l'id
                out << static_cast<int>(task->Type); // Serializzo il tipo
                out << task->Date; // Serializzo la data
                out << task->Name; // Serializzo il nome
                out << task->Days; // Serializzo la durata
                break;
            }
            case PROJECT_MILESTONE:
            {
                /* Recupero la Milestone */
                RoadmapMilestone* mile = static_cast<RoadmapMilestone*>(element);
                out << mile->Id; // Serializzo l'id
                out << static_cast<int>(mile->Type); // Serializzo il tipo
                out << mile->Date; // Serializzo la data
                out << mile->Name; // Serializzo il nome
                out << mile->Delivered; // Serializzo il flag Delivered
                break;
            }
            }

            //Instanzio la lista degli id figli
            QList<int> cIds;

            /*
             * Per ogni figlio
             */
            for (RoadmapProjectElement* lElement : element->Childs)
            {
                /* Aggiungo l'id dell'elemento alla lista */
                cIds.append(lElement->Id);
            }

            /*
             * Aggiungo la lista degli id all'elemento del progetto
             */
            links.insert(element->Id, cIds);
        }


    }

    /*
     * Serializzo la mappa dei links
     */
    out << links;

    // Ritorno lo stream
    return out;
}

QDataStream& operator >> (QDataStream& in, Roadmap& rmap)
{
    QString versionName;

    in >> versionName; // Deserializzo la stringa di versione

    int pCount;
    in >> pCount; // Ottengo il numero dei progetti

    while (pCount-- > 0) // Itero per ogni progetto
    {
        RoadmapProject* pro = rmap.addProject(); // Ricreo un progetto

        in >> pro->Name; // Recupero il nome
        in >> pro->Color; // Recupero il colore

        int eCount;
        in >> eCount; // Recupero il numero di elementi figli

        while (eCount-- > 0) // Itero per ogni elemento
        {
            /* Dichiaro i field in comune per ogni project element */
            int id;
            int type;
            QDate date;
            QString name;

            in >> id; // Recupero l'id
            in >> type; // Recupero il tipo
            in >> date; // Recupero la data
            in >> name; // recupero il nome

            RoadmapProjectElement* element = nullptr;

            // A seconda del tipo creo un elemento diverso
            if (type == PROJECT_MILESTONE)
            {
                // Creo una milestone con l'id recuperato
                RoadmapMilestone* mile = new RoadmapMilestone(pro, id);
                bool delivered;
                in >> delivered; // Recupero lo stato
                mile->Delivered = delivered; // Imposto lo stato

                element = static_cast<RoadmapProjectElement*>(mile); // Imposto element
            }
            else
            {
                // Creo un Task con l'id recuperato
                RoadmapTask* task = new RoadmapTask(pro, id);
                int days;
                in >> days; // Recupero il numero di giorni
                task->Days = days; // Imposto i giorni

                element = static_cast<RoadmapProjectElement*>(task); // Imposto element
            }

            // Se non è stato possibile recuperare un tipo "coerente"
            if(element == nullptr)
                throw std::exception();

            // Ora che ho un elemento di riferimento
            element->Name = name; // Imposto il nome
            element->Date = date; // Imposto la data

            // Lo aggiungo al progetto
            pro->Elements.append(element);
        }
    }

    QMap<int, QList<int>> links;
    in >> links; // Recupero la lista dei links

    // Per ogni link
    for (int id : links.keys())
    {
        // Recupero l'elemento parent di riferimento
        RoadmapProjectElement* element = rmap.findElementById(id);

        // Se l'id non ha prodotto risultati
        if (element == nullptr)
            throw std::exception();

        //Per ogni figlio
        for (int sid : links.value(id))
        {
            // Recupero il riferimento al figlio
            RoadmapProjectElement* celement = rmap.findElementById(sid);

            // Se l'id non ha prodotto risultati
            if (celement == nullptr)
                throw std::exception();

            // aggiungo l'elemento al parent
            element->addChild(celement);
        }
    }

    // Ritorno lo stream
    return in;
}
