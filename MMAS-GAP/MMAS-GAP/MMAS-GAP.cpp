#include "stdafx.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <limits.h>
#include <random>
#include <iomanip>
#include <fstream>
#define PRESKOCI_NEISPLATIVA_RJESENJA 0
#define UKLJUCI_NEISPLATIVA_RJESENJA 1
#define NEHEURISTICNO 0
#define HEURISTICNO_DIJ 1
using namespace std;
int ukupanBrojIsplativih;
int ukupanBrojNeisplativih;
int *svaRjesenja;
int *svaIsplativaRjesenja;
int *najboljaRjesenja;
tm *paljenjePrograma;

int broj_mrava;
int* *d;//cijene
int* *r;//tezine
long double* *feromoni;
long double tauMAX;
long double tauMIN;
long double initTauMax;
long double initTauMin;
long double param_a;
int iter_max;
long double param_RO;

int brojAgenata;
int brojPoslova;
long double isplativoMult;
long double neisplativoMult;
int brojNeisplativihRjesenja = 0;
mt19937 gen(random_device{}());//generator brojeva
bool prekoNoci;

struct Dodjela {//struktura koja bilježi dodjelu posla agentu (možda se može matrica napraviti umjesto ovoga)
	int agent;
	int posao;
	//int cijena;
};

class Agent {
public:
	int radnoVrijeme;
	int ID;
}*agenti;

class Mrav {
private:
	//struktura posla samo sadrži opis posla, no kada pojedini mrav pokušava naæi rješenje potrebno mu je više informacija
	//sa kojima æe to realizirati pa unutar klase mrava postoji proširena struktura za agenta
	struct AgentZaRjesenje {
		Agent *agent;
		int preostaloVrijeme;
		AgentZaRjesenje() :
			agent(NULL) {
		};
	};

	//metoda miješa poslove (dakle simulira "nasumièni odabir")
	void PromjesajRedoslijedPoslova() {
		for (int i = 0, j, pom; i < brojPoslova - 1; i++) {
			j = rand() % brojPoslova;
			pom = redoslijedPoslova[i];
			redoslijedPoslova[i] = redoslijedPoslova[j];
			redoslijedPoslova[j] = pom;
		}
	}
	//metoda evidentira dodjelu, i smješta agenta udesno na njegovo mjesto (ovisno o preostalom kapacitetu)
	//ovo je najisplativiji naèin pomoæu kojega "tražimo" odnosno "dobivamo" skup agenata koji imaju kapaciteta za tekuæi posao
	void EvidentirajDodjelu(int iteracija, int ai, int p) {
		int a = agentiZaRjesenje[ai].agent->ID;
		rjesenje[iteracija].posao = p;
		rjesenje[iteracija].agent = a;
		
		agentiZaRjesenje[ai].preostaloVrijeme -= r[a][p];
		this->trosakRjesenja += d[a][p];
	}

	void PremjestiAgenta(int a) {
		//POSTAVI AGENTA NA NJEGOVO MJESTO (šaljemo ga desno jer mu se kapacitet smanjio, polje treba biti sortirano od onih sa najviše kapaciteta prema onima sa najmanje)
		if (a == brojAgenata - 1)
			return;//ako se veæ nalazi na zadnjem (najdesnijem) mjestu nema potrebe da ga šaljemo desno (ne bi ni mogli, algoritam premještanja bi se rušio)
		//vlastiti algoritam premještanja, radi po uzoru na sortiranje umetanjem
		AgentZaRjesenje pom = agentiZaRjesenje[a++];
		while (a < brojAgenata && agentiZaRjesenje[a].preostaloVrijeme > pom.preostaloVrijeme)
			agentiZaRjesenje[a - 1] = agentiZaRjesenje[a++];
		agentiZaRjesenje[a - 1] = pom;
		return;
	}

	int *redoslijedPoslova;
public:
	//potrebno je povezati polja nove proširene strukture sa 
	//poljima osnovne strukture (pomoæu pokazivaèa, moglo 
	//se možda i sa nasljeðivanjem al ovako se manje memorije zauzme)

	//static poslovi i agenti - da ne bi svaki put vezao redne brojeve i pokazivaèe i sl., napravim to jednom a kasnije samo kopiram po potrebi (memcpy)
	static AgentZaRjesenje *inicijalniAgenti;
	static void PostaviStatickaPolja() {
		inicijalniAgenti = new AgentZaRjesenje[brojAgenata];
		for (int i = 0; i < brojAgenata; i++) {
			inicijalniAgenti[i].agent = &agenti[i];
			inicijalniAgenti[i].preostaloVrijeme = inicijalniAgenti[i].agent->radnoVrijeme;
		}
	}

	Dodjela *rjesenje;//u polje dodjela bilježimo sve dodjele
	bool nesiplativoRjesenje;//indikator je li rješenje isplativo ili ne
	int trosakRjesenja;//trošak rješenja
	AgentZaRjesenje *agentiZaRjesenje;//polje za agente
	long double *granice;//polje za granice
	long double nasumicniBroj;


	void GenerirajRjesenje() {
		//priprema:
		this->nesiplativoRjesenje = false;//resetiramo vrijednosti
		this->trosakRjesenja = 0;

		memcpy(agentiZaRjesenje, this->inicijalniAgenti, sizeof(AgentZaRjesenje)*brojAgenata);//kopiramo agente iz statièkih polja
		PromjesajRedoslijedPoslova();
		
		//poèetak
		for (int i = 0, j=0, p; i < brojPoslova; i++) {
			
			//if (i == brojPoslova - 1){
			//}
			p = redoslijedPoslova[i];//p predstavlja posao
			j = 0;//j nam služi kao brojaè i za granicu agenata (skup agenata koji mogu obaviti posao je od 0 do "j")
			granice[j] = 0;//buduæi da kreiranje granica zahtjeva prethodne vrijednosti, na indeks "j" odnosno "0" postavljam
			while (j < brojAgenata && agentiZaRjesenje[j].preostaloVrijeme >= r[agentiZaRjesenje[j].agent->ID][p]) {
				granice[j + 1] = granice[j] + feromoni[agentiZaRjesenje[j].agent->ID][p];//raèunamo granicu za trenutnog agenta (granice se mogu direktno raèunati pa nema potrebe raèunati vjerojatnosti, osim ako bi negdje to trebalo bilježiti za kasniju analizu)
				j++;
			}
			if (j == 0) {//ukoliko se nijednom agentu ne može dodijeliti ovaj posao (jer nemaju kapaciteta):
				int randAgent = rand()%brojAgenata;
				EvidentirajDodjelu(i, randAgent, p);//onda evidentiramo dodjelu ali "nasumiènom" agentu
				PremjestiAgenta(randAgent);
				this->nesiplativoRjesenje = true;//te oznaèavamo rješenje kao neisplativo/nemoguæe
			}
			else{
				uniform_real_distribution<> distr(0, granice[j]);//postavljamo granice generiranja broja (od 0 do zadnje granice)
				nasumicniBroj = distr(gen);//generiramo nasumièni broj
			
				for (int ai = 0; ai < j; ai++) {//tražimo kojeg smo agenta dobili sa nasumiènim brojem
					if (nasumicniBroj <= granice[ai + 1]) {//granice[0] je prazno pa stoga gledamo k+1 od granica
						EvidentirajDodjelu(i, ai, p);//kada ga pronaðemo, dodjelimo mu posao
						PremjestiAgenta(ai);
						break;
					}
				}
			}
		}
		bool minus = false;
		for (int i = 0; i < brojAgenata; i++) {
			if (agentiZaRjesenje[i].preostaloVrijeme < 0)
				minus = true;
		}
		if (minus)
			ukupanBrojNeisplativih++;
		else
			ukupanBrojIsplativih++;
	}

	Mrav() {
		rjesenje = new Dodjela[brojPoslova];
		granice = new long double[brojAgenata + 1];//brojAgenata + 1, jer nam nultni indeks služi samo za zbrajanje
		agentiZaRjesenje = new AgentZaRjesenje[brojAgenata];
		redoslijedPoslova = new int[brojPoslova];
		for (int i = 0; i < brojPoslova; i++) {
			redoslijedPoslova[i] = i;
		}
	};

	~Mrav() {
		delete[] rjesenje;
		delete[] agentiZaRjesenje;
		delete[] granice;
		delete[] redoslijedPoslova;
	}
};

//deklaracija statièkih atributa (pokazivaèa) klase Mrav
Mrav::AgentZaRjesenje *Mrav::inicijalniAgenti;
Mrav *mravi;

void IspisiStanjeFeromona(int p = brojPoslova, int a = brojAgenata) {
	cout << fixed<<setprecision(6)<<endl;
	cout << "    ";
	for (int i = 0; i<p; i++)
		cout << "P" << i + 1 << "        ";
	cout << endl << endl;;
	for (int i = 0; i<a; i++) {
		cout << "A" << i + 1 << "  ";
		for (int j = 0; j<p; j++)
			cout << feromoni[i][j] << "  ";
		cout << endl;
	}
	cout << endl;
}

void SortirajAgentePoKapacitetu() {//sortira agente od onog sa najmanjim radnim vremenom prema onom sa najveæim radnim vremenom (bubble sort)
	for (int i = brojAgenata; i>0; i--) {
		for (int j = 1; j<i; j++) {
			if (agenti[j - 1].radnoVrijeme<agenti[j].radnoVrijeme) {
				Agent pom = agenti[j - 1];
				agenti[j - 1] = agenti[j];
				agenti[j] = pom;
			}
		}
	}
}

void SortirajAgentePoOznaci() {
	for (int i = brojAgenata; i>0; i--) {
		for (int j = 1; j<i; j++) {
			if (agenti[j - 1].ID>agenti[j].ID) {
				Agent pom = agenti[j - 1];
				agenti[j - 1] = agenti[j];
				agenti[j] = pom;
			}
		}
	}
}

int PronadiNajboljegMrava(int uvjetPretrage) {
	int max = 0;
	for (int i = 1; i < broj_mrava; i++) {
		if (uvjetPretrage == PRESKOCI_NEISPLATIVA_RJESENJA)
			if (mravi[i].nesiplativoRjesenje)
				continue;
		if (mravi[i].trosakRjesenja < mravi[max].trosakRjesenja) {
			max = i;
		}
	}
	return max;
}

void SimulirajHlapljenjeFeromona() {
	for (int i = 0; i < brojAgenata; i++) {
		for (int j = 0; j < brojPoslova; j++) {
			if (feromoni[i][j] * (1 - param_RO) < tauMIN)
				feromoni[i][j] = tauMIN;
			else
				feromoni[i][j] *= (1 - param_RO);
		}
	}
}

void NagradiNajboljeg(int najboljiMrav) {

	if (mravi[najboljiMrav].nesiplativoRjesenje) {
		brojNeisplativihRjesenja++;
	}

	Dodjela *rjesenje = mravi[najboljiMrav].rjesenje;
	long double nagrada = 1 / (long double)mravi[najboljiMrav].trosakRjesenja;

	for (int i = 0; i < brojPoslova; i++) {
		if (feromoni[rjesenje[i].agent][rjesenje[i].posao] + nagrada > tauMAX)
			feromoni[rjesenje[i].agent][rjesenje[i].posao] = tauMAX;
		else
			feromoni[rjesenje[i].agent][rjesenje[i].posao] += nagrada;
	}
}

Dodjela *najboljeRjesenjeDosad;
int najmanjiTrosakDosad;

char nazivDatotekeProblema[100];

void UcitajNazivDatoteke() {
	cout << "Naziv datoteke: ";
	cin.ignore();
	cin.getline(nazivDatotekeProblema, 100);
}

bool ProcitajJavniProblem(char *nazivDatoteke, bool prekoNoci = false) {
	char *putDatoteke = new char[1000];
	strcpy(putDatoteke, ".\\Instance problema\\");
	strcat(putDatoteke, nazivDatoteke);
	ifstream *ulaznaDatoteka = new ifstream(putDatoteke);
	if (!ulaznaDatoteka->is_open()) {
		return false;
	}
	char *broj = new char[10];
	*(ulaznaDatoteka) >> broj;
	brojAgenata = atoi(broj);
	*(ulaznaDatoteka) >> broj;
	brojPoslova = atoi(broj);
	d = new int*[brojAgenata];
	for (int i = 0; i < brojAgenata; i++) {
		d[i] = new int[brojPoslova];
		for (int j = 0; j < brojPoslova; j++) {
			*(ulaznaDatoteka) >> broj;
			d[i][j] = atoi(broj);
		}
	}

	int sumTezine = 0;
	r = new int*[brojAgenata];
	for (int i = 0; i < brojAgenata; i++) {
		r[i] = new int[brojPoslova];
		for (int j = 0; j < brojPoslova; j++) {
			*(ulaznaDatoteka) >> broj;
			r[i][j] = atoi(broj);
			sumTezine += r[i][j];
		}
	}
	int sumKapaciteta = 0;
	agenti = new Agent[brojAgenata];
	for(int i=0; i<brojAgenata; i++){
		*(ulaznaDatoteka) >> broj;
		agenti[i].radnoVrijeme = atoi(broj);
		sumKapaciteta += agenti[i].radnoVrijeme;
		agenti[i].ID = i;
	}

	cout << "Prosjek tezine: " << sumTezine / long double(brojPoslova*brojAgenata) << endl;
	cout << "Prosjek tezine po agentu: " << sumTezine / long double(brojPoslova/(long double)brojAgenata) << endl;
	cout << "Prosjek kapaciteta agenta: " << sumKapaciteta / long double(brojAgenata) << endl;
	ulaznaDatoteka->close();
	delete ulaznaDatoteka;
	delete[] broj;
	if(prekoNoci == false){
		system("pause");
	}
	return true;
}

int PronadiNajmanjuCijenu() {
	int min_i = 0, min_j = 0;
	for (int i = 0; i < brojAgenata; i++) {
		for (int j = 0; j < brojPoslova; j++) {
			if (d[i][j] < d[min_i][min_j]) {
				min_i = i;
				min_j = j;
			}
		}
	}
	return d[min_i][min_j];
}

void PostaviFeromoneNaTauMAX(int heuristika) {
	int mnozitelj = 1;
	if (heuristika == HEURISTICNO_DIJ)
		mnozitelj = PronadiNajmanjuCijenu();
	for (int i = 0; i < brojAgenata; i++) {
		for (int j = 0; j < brojPoslova; j++) {
			if(heuristika == HEURISTICNO_DIJ)
				feromoni[i][j] = tauMAX*(mnozitelj/(long double)(d[i][j]));//niže cijene dobiju veæu kolièinu feromona na poèetku od viših cijena
			else if(heuristika == NEHEURISTICNO)
				feromoni[i][j] = tauMAX;
		}
	}
	//cout << "RESETIRAO FEROMONE" << endl;
}

void init() {
	ukupanBrojIsplativih = 0;
	ukupanBrojNeisplativih = 0;

	memset(svaRjesenja, 0, 100000 * sizeof(int));
	memset(najboljaRjesenja, 0, 100000 * sizeof(int));
	memset(svaIsplativaRjesenja, 0, 100000 * sizeof(int));
	srand(time(0) % 32768);
	rand();
	brojNeisplativihRjesenja = 0;
	param_RO = 0.02;
	param_a = 50;
	neisplativoMult = 2;
	isplativoMult = 20;
	
	feromoni = new long double*[brojAgenata];//alociramo pokazivaèe za redove
	for (int i = 0; i<brojAgenata; i++) {
		feromoni[i] = new long double[brojPoslova];//alociramo redove pomoæu pokazivaèa
		for (int j = 0; j<brojPoslova; j++)//odmah prilikom alokacije
			feromoni[i][j] = 0.99;//iniciramo sve vrijednosti matrice na proizovljno visoku vrijednost (nisam siguran kako ovo treba)
	}
	//IspisiStanjeFeromona();


	broj_mrava = 650; //((brojAgenata+brojPoslova)/2)/6;//broj mravi aritmetièka sredina broja agenata i poslova podjeljena sa 6
	mravi = new Mrav[broj_mrava];
	SortirajAgentePoKapacitetu();//pomaže pri ubrzanju algoritma kasnije
	//N^k¢i (skup agenata kojima se može dodijeliti trenutni posao) je najlakše odrediti
	//ako je polje agenata sortirano prema preostalom kapacitetu, stoga se odmah nakon uèitavanja agenata to polje sortira

	Mrav::PostaviStatickaPolja();//nakon što su agenti sortirani, možemo postaviti statièka "inicijalna (proširena) polja"
	najboljeRjesenjeDosad = new Dodjela[brojPoslova];
	//poèetna iteracija (koja služi za postavljanje tauMAX i shodno tome tauMIN)
	for (int i = 0; i < broj_mrava; i++) {
		mravi[i].GenerirajRjesenje();
		int pom;
	}
	int najboljiMravDosad = PronadiNajboljegMrava(UKLJUCI_NEISPLATIVA_RJESENJA);
	memcpy(najboljeRjesenjeDosad, mravi[najboljiMravDosad].rjesenje, sizeof(Dodjela)*brojPoslova);
	najmanjiTrosakDosad = mravi[najboljiMravDosad].trosakRjesenja;

	int medu = param_RO * najmanjiTrosakDosad;
	tauMAX = 1 / (param_RO * najmanjiTrosakDosad);
	tauMIN = tauMAX / param_a;

	PostaviFeromoneNaTauMAX(HEURISTICNO_DIJ);

	initTauMax = tauMAX;
	initTauMin = tauMIN;
	cout << "NAJMANI TROSAK U INIT = " << najmanjiTrosakDosad << endl;
	cout << "tauMAX = " << tauMAX << endl;
	cout << "tauMIN = " << tauMIN << endl;
	cout << "param_RO = " << param_RO << endl;
	cout << "param_a = " << param_a << endl;

	//IspisiStanjeFeromona();
}

void SortirajRjesenjePremaAgentima() {
	Dodjela *NRD = najboljeRjesenjeDosad;
	for (int i = brojPoslova; i>0; i--) {
		for (int j = 1; j<i; j++) {
			if (NRD[j - 1].agent>NRD[j].agent) {
				Dodjela pom = NRD[j - 1];
				NRD[j - 1] = NRD[j];
				NRD[j] = pom;
			}
		}
	}
}

void IspisiRjesenjeProblema() {
	cout << "-----------------------------" << endl;
	cout << "ALGORITAM ZAVRŠEN" << endl;
	cout << "MATRICA FEROMONSKIH TRAGOVA:" << endl;
	//IspisiStanjeFeromona();

	cout << "tauMAX = " << tauMAX << endl;
	cout << "tauMIN = " << tauMIN << endl;
	cout << "param_RO = " << param_RO << endl;
	cout << "param_a = " << param_a << endl;

	SortirajRjesenjePremaAgentima();
	//for (int i = 0; i < brojPoslova; i++)
	//	cout << "A " << najboljeRjesenjeDosad[i].agent << "-> P" << najboljeRjesenjeDosad[i].posao << endl;
	cout << endl << "ISPIS RJEŠENJA: " << endl;
	for (int i = 0, k = 0; i < brojAgenata; i++) {
		cout << "AGENT " << i << " : ";
		while (najboljeRjesenjeDosad[k].agent == i) {
			cout << najboljeRjesenjeDosad[k++].posao << " ";
		}
		cout << endl;
	}

	cout << "Najniži trošak (rješenje) iznosi: " << najmanjiTrosakDosad << endl;
	cout << "-----------------------------" << endl;
}

int UkupniKapacitetAgenata() {
	int sum = 0;
	for (int i = 0; i < brojAgenata; i++) {
		sum += agenti[i].radnoVrijeme;
	}
	return sum;
}

char *UkloniEkstenziju(char *nazivDatoteke) {
	char *kopija = new char[strlen(nazivDatoteke) + 5];
	strcpy(kopija, nazivDatoteke);
	if(strlen(nazivDatoteke) >= 4 && nazivDatoteke[strlen(nazivDatoteke)-4] == '.')
		kopija[strlen(kopija) - 4] = 0;
	return kopija;
}

void ZapisiRjesenjeProblemaUDatoteku(time_t *pocetak, time_t *kraj) {

	struct tm * timeInfoPocetak = new tm;
	struct tm * timeInfoKraj = new tm;

	char bufferPocetak[80];
	char bufferKraj[80];

	localtime_s(timeInfoPocetak, pocetak);
	localtime_s(timeInfoKraj, kraj);

	strftime(bufferPocetak, 80, "%d.%m.%Y. %H:%M:%S", timeInfoPocetak);
	strftime(bufferKraj, 80, "%d.%m.%Y. %H:%M:%S", timeInfoKraj);

	int protekloVrijemeSekunde = difftime(*kraj, *pocetak);

	int sati = protekloVrijemeSekunde / 3600;
	int minute = (protekloVrijemeSekunde % 3600) / 60;
	int sekunde = protekloVrijemeSekunde % 60;

	char *rezultatRjesenja = new char[20];
	_itoa(najmanjiTrosakDosad, rezultatRjesenja, 10);

	char *nazivIzlazneDatoteke = new char[100];
	nazivIzlazneDatoteke[0] = 0;
	strcat(nazivIzlazneDatoteke, ".\\Instance rjesenja\\test ");
	char *pom = UkloniEkstenziju(nazivDatotekeProblema);
	strcat(nazivIzlazneDatoteke, pom);
  strcat(nazivIzlazneDatoteke, "  ");
	char buffer[30];
	strftime(buffer, 80, "%Y-%m-%d  %Hh %Mm %Ss", timeInfoPocetak);
	strcat(nazivIzlazneDatoteke, buffer);
	if(prekoNoci){
		char *brojIteracija = new char[50];
		_itoa(iter_max, brojIteracija, 10);
		strcat(nazivIzlazneDatoteke, " NOÆ");
		strcat(nazivIzlazneDatoteke, brojIteracija);
		char buff[15];
		strftime(buff, 15, "%M%S", paljenjePrograma);
		strcat(nazivIzlazneDatoteke, "_");
		strcat(nazivIzlazneDatoteke, buff);
		delete[] brojIteracija;
	}
	strcat(nazivIzlazneDatoteke, " (");
	strcat(nazivIzlazneDatoteke, rezultatRjesenja);
	strcat(nazivIzlazneDatoteke, ")");
	strcat(nazivIzlazneDatoteke, ".txt");
	

	ofstream izlaznaDatoteka(nazivIzlazneDatoteke);

	izlaznaDatoteka << fixed << setprecision(8);
	izlaznaDatoteka << "--------------------------------------------------" << endl;
	izlaznaDatoteka << "   FOI Završni rad, mentor dr.sc. N. Ivkoviæ" << endl;
	izlaznaDatoteka << "     MMAS-GAP Algoritam, Tomislav Èivèija" << endl;
	izlaznaDatoteka << "--------------------------------------------------" << endl;
	izlaznaDatoteka << "      Instanca rješenja problema \"" << pom << "\"" << endl;
	izlaznaDatoteka << "--------------------------------------------------" << endl;
	izlaznaDatoteka << "Kratki opis problema" << endl;
	izlaznaDatoteka << "BROJ AGENATA  " << brojAgenata << endl;
	izlaznaDatoteka << "BROJ POSLOVA  " << brojPoslova << endl;
	izlaznaDatoteka << "UKUPAN KAPACITET AGENATA  " << UkupniKapacitetAgenata() << endl;
	izlaznaDatoteka << "--------------------------------------------------" << endl;
	izlaznaDatoteka << "Vremenske odrednice instance" << endl;
	izlaznaDatoteka << "POÈETAK \t" << bufferPocetak << endl;
	izlaznaDatoteka << "ZAVRŠETAK\t" << bufferKraj << endl;
	izlaznaDatoteka << "TRAJANJE\t" << sati << ":" << minute << ":" << sekunde << endl;
	izlaznaDatoteka << "--------------------------------------------------" << endl;
	izlaznaDatoteka << "Parametri" << endl;
	izlaznaDatoteka << "param_RO  " << param_RO << endl;
	izlaznaDatoteka << "param_a  " << param_a << endl;
	izlaznaDatoteka << "iter_max  " << iter_max << endl;
	izlaznaDatoteka << "broj_mrava  " << broj_mrava << endl;
	izlaznaDatoteka << "isplativoMult  " << isplativoMult << endl;
	izlaznaDatoteka << "neisplativoMult  " << neisplativoMult << endl;
	izlaznaDatoteka << "--------------------------------------------------" << endl;
	izlaznaDatoteka << "Inicijalni korak" << endl;
	izlaznaDatoteka << "initTauMax  " << initTauMax << endl;
	izlaznaDatoteka << "initTauMin  " << initTauMin << endl;
	izlaznaDatoteka << "--------------------------------------------------" << endl;
	izlaznaDatoteka << endl;
	izlaznaDatoteka << "/////////////////////////////////////////////////" << endl;
	izlaznaDatoteka << "                    RJEŠENJE" << endl;
	izlaznaDatoteka << "/////////////////////////////////////////////////" << endl;
	izlaznaDatoteka << endl;
	izlaznaDatoteka << "--------------------------------------------------" << endl;
	izlaznaDatoteka << "Završni korak" << endl;
	izlaznaDatoteka << "tauMAX  " << tauMAX << endl;
	izlaznaDatoteka << "tauMIN  " << tauMIN << endl;
	izlaznaDatoteka << "brojNeisplativihRjesenja  " << brojNeisplativihRjesenja << endl;
	izlaznaDatoteka << "ukupanBrojNeisplativihRjesenja  " << ukupanBrojNeisplativih << endl;
	izlaznaDatoteka << "ukupanBrojIsplativihRjesenja  " << ukupanBrojIsplativih << endl;

	izlaznaDatoteka << "--------------------------------------------------" << endl;
	izlaznaDatoteka << "" << endl;

	izlaznaDatoteka << endl;


	SortirajRjesenjePremaAgentima();
	//for (int i = 0; i < brojPoslova; i++)
	//	cout << "A " << najboljeRjesenjeDosad[i].agent << "-> P" << najboljeRjesenjeDosad[i].posao << endl;
	izlaznaDatoteka << endl << "ISPIS DODJELA: " << endl;
	int sumaKapaciteta, agent;
	for (int i = 0, k = 0; i < brojAgenata; i++) {
		sumaKapaciteta = 0;

		//agent = najboljeRjesenjeDosad[k].agent;
		izlaznaDatoteka << "AGENT " << i << " : ";
		while (najboljeRjesenjeDosad[k].agent == i) {
			sumaKapaciteta += r[najboljeRjesenjeDosad[k].agent][najboljeRjesenjeDosad[k].posao];
			izlaznaDatoteka << "P" << najboljeRjesenjeDosad[k].posao;
			if (najboljeRjesenjeDosad[k].agent == najboljeRjesenjeDosad[k + 1].agent)
				izlaznaDatoteka << ", ";
			k++;
		}
		izlaznaDatoteka << "(" << sumaKapaciteta << "/" << agenti[i].radnoVrijeme << ")" << endl;
	}

	izlaznaDatoteka << "Najniži trošak (rješenje) iznosi: " << najmanjiTrosakDosad << endl;
	izlaznaDatoteka << "-----------------------------" << endl;

	izlaznaDatoteka << "matrica feromona (tau)" << endl;

	izlaznaDatoteka << "\t";
	for (int i = 0; i < brojPoslova; i++) {
		izlaznaDatoteka << "P" << i + 1 << "\t";
	}
	int brojVisokih = 0;
	izlaznaDatoteka << endl << endl;
	for (int i = 0; i < brojAgenata; i++) {
		izlaznaDatoteka << "A" << i + 1 << "\t";
		for (int j = 0; j < brojPoslova; j++) {
			if (tauMAX * 0.75 < feromoni[i][j])
				brojVisokih++;
			izlaznaDatoteka << feromoni[i][j] << "\t";
		}
		izlaznaDatoteka << endl << endl;
	}

	izlaznaDatoteka << "BROJ VISOKIH POSTOTAKA tauMAX/2 = " << brojVisokih  << endl;
	izlaznaDatoteka << "SVA RJESENJA" << endl;

	for (int i = 0; i < 100000; i++) {
		if (svaRjesenja[i] != 0)
			izlaznaDatoteka << " " << i << " " << svaRjesenja[i] << endl;
	}
	izlaznaDatoteka << "SVA ISPLATIVA RJESENJA:" << endl;
	for (int i = 0; i < 100000; i++) {
		if (svaIsplativaRjesenja[i] != 0)
			izlaznaDatoteka << " " << i << " " << svaIsplativaRjesenja[i] << endl;
	}
	izlaznaDatoteka.close();
	delete[] pom;

	if (prekoNoci == false) {
		char *pozivanjeNotepada = new char[1000];
		pozivanjeNotepada[0] = 0;
		strcat(pozivanjeNotepada, "notepad '");
		strcat(pozivanjeNotepada, nazivIzlazneDatoteke);
		strcat(pozivanjeNotepada, "'");
		system(pozivanjeNotepada);
	}
}

void PokreniAlgoritam(bool prekoNoci = false) {

	cout << fixed << setprecision(2);
	srand(time(0) % 32768);
	rand();
	//init();
	if (prekoNoci == false) {
		cout << "Koliko iteracija želite: ";
		cin >> iter_max;
	}
	//cout << "Napredak pretrage   0.00 %";
	time_t  pocetak;
	time(&pocetak);
	int najboljiMravIteracije;
	int frekvencija = (iter_max / 5) + 1;
	float deset_t = 0.01;
	float pocetni = deset_t;

	for (int iter = 1; iter < iter_max; iter++) {
		
		if (pocetni < (((float)iter) / iter_max) * 100) {
			//cout << (char)8 << (char)8 << (char)8 << (char)8 << (char)8 << (char)8;
			//if ((((float)iter) / iter_max) * 100 >= 10) cout << (char)8;
			cout << (char)13 << (((float)iter) / iter_max) * 100 << " %";
			pocetni += deset_t;
		}

		if (iter%frekvencija == 0) {//resetiranje feromona tri puta kroz algoritam (UÈINKOVITO)
			//param_a *= 1.15;
			//tauMIN = tauMAX / param_a;
			//PostaviFeromoneNaTauMAX();
			//cout << ": tau - MAX reset" << endl;
		}
		
		for (int m = 0; m<broj_mrava; m++) {
			mravi[m].GenerirajRjesenje();
			svaRjesenja[mravi[m].trosakRjesenja]++;
			if (!mravi[m].nesiplativoRjesenje)
				svaIsplativaRjesenja[mravi[m].trosakRjesenja]++;
		}

		najboljiMravIteracije = PronadiNajboljegMrava(UKLJUCI_NEISPLATIVA_RJESENJA);
		//cout << "Najbolje rješenje iteracije: " << mravi[najboljiMravIteracije].trosakRjesenja << endl;

		if (
			//!mravi[najboljiMravIteracije].nesiplativoRjesenje && 
			mravi[najboljiMravIteracije].trosakRjesenja < najmanjiTrosakDosad) {//ako je rješenje iteracije bolje od najboljeg dosada:

			memcpy(najboljeRjesenjeDosad, mravi[najboljiMravIteracije].rjesenje, sizeof(Dodjela)*brojPoslova);//zapisujemo to rješenje
			najmanjiTrosakDosad = mravi[najboljiMravIteracije].trosakRjesenja;//bilježimo nejgov rezultat
			tauMAX = 1 / (param_RO*najmanjiTrosakDosad);//podešavamo tauMAX i tauMIN
			tauMIN = tauMAX / param_a;
			cout << (char)13 << "NNR=" << najmanjiTrosakDosad << endl;
			//cout << "Imamo novo najbolje dosadašnje rješenje iteracije: " << najmanjiTrosakDosad << endl;
			//IspisiStanjeFeromona();
		}

		
		//cout << "/////////////PRIJE/////////////"<<endl;
		//IspisiStanjeFeromona(7,5);
		SimulirajHlapljenjeFeromona();
		NagradiNajboljeg(najboljiMravIteracije);
		//cout << "/////////////POSLIJE/////////////"<<endl;
		//IspisiStanjeFeromona(7,5);
		//system("pause");
		
	}
	cout << endl << setprecision(3);
	SortirajAgentePoOznaci();
	time_t kraj;
	time(&kraj);
	
	struct tm * timeInfoPocetak = new tm;
	struct tm * timeInfoKraj = new tm;
	char bufferPocetak[80];
	char bufferKraj[80];

	localtime_s(timeInfoPocetak, &pocetak);
	localtime_s(timeInfoKraj, &kraj);
	strftime(bufferPocetak, 80, "%c.", timeInfoPocetak);
	strftime(bufferKraj, 80, "%c.", timeInfoKraj);

	if (prekoNoci == false) {
		IspisiRjesenjeProblema();
		int protekloVrijemeSekunde = difftime(kraj, pocetak);
		cout << "Vrijeme pocetka:  " << bufferPocetak << endl;
		cout << "Vrijeme kraja  " << bufferKraj << endl;

		int sati = protekloVrijemeSekunde / 3600;
		int minute = (protekloVrijemeSekunde % 3600) / 60;
		int sekunde = protekloVrijemeSekunde % 60;

		cout << "Trajanje algoritma: " << sati << ":" << minute << ":" << sekunde << endl;
		cout << endl << endl;
	}

	ZapisiRjesenjeProblemaUDatoteku(&pocetak, &kraj);

	delete[] najboljeRjesenjeDosad;
	for (int i = 0; i < brojAgenata; i++) {
		delete[] feromoni[i];
	}
	delete[] feromoni;
	//delete[] poslovi;
	delete[] mravi;
	delete[] Mrav::inicijalniAgenti;
}

void RadiPrekoNoci() {
	cout << "Koliko iteracija zelite preko noci: ";
	cin >> iter_max;
	bool stanjeDatoteke;
	while (1) {
		stanjeDatoteke = ProcitajJavniProblem(nazivDatotekeProblema, true);
		if (stanjeDatoteke == false) {
			cout << "Pogreska pri otvaranju datoteke" << endl;
			break;
		}
		init();
		PokreniAlgoritam(true);
	}
}


void TestiranjeVjerojatnosti() {
	strcpy(nazivDatotekeProblema, "_test_vjerojatnosti.txt");
	bool stanjeDatoteke = ProcitajJavniProblem(nazivDatotekeProblema);
	if (stanjeDatoteke == false) {
		cout << "Pogreska pri otvaranju datoteke" << endl;
		return;
	}
	//
	feromoni = new long double*[brojAgenata];//alociramo pokazivaèe za redove
	for (int i = 0; i<brojAgenata; i++) {
		feromoni[i] = new long double[brojPoslova];//alociramo redove pomoæu pokazivaèa
		for (int j = 0; j<brojPoslova; j++)//odmah prilikom alokacije
			feromoni[i][j] = d[i][j];//iniciramo sve vrijednosti matrice na proizovljno visoku vrijednost (nisam siguran kako ovo treba)
	}
	//IspisiStanjeFeromona();


	broj_mrava = 10; //((brojAgenata+brojPoslova)/2)/6;//broj mravi aritmetièka sredina broja agenata i poslova podjeljena sa 6
	mravi = new Mrav[broj_mrava];
	SortirajAgentePoKapacitetu();//pomaže pri ubrzanju algoritma kasnije
															 //N^k¢i (skup agenata kojima se može dodijeliti trenutni posao) je najlakše odrediti
															 //ako je polje agenata sortirano prema preostalom kapacitetu, stoga se odmah nakon uèitavanja agenata to polje sortira

	Mrav::PostaviStatickaPolja();

	//
	int* *vjerojatnosti;
	vjerojatnosti = new int*[brojAgenata];
	for (int i = 0; i < brojAgenata; i++) {
		vjerojatnosti[i] = new int[brojPoslova];
		for (int j = 0; j < brojPoslova; j++) {
			vjerojatnosti[i][j] = 0;
		}
	}
	IspisiStanjeFeromona();
	cout << endl;
	system("pause");
	cout << endl;
	int iter;
	cout << "Broj iteracija: ";
	cin >> iter;
	for (int i = 0; i < iter; i++) {
		for (int j = 0; j < broj_mrava; j++) {
			mravi[j].GenerirajRjesenje();
			for (int k = 0; k < brojPoslova; k++) {
				vjerojatnosti[mravi[j].rjesenje[k].agent][mravi[j].rjesenje[k].posao]++;
			}
		}
	}
	IspisiStanjeFeromona();
	cout << "VJEROJATNOSTI NA KRAJU: "<<endl;
	cout << fixed << setprecision(6) << endl;
	cout << "    ";
	for (int i = 0; i<brojPoslova; i++)
		cout << "P" << i + 1 << "    ";
	cout << endl << endl;
	for (int i = 0; i<brojAgenata; i++) {
		cout << "A" << i + 1 << "  ";
		for (int j = 0; j<brojPoslova; j++)
			cout << vjerojatnosti[i][j] << "  ";
		cout << endl;
	}
	cout << endl;

	for (int i = 0; i < brojAgenata; i++) {
		delete[] feromoni[i];
	}
	delete[] feromoni;
	//delete[] poslovi;
	delete[] mravi;
	delete[] Mrav::inicijalniAgenti;
}

int main() {
	svaRjesenja = new int[100000];
	memset(svaRjesenja, 0, 100000 * sizeof(int));
	najboljaRjesenja = new int[100000];
	memset(najboljaRjesenja, 0, 100000 * sizeof(int));
	svaIsplativaRjesenja = new int[100000];
	memset(najboljaRjesenja, 0, 100000 * sizeof(int));
	paljenjePrograma = new tm;
	time_t pocetakPrograma;
	time(&pocetakPrograma);
	localtime_s(paljenjePrograma, &pocetakPrograma);
	int izbor, izborJavneDatoteke;
	bool stanjeDatoteke = true;
	do {
		cout << "0 Izlaz iz programa" << endl;
		cout << "1 Javni problem(C - tezina)" << endl;
		cout << "2 Obraduj preko noci" << endl;
		cout << "  Izbor: ";
		cin >> izbor;
		switch (izbor) {
		case 0: break;
		
		case 1:
			do{
				cout <<endl<<endl<< "Odabir problema"<<endl << endl;
				cout << "0 Povratak u glavni izbornik" << endl;
				cout << "1. c05100" << endl;
				cout << "2. c10100" << endl;
				cout << "3. c20100" << endl;
				cout << "4. c05200" << endl;
				cout << "5. c10200" << endl;
				cout << "6. c20200" << endl;
				cout << "7. Neki drugi problem" << endl;
				cout << " Izbor: ";
				cin >> izborJavneDatoteke;
				switch (izborJavneDatoteke) {
					case 0: break;
					case 1: 
						strcpy(nazivDatotekeProblema, "c05100"); break;
					case 2:
						strcpy(nazivDatotekeProblema, "c10100"); break;
					case 3:
						strcpy(nazivDatotekeProblema, "c20100"); break;
					case 4:
						strcpy(nazivDatotekeProblema, "c05200"); break;
					case 5:
						strcpy(nazivDatotekeProblema, "c10200"); break;
					case 6:
						strcpy(nazivDatotekeProblema, "c20200"); break;
					case 7:
						UcitajNazivDatoteke(); break;
					default:
						cout << "Pogresan unos!";
				}
			} while (izborJavneDatoteke < 0 && izborJavneDatoteke > 7);

			if (izborJavneDatoteke == 0) 
				break;
			else {
				stanjeDatoteke = ProcitajJavniProblem(nazivDatotekeProblema);
				if (stanjeDatoteke == false) {
					cout << "Pogreska pri otvaranju datoteke" << endl;
					break;
				}
				init();
				PokreniAlgoritam();
			}
			break;
		case 2:
			prekoNoci = true;
			UcitajNazivDatoteke();
			RadiPrekoNoci();
			break;
		}
		system("pause");
		system("cls");
	} while (izbor != 0);
	delete[] svaRjesenja;
	delete[] najboljaRjesenja;
	delete[] svaIsplativaRjesenja;
	return 0;
}

