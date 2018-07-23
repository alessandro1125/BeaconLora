# Localizzazione BLE

Questo sketch permette di utlizzare le [librerie esp32](https://github.com/Sauro98/esp32ArduinoLibraries) per ottenere la misura della distanza dei beacon presenti dell'area dal device che esegue questo codice. 

## Struttura del sistema

Il sistema è composto da:

* Un server centrale: raccoglie i dati dei dispositivi ed effettua operazioni su du essi

* Dei beacon: dispositivi che emettono ad intervalli regolari messaggi bluetooth di advertising

* Dei dispositivi ESP32: sono locati in posizioni strategiche della struttura in cui viene installato il sistema e raccolgono le informazioni sulla distanza dei beacon rilevati rispetto alla loro posizione.

Sono presenti 3 diversi tipi di dispositivi ESP32:

* Nodo
* Sensore
* Sensore autonomo

#### Nodo

Il nodo è un dipositivo connesso ad internet che ha il compito di raccogliere ed immettere in rete i messaggi dei sensori che non hanno la possibilità di essere connessi. Il nodo resta in perenne ascolto di messaggi LoRa dai sensori per poterli inoltrare al server

#### Sensore

Il sensore è un dispositivo che non ha la possibilità, per la sua posizione fisica, di essere connesso alla rete, quindi delega al nodo la responsabilità di inviare al server i dati raccolti. Solitamente è collegato all'esterno o in aree con forte attenuazione del segnale di rete.

Nodo e sensore comunicano tramite la tecnologia LoRa.

#### Sensore autonomo

Questo è un sensore che ha la possibilità di essere connesso alla rete wifi e quindi può inoltrare autonomamente i dati raccolti al server, senza la necessità di passare attraverso un nodo.

## Localizzazione bluetooth

I dispositivi utilizzano la tecnologia BLE (bluetooth low energy) per ricevere i pacchetti di advertising dai beacon ed utilizzano il valore di RSSI letto dal pacchetto per effettuare una stima della distanza. La formula per il calcolo della distanza è:

$$RSSI(d) = RSSI(d_0) - 10nlog_{10}\left(\frac{d}{d_0}\right)$$

Dove $RSSI(d)$ è il valore di RSSI letto dal pacchetto BLE, $RSSI(d_0)$ è il valore di RSSI che si leggerebbe se il beacon fosse alla distanza $d_0$, detta **distanza di riferimento**, che può essere scelta arbitrariamente, ed $n$ è un coefficiente che rappresenta il livello di rumore dell'ambiente in cui si svolge la misurazione.

Ponendo $d_0 = 1\text{m}$ e defienendo $RSSI(1\text{m}) = A$, la formula iniziale può essere ridotta a:
$$RSSI(d) = A - 10nlog_{10}(d)$$

Questa formula poi può essere invertita per ottenere la distanza:

$$ 
d = 10^{\dfrac{A-RSSI(d)}{10n}}$$

Per poter applicare questa dormula è necessario conoscere i valori di $A$ ed $n$. Tali valori possono essere ottenuti empiricamente se si utlizzano tre dispositivi capaci di emettere e ricevere pacchetti BLE:

1. Si posiziona un dispositivo, dal quale poi si leggeranno i risultati ottenuti, in un punto arbitrario e gli altri 2 a due distanze conosciute dal primo dispositivo, necessariamente diverse fra di loro.
2. Si misurano i valori di RSSI letti dal primo dispositivo alla ricezione dei pacchetti degli altri due dispositivi. Per avere un valore attendibile è bene raccogliere un campione significativo di misurazioni RSSI, per cercare di attenuare l'errore casuale sulla misurazione.
3. Dalle misurazioni RSSI ottenute si ricava la media, una per dispositivo, e si assume che quel dato sia il valore di RSSI che si riceverà dai beacon alla stessa distanza

Avendo ora disponibili due valori di distanza e due valori di RSSIè possibile risolvere il seguente sistema per ricavare le due incognite $A$ ed $n$:

Definendo $d_1$ la distanza a cui si trova il primo dispositivo, $d_2$ la distanza a cui si trova il secondo , $R_1$ il valore di RSSI dal primo e $R_2$ il valore di RSSI del secondo si ottiene:

$$ \begin{cases}
	R_1 = A - 10nlog_{10}d_1\\
	R_2 = A - 10nlog_{10}d_2
	\end{cases} $$

Che può essere risolto per $n$ ed $A$:

$$ 
	\begin{cases}
	n = \dfrac{R_1-R_2}{10log_{10}\frac{d_2}{d_1}}\\
	A = R_2 + \dfrac{R_1-R_2}{log_{10}\frac{d_2}{d_1}}log_{10}d2
	\end{cases} 
$$

Avendo ora trovato le due incognite è possibile utilizzarle per il calcolo della distanza dei beacon.

Per poter effettuare una stima attendibile della distanza a cui si trova un beacon occorre effettuare più misurazioni ed aggregarle per cercare di eliminare gli errori casuali nella lettura dei valori di RSSI ricevuti. Empiricamente si è osservato che la media delle misure può fornire un valore fuorviante perchè la media viene influenzata in misura sensibile dagli estremi del range di valori considerato. Per un oggetto statico un buon indicatore di quale sia il valore reale di RSSI è risultato essere la moda delle misurazioni effettuate ma, per un oggetto in movimento, la moda delle misurazioni perde di significato. Pertanto si propone l'utilizzo di una media calcolata scartando una percentuale di misurazioni agli estremi, ad esempio un 10%.

Al momento vengono aggregate 30 misurazioni consecutive per beacon che, con un periodo di trasmissione del beacon di un secondo, spazia un intervallo di 30 secondi.

## Realizzazione del sistema

[Inserire parte sulla struttura del database]

I dispositivi ESP32 sono programmati in modo da poter svolgere uno qualsiasi dei tre diversi ruoli descritti all'inizio. Il ruolo del dispositivo viene deciso dall'utente nel momento dell'accensione, a seconda delle proprie necessità e può essere cambiato in ogni momento riaccendendo il dispositivo.

Al momento dell'accensione infatti l'ESP32 genera una rete WiFi con come nome il proprio indirizzo mac, che l'utente può trovare visualizzato sul display. Una volta connessi a tale rete WiFi una pagina web verrà visualizzata automaticamente nel proprio browser. Se tale pagina non dovesse aprirsi in modo automatico vi si può accedere digitando nella barra degli indirizzi del browser l'indirizzo IP che viene visualizzato nel display del dispositivo (default 192.168.4.1). 

La pagina web che verrà visualizzata permette all'utente di configurare a piacimento il proprio dispositivo. Se è già stata effettuata una configurazione prima di questo accesso la pagina verrà automaticamente inizializzata con i parametri precedenti, che possono essere cambiati.








