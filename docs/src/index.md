% Localizzazione BLE

Questo sketch permette di utlizzare le [librerie esp32](https://github.com/Sauro98/esp32ArduinoLibraries) per ottenere la misura della distanza dei beacon presenti dell'area dal device che esegue questo codice. 

**Tabella dei contenuti:**

* [Struttura del sistema](#struttura-del-sistema)
	* [Nodo](#nodo)
	* [Sensore](#sensore)
	* [Sensore autonomo](#sensore-autonomo)
* [Localizzazione bluetooth](#localizzazione-bluetooth)
* [Realizzazione del sistema](#realizzazione-del-sistema)
	* [Descrizione delle tre categorie di configurazione](#descrizione-delle-tre-categorie-di-configurazione)
	* [Inizializzazione dei dispositivi](#inizializzazione-dei-dispositivi)
	* [Fase operativa](#fase-operativa)

## Struttura del sistema

Il sistema &egrave; composto da:

* Un server centrale: raccoglie i dati dei dispositivi ed effettua operazioni su du essi

* Dei beacon: dispositivi che emettono ad intervalli regolari messaggi bluetooth di advertising

* Dei dispositivi ESP32: sono locati in posizioni strategiche della struttura in cui viene installato il sistema e raccolgono le informazioni sulla distanza dei beacon rilevati rispetto alla loro posizione.

Sono presenti 3 diversi tipi di dispositivi ESP32:

* Nodo
* Sensore
* Sensore autonomo

### Nodo

Il nodo &egrave; un dipositivo connesso ad internet che ha il compito di raccogliere ed immettere in rete i messaggi dei sensori che non hanno la possibilit&agrave; di essere connessi. Il nodo resta in perenne ascolto di messaggi LoRa dai sensori per poterli inoltrare al server

### Sensore

Il sensore &egrave; un dispositivo che non ha la possibilit&agrave;, per la sua posizione fisica, di essere connesso alla rete, quindi delega al nodo la responsabilit&agrave; di inviare al server i dati raccolti. Solitamente &egrave; collegato all'esterno o in aree con forte attenuazione del segnale di rete.

Nodo e sensore comunicano tramite la tecnologia LoRa.

### Sensore autonomo

Questo &egrave; un sensore che ha la possibilit&agrave; di essere connesso alla rete wifi e quindi pu&ograve; inoltrare autonomamente i dati raccolti al server, senza la necessit&agrave; di passare attraverso un nodo.

## Localizzazione bluetooth

I dispositivi utilizzano la tecnologia BLE (bluetooth low energy) per ricevere i pacchetti di advertising dai beacon ed utilizzano il valore di RSSI letto dal pacchetto per effettuare una stima della distanza. La formula per il calcolo della distanza &egrave;:

$$RSSI(d) = RSSI(d_0) - 10 \eta log_{10}\left(\frac{d}{d_0}\right)$$

Dove $RSSI(d)$ &egrave; il valore di RSSI letto dal pacchetto BLE, $RSSI(d_0)$ &egrave; il valore di RSSI che si leggerebbe se il beacon fosse alla distanza $d_0$, detta **distanza di riferimento**, che pu&ograve; essere scelta arbitrariamente, ed $\eta$ &egrave; un coefficiente che rappresenta il livello di rumore dell'ambiente in cui si svolge la misurazione.

Ponendo $d_0 = 1\text{m}$ e defienendo $RSSI(1\text{m}) = A$, la formula iniziale pu&ograve; essere ridotta a:
$$RSSI(d) = A - 10 \eta log_{10}(d)$$

Questa formula poi pu&ograve; essere invertita per ottenere la distanza:

$$ 
d = 10^{\dfrac{A-RSSI(d)}{10 \eta }}$$

***

Un metodo alternativo per calcolare i valori di $A$ ed $\eta$ consiste nel posizionare un dispositivo, dal quale si possono leggere i dati ricevuti, ad un metro da un beacon ed effettuare una rilevazione del valore di RSSI di un numero significativo di messaggi (es: 100) ed effettuarne una media. Il valore ottenuto pu&ograve; essere assegnato ad $A$.    
Per calcolare $\eta$ invece, occorre rifare lo stesso esperimento ma mettendo il beacon a distanze diverse (es: 2m, 3m, 4m, 5m) in modo da ottenere una media del valore di RSSI dei messaggi per ogni distanza. Occorre poi inserire, uno alla volta, i valori trovati nella formula inversa della distanza, cio&egrave;:
$$
	\eta = \dfrac{A - RSSI}{10 log_{10}(d)}
$$
dove $A$ &egrave; il valore calcolato nel primo esperimento, $RSSI$ &egrave; il valore di RSSI ottenuto per la distanza e $d$ &egrave; la distanza a cui si trovava il dilspositivo dal beacon.

Alla fine di questo processo si sono ottenuti tanti valori di $\eta$ tante quante sono le distanze da cui si &egrave; campionato il valore di RSSI e, se tutto &egrave; andato bene tali valori non dovrebbero essere molto diversi l'uno dall'altro. Il valore finale di $\eta$ &egrave; dato dalla media di tutti i valori trovati. 

Un metodo alternativo per calcolare i valori di $A$ ed $\eta$ utlizza tre dispositivi capaci di emettere e ricevere pacchetti BLE:

1. Si posiziona un dispositivo, dal quale poi si leggeranno i risultati ottenuti, in un punto arbitrario e gli altri 2 a due distanze conosciute dal primo dispositivo, necessariamente diverse fra di loro.
2. Si misurano i valori di RSSI letti dal primo dispositivo alla ricezione dei pacchetti degli altri due dispositivi. Per avere un valore attendibile &egrave; bene raccogliere un campione significativo di misurazioni RSSI, per cercare di attenuare l'errore casuale sulla misurazione.
3. Dalle misurazioni RSSI ottenute si ricava la media, una per dispositivo, e si assume che quel dato sia il valore di RSSI che si ricever&agrave; dai beacon alla stessa distanza

Avendo ora disponibili due valori di distanza e due valori di RSSI &egrave; possibile risolvere il seguente sistema per ricavare le due incognite $A$ ed $\eta$:

Definendo $d_1$ la distanza a cui si trova il primo dispositivo, $d_2$ la distanza a cui si trova il secondo , $R_1$ il valore di RSSI dal primo e $R_2$ il valore di RSSI del secondo si ottiene:

$$ \begin{cases}
	R_1 = A - 10 \eta log_{10}d_1\\
	R_2 = A - 10 \eta log_{10}d_2
	\end{cases} $$

Che pu&ograve; essere risolto per $\eta$ ed $A$:

$$ 
	\begin{cases}
	\eta = \dfrac{R_1-R_2}{10log_{10}\frac{d_2}{d_1}}\\
	A = R_2 + \dfrac{R_1-R_2}{log_{10}\frac{d_2}{d_1}}log_{10}d2
	\end{cases} 
$$

Avendo ora trovato le due incognite &egrave; possibile utilizzarle per il calcolo della distanza dei beacon.

***

Per poter effettuare una stima attendibile della distanza a cui si trova un beacon occorre effettuare pi&ugrave; misurazioni ed aggregarle per cercare di eliminare gli errori casuali nella lettura dei valori di RSSI ricevuti. Empiricamente si &egrave; osservato che la media delle misure pu&ograve; fornire un valore fuorviante perch&egrave; la media viene influenzata in misura sensibile dagli estremi del range di valori considerato. Per un oggetto statico un buon indicatore di quale sia il valore reale di RSSI &egrave; risultato essere la moda delle misurazioni effettuate ma, per un oggetto in movimento, la moda delle misurazioni perde di significato. Pertanto si propone l'utilizzo di una media calcolata scartando una percentuale di misurazioni agli estremi, ad esempio un 10%.

Al momento vengono aggregate 30 misurazioni consecutive per beacon che, con un periodo di trasmissione del beacon di un secondo, spazia un intervallo di 30 secondi.

## Realizzazione del sistema

[Inserire parte sulla struttura del database]

I dispositivi ESP32 sono programmati in modo da poter svolgere uno qualsiasi dei tre diversi ruoli descritti all'inizio. Il ruolo del dispositivo viene deciso dall'utente nel momento dell'accensione, a seconda delle proprie necessit&agrave; e pu&ograve; essere cambiato in ogni momento riaccendendo il dispositivo.

Al momento dell'accensione infatti l'ESP32 genera una rete WiFi con come nome il proprio indirizzo MAC, che l'utente pu&ograve; trovare mostrato sul display sotto il titolo `Indirizzo MAC`.    
Una volta connessi a tale rete WiFi una pagina web verr&agrave; aperta automaticamente nel proprio browser. Se tale pagina non dovesse aprirsi in modo automatico vi si pu&ograve; accedere digitando nella barra degli indirizzi del browser l'indirizzo IP che viene visualizzato nel display del dispositivo (default 192.168.4.1). 

La pagina web che verr&agrave; visualizzata permette all'utente di configurare a piacimento il proprio dispositivo. Se &egrave; gi&agrave; stata effettuata una inizializzazione prima di questo accesso la pagina verr&agrave; automaticamente riempita con i parametri precedenti, che possono essere cambiati.

La pagina di avvio cambia a seconda della funzione scelta per il dispositivo, mostrando o nascondendo una delle tre categorie di configurazioni:

* Configurazione della connessione ad internet
* Configurazione dei parametri di rilevazione bluetooth
* Configurazione della connessione LoRa

Se si sta configurando un nodo si avr&agrave; accesso alla prima categoria, in modo da poter configurare la connessione WiFi, la seconda non viene mostrata in quanto il nodo non deve eseguire scansioni bluetooth e la terza non viene mostrata perch&egrave; il nodo contiene gi&agrave; al suo interno le configurazioni necessarie.

Se si sta configurando un sensore saranno disponibili la seconda e la terza categoria perch&egrave; il sensore va posizionato in un luogo in cui non c'&egrave; copertura di rete.

Se si sta configurando un sensore autonomo saranno disponibili la prima e la seconda categoria perch&egrave; il sensore autonomo non utilizza la connessione LoRa.

### Descrizione delle tre categorie di configurazione

* **Configurazione della connessione ad internet:** consiste in due campi obbligatori che sono:
  
	* SSID: nome della rete wifi a cui ci si vuole collegare
	* Password: password della rete wifi specificata nel campo precedente     
  
	e in quattro campi facoltativi che compaiono se si toglie la spunta dalla casella `DHCP`:

	* IP: l'indirizzo ip statico che si vuole avere nella rete
	* Maschera: la maschera di rete
	* Gateway: l'indirizzo del gateway che si vuole utilizzare
	* DNS: l'indirizzo del DNS scelto 

	Se si lascia la spunta su `DHCP` questi parametri verranno impostati automaticamente.

* **Configurazione dei parametri di rilevazione bluetooth:** Anche questi sono due campi che diventano visibili solo selezionando la casella `Avanzate Bluetooth` nella parte inferiore dell pagina e sono:
  
	* Potenza alla distanza di riferimento: valore di RSSI misurato ad un metro di distanza dal beacon
	* Coefficiente di rumore: valore del rumore di disturbo presente nell'ambiente    
 
	 Questi due valori non sono obbligatori e cambiarli pu&ograve; comportare difetti nella misurazione della distanza. Per sapere come calcolarli consultare [Localizzazione Bluetooth](#localizzazione-bluetooth). Se dopo averli configurati si volesse tornare ad utilizzare i valori preimpostati occorre inserire `0` in entrambi i parametri.

* **Configurazione della connessione LoRa:** consiste in un campo che serve ad indicare a quale nodo il sensore dovr&agrave; fare riferimento:
  
	* Indirizzo del nodo: l'indirizzo MAC del nodo di riferimento, pu&ograve; essere letto dal display del nodo in questione sotto il titolo `MAC`


### Inizializzazione dei dispositivi

Una volta che sono stati scelti i parametri voluti per il dispositivo, premendo il pulsante `Invia`, questo tenter&agrave; di inizializzarsi utilizzando tali parametri.     

* **Nodo e sensore autonomo:** Entrambi proveranno a connettersi alla rete WiFi selezionata e sucessivamente a sincronizzare il loro orologio con quello del server. Queste operazioni possono portare a due casi di errore:
 
	* Se SSID o Password della eventuale rete WiFi a cui si vuole connettere dovessero essere sbagliati oppure se, per altri motivi, non fosse possibile per il dispositivo connettersi alla rete WiFi voluta esso mostrer&agrave; all'utente un messaggio che lo invita a premere il pulsante `Reset` per poter riaccedere alla pagina iniziale e rivedere i parametri inseriti.
	* Se la sincronizzazione dell'orologio con il server non dovesse andare a buon fine verr&agrave; visualizzato sul display un messaggio che informa l'utente della mancata riuscita dell'operazione e il dispositivo viene automaticamente riavviato dopo 10 secondi, a meno che non venga premuto il tasto reset prima dello scadere di tale intervallo.
* **Sensore autonomo e sensore:** Entrambi inizializzeranno il proprio modulo LoRa con il proprio indirizzo MAC ed il sensore registrer&agrave; l'indirizzo del nodo a cui dovr&agrave; poi inviare i propri dati. Questi passaggi non portano ad avere errori bloccanti quindi verranno sempre eseguiti correttamente, tranne nel caso in cui il chip LoRa fosse fisicamente danneggiato.

Nel momento in cui i dispositivi sono correttamente configurati essi salvano le configurazioni appena utilizzate nella loro memoria non volatile per poterle riutilizzare in avvii futuri.

Se un dispositivo dovesse essere riavviato dopo che ha salvato le proprie configurazioni e l'utente non dovesse essere presente per confermare tali dati tramite connessione WiFi, esso attender&agrave; cinque minuti, dopo i quali considerer&agrave; validi i dati che ha gi&agrave; presenti nella propria memoria e li utilizzer&agrave; per l'inizializzazione.

### Fase operativa
A configurazione completata i dispositivi iniziano la loro fase di funzionamento vero e proprio, che differisce in base al tipo scelto:

* **Nodo:** resta costantemente in ascolto sul canale LoRa di pacchetti dei sensori. Ogni volta  che ne riceve uno lo salva in una coda che viene inoltrata al server ad intervalli di 120 secondi. Se l'operazione di comunicazione della coda al server va a buon fine allora i dati inviati vengono eliminati, altrimenti i dati vengono conservati e verr&agrave; effettuato un ulteriore tentativo di invio allo scadere dell'intervallo successivo, fino a quando l'operazione avr&agrave; successo, in modo da evitare perdite di dati. Sul display del dispositivo viene mostrato:

	* Op. completata/ Conn. timeout/ Conn. fallita: stato dell'ultimo tentativo di connessione al server

* **Sensore:** resta costantemente in ascolto sul canale bluetooth di pacchetti inviati dai beacon. Per effettuare una stima pi&ugrave; precisa della distanza esso aggrega 30 scansioni dello stesso beacon in un unico dato che poi verr&agrave; inviato in LoRa all'indirizzo del nodo inserito durante la fase iniziale.

* **Sensore autonomo:** come il sensore anche esso rimane in ascolto dei pacchetti dei beacon ma, una volta ottenuto il dato aggregato, questo viene inserito in una coda interna che viene inviata al server con le stesse modalit&agrave; previste dal nodo. Anche in questo caso viene mostrato lo stato dell'ultimo tentativo di connessione al server.

Tutte le tipologie di dispositivo mostrano inoltre i seguenti messaggi sul display:

* Attivo: sta ad indicare che il dispositivo ha completato correttamente l'inizializzazzione

* RAM libera: la quantit&agrave; di RAM disponibile per il programma
* MAC: l'indirizzo MAC del dispositivo

Pu&ograve; capitare che il dispositivo in modo autonomo si disconnetta dalla rete WiFi. In tal caso verr&agrave; mostrato sul display lo stato della riconnessione, che avviene automaticamente nel momento in cui la disconnessione viene rilevata, prima che si possano tornare a vedere i dati sopra descritti.

Ogni volta che si vorr&agrave; cambiare le impostazioni del dipositivo sar&agrave; sufficiente premere il pulsante reset presente nel dispositivo, connettersi alla rete wifi creata e modificare i parametri voluti.













