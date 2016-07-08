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
using namespace std;
 
int broj_mrava;
int* *d;
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

struct Dodjela {//struktura koja bilježi dodjelu posla agentu (možda se može matrica napraviti umjesto ovoga)
	int agent;
	int posao;
	//int cijena;
};

class Agent {
public:
	int radnoVrijeme;
	int *cijenePoslova;//alternativa matrici "D¢ij", svaki agent ima popis cijena za svaki posao... po potrebi se rješenje može napraviti i sa matricom
	int ID;
}*agenti;

class Posao {
public:
	int trajanje;
}*poslovi;

class Mrav {
private:
	//struktura posla samo sadrži opis posla, no kada pojedini mrav pokušava naæi rješenje potrebno mu je više informacija
	//sa kojima æe to realizirati pa unutar klase mrava postoji proširena struktura za posao
	struct PosaoZaRjesenje {
		Posao *posao;
		int rbrPosla;//buduæi da se polje "proširenih" poslova miješa (zbog nasumiènog redoslijeda) moramo pamtiti o kojem se poslu radi (drugi naèin bi bio da unutar same strukture "Posao" pamtimo redni broj posla)
		bool posaoDodjeljen;//ova bool varijabla indicira jel posaoDodjeljen - korišteno u prvim verzijama algoritma, sada je nepotrebno no možda zatreba
		PosaoZaRjesenje() :
			posao(NULL),
			posaoDodjeljen(false) {
		};
	};
	//isto vrijedi i za agenta
	struct AgentZaRjesenje {
		Agent *agent;
		int preostaloVrijeme;
		int rbrAgenta;
		AgentZaRjesenje() :
			agent(NULL) {
		};
	};
	/*
	int PronadiPosaoNajkracegTrajanja(PosaoZaRjesenje *posloviZaRjesenje) {
	int min = 0;
	//int vrij
	for (int i = 1; i<brojPoslova; i++) {
	if (!posloviZaRjesenje[i].posaoDodjeljen)
	if (poslovi[i].trajanje<poslovi[min].trajanje)
	min = i;
	}
	return min;
	}
	*/
	/*
	int DajNasumicniPosao(PosaoZaRjesenje *posloviZaRjesenje) {//metoda služi da se dobije "random" NEDODIJELJENI posao kojeg æemo dodijeliti negdje
	int ind = 0;
	do {
	ind = rand() % brojPoslova;
	}while (posloviZaRjesenje[ind].posaoDodjeljen);
	return ind;
	}
	*/
	//metoda miješa poslove (dakle simulira "nasumièni odabir")
	void PromjesajPoslove(PosaoZaRjesenje *posloviZaRjesenje) {
		int j;
		PosaoZaRjesenje pom;
		for (int i = 0; i<brojPoslova - 1; i++) {
			j = rand() % brojPoslova;
			pom = posloviZaRjesenje[i];
			posloviZaRjesenje[i] = posloviZaRjesenje[j];
			posloviZaRjesenje[j] = pom;
		}
	}
	//metoda evidentira dodjelu, i smješta agenta udesno na njegovo mjesto (ovisno o preostalom kapacitetu)
	//ovo je najisplativiji naèin pomoæu kojega "tražimo" odnosno "dobivamo" skup agenata koji imaju kapaciteta za tekuæi posao
	void EvidentirajDodjelu(PosaoZaRjesenje tekuciPosao, int iteracija, AgentZaRjesenje *agentiZaRjesenje, int odabraniAgent) {
		rjesenje[iteracija].posao = tekuciPosao.rbrPosla;
		rjesenje[iteracija].agent = agentiZaRjesenje[odabraniAgent].agent->ID;
		int k = odabraniAgent;
		agentiZaRjesenje[k].preostaloVrijeme -= tekuciPosao.posao->trajanje;
		this->trosakRjesenja += agentiZaRjesenje[k].agent->cijenePoslova[tekuciPosao.rbrPosla];
		//POSTAVI AGENTA NA NJEGOVO MJESTO (šaljemo ga desno jer mu se kapacitet smanjio, polje treba biti sortirano od onih sa najviše kapaciteta prema onima sa najmanje)
		if (k == brojAgenata - 1)
			return;//ako se veæ nalazi na zadnjem (najdesnijem) mjestu nema potrebe da ga šaljemo desno (ne bi ni mogli, algoritam premještanja bi se rušio)

						 //vlastiti algoritam premještanja, radi po uzoru na sortiranje umetanjem
		int t = k + 1;
		AgentZaRjesenje pom = agentiZaRjesenje[k];
		while (t < brojAgenata && agentiZaRjesenje[t].preostaloVrijeme > pom.preostaloVrijeme)
			agentiZaRjesenje[t - 1] = agentiZaRjesenje[t++];
		agentiZaRjesenje[t - 1] = pom;


		//ako imamo polje:
		//0.  1.  2.  3.  4.  5.
		//29  21  20  15  12  10  
		//29  21  11  15  12  10 //npr. agent na indeksu 2 dobije posao koji mu uzme 9 radnih sati pa mu ostane samo 11

		//PREMJEŠTANJE:
		//iter1
		//t = 2 + 1 = 3
		//pom = a[2] (11)
		//t<brojAgenata [3<6 = TRUE]
		//&& a[3] (15) > pom(11)  [15>11 = TRUE]
		//a[t-1] = a[t] -> a[2] = a[3]:
		//0.  1.  2.  3.  4.  5.
		//29  21  15  15  12  10

		//iter2
		//t++ -> t = 4
		//pom = a[2] (11) (pom ostaje isti kroz cijelo premještanje)
		//t<brojAgenata [4<6 = TRUE]
		//	&& a[4] (12) > pom(11)  [12>11 = TRUE]
		//a[t-1] = a[t] -> a[3] = a[4]:
		//0.  1.  2.  3.  4.  5.
		//29  21  15  12  12  10

		//iter3
		//t++ -> t = 5
		//pom = a[2] (11) (pom ostaje isti kroz cijelo premještanje)
		//t<brojAgenata [5<6 = TRUE]
		//	&& a[5] (10) > pom(11)  [10>11 = FALSE]
		//PREKID PETLJE
		//a[t-1] = pom -> a[4] = pom(11)
		//0.  1.  2.  3.  4.  5.
		//29  21  15  12  11  10
	}

public:
	//potrebno je povezati polja nove proširene strukture sa 
	//poljima osnovne strukture (pomoæu pokazivaèa, moglo 
	//se možda i sa nasljeðivanjem al ovako se manje memorije zauzme)

	//static poslovi i agenti - da ne bi svaki put vezao redne brojeve i pokazivaèe i sl., napravim to jednom a kasnije samo kopiram po potrebi (memcpy)
	static PosaoZaRjesenje *inicijalniPoslovi;
	static AgentZaRjesenje *inicijalniAgenti;
	static void PostaviStatickaPolja() {
		inicijalniPoslovi = new PosaoZaRjesenje[brojPoslova];
		for (int i = 0; i < brojPoslova; i++) {
			inicijalniPoslovi[i].posao = &poslovi[i];
			inicijalniPoslovi[i].rbrPosla = i;
		}

		inicijalniAgenti = new AgentZaRjesenje[brojAgenata];
		for (int i = 0; i < brojAgenata; i++) {
			inicijalniAgenti[i].agent = &agenti[i];
			inicijalniAgenti[i].rbrAgenta = agenti[i].ID;
			inicijalniAgenti[i].preostaloVrijeme = inicijalniAgenti[i].agent->radnoVrijeme;
		}
	}

	Dodjela *rjesenje;//u polje dodjela bilježimo sve dodjele
	bool nesiplativoRjesenje;//indikator je li rješenje isplativo ili ne
	int trosakRjesenja;//trošak rješenja
	PosaoZaRjesenje *posloviZaRjesenje;//polje za poslove
	AgentZaRjesenje *agentiZaRjesenje;//polje za agente
	long double *granice;//polje za granice

	/*
	funkcija raèuna sumu feromona ukoliko bude potrebno raèunati vjerojatnosti da im je zbroj = 1
	long double IzracunajSumuFeromona(int agent, bool *posaoDodjeljen) {
	long double suma = 0;
	for (int i = 0; i<brojPoslova; i++)
	if (posaoDodjeljen[i]) continue;
	else suma += feromoni[agent][i];
	return suma;
	}
	*/

	void GenerirajRjesenje() {
		//priprema:
		this->nesiplativoRjesenje = false;//resetiramo vrijednosti
		this->trosakRjesenja = 0;

		memcpy(posloviZaRjesenje, this->inicijalniPoslovi, sizeof(PosaoZaRjesenje)*brojPoslova);//kopiramo poslove iz statièkih polja
		PromjesajPoslove(posloviZaRjesenje);//miješamo poslove
		memcpy(agentiZaRjesenje, this->inicijalniAgenti, sizeof(AgentZaRjesenje)*brojAgenata);//kopiramo agente iz statièkih polja

		long double nasumicniBroj;
		mt19937 gen(random_device{}());//generator brojeva

											//poèetak
		for (int i = 0; i < brojPoslova; i++) {
			int j = 0;//j nam služi kao brojaè i za granicu agenata (skup agenata koji mogu obaviti posao je od 0 do "j")
			granice[j] = 0;//buduæi da kreiranje granica zahtjeva prethodne vrijednosti, na indeks "j" odnosno "0" postavljam
			while (j < brojAgenata && agentiZaRjesenje[j].preostaloVrijeme >= posloviZaRjesenje[i].posao->trajanje) {//sve dok nismo prošli kroz sve agente ili dok nismo došli do prvog koji ima nedovoljan kapacitet(buduæi da su uvijek sortirani samo do njega trebamo iæi)
				granice[j + 1] = granice[j] + feromoni[agentiZaRjesenje[j].agent->ID][i];//raèunamo granicu za trenutnog agenta (granice se mogu direktno raèunati pa nema potrebe raèunati vjerojatnosti, osim ako bi negdje to trebalo bilježiti za kasniju analizu)
				j++;
			}

			if (j == 0) {//ukoliko se nijednom agentu ne može dodijeliti ovaj posao (jer nemaju kapaciteta):
				EvidentirajDodjelu(posloviZaRjesenje[i], i, agentiZaRjesenje, rand() % brojAgenata);//onda evidentiramo dodjelu ali "nasumiènom" agentu
				this->nesiplativoRjesenje = true;//te oznaèavamo rješenje kao neisplativo/nemoguæe
				continue;
			}

			uniform_real_distribution<> distr(0, granice[j]);//postavljamo granice generiranja broja (od 0 do zadnje granice)
			nasumicniBroj = distr(gen);//generiramo nasumièni broj

			for (int k = 0; k < j; k++) {//tražimo kojeg smo agenta dobili sa nasumiènim brojem
				if (nasumicniBroj <= granice[k + 1]) {//granice[0] je prazno pa stoga gledamo k+1 od granica
					EvidentirajDodjelu(posloviZaRjesenje[i], i, agentiZaRjesenje, k);//kada ga pronaðemo, dodjelimo mu posao
					break;
				}
			}
		}
	}

	Mrav() {
		rjesenje = new Dodjela[brojPoslova];
		granice = new long double[brojAgenata + 1];//brojAgenata + 1, jer nam nultni indeks služi samo za zbrajanje
		posloviZaRjesenje = new PosaoZaRjesenje[brojPoslova];
		agentiZaRjesenje = new AgentZaRjesenje[brojAgenata];
	};

	~Mrav() {
		delete[] rjesenje;
		delete[] posloviZaRjesenje;
		delete[] agentiZaRjesenje;
		delete[] granice;
	}
};

Mrav::PosaoZaRjesenje *Mrav::inicijalniPoslovi;//deklaracija statièkih atributa (pokazivaèa) klase Mrav
Mrav::AgentZaRjesenje *Mrav::inicijalniAgenti;
Mrav *mravi;

void IspisiStanjeFeromona() {
	cout << endl;
	cout << "    ";
	for (int i = 0; i<brojPoslova; i++)
		cout << "P" << i + 1 << "     ";
	cout << endl << endl;;
	for (int i = 0; i<brojAgenata; i++) {
		cout << "A" << i + 1 << "  ";
		for (int j = 0; j<brojPoslova; j++)
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
	for (int i = 1; i < broj_mrava; i++){
		if(uvjetPretrage == UKLJUCI_NEISPLATIVA_RJESENJA)
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
	Dodjela *rjesenje = mravi[najboljiMrav].rjesenje;
	if (mravi[najboljiMrav].nesiplativoRjesenje) {
		for (int i = 0; i < brojPoslova; i++) {
			if (feromoni[rjesenje[i].agent][rjesenje[i].posao] + 0.01 > tauMAX)
				feromoni[rjesenje[i].agent][rjesenje[i].posao] = tauMAX;
			else
				feromoni[rjesenje[i].agent][rjesenje[i].posao] += 0.01;
		}
	}

	else {
		for (int i = 0; i < brojPoslova; i++) {
			if (feromoni[rjesenje[i].agent][rjesenje[i].posao] + 0.05 > tauMAX)
				feromoni[rjesenje[i].agent][rjesenje[i].posao] = tauMAX;
			else
				feromoni[rjesenje[i].agent][rjesenje[i].posao] += 0.05;
		}
	}
}

Dodjela *najboljeRjesenjeDosad;
int najmanjiTrosakDosad;


char nazivDatotekeProblema[100];

bool ProcitajProblemIzDatoteke() {
	cout << "Naziv datoteke: ";
	cin.ignore();
	cin.getline(nazivDatotekeProblema, 100);
	ifstream ulaznaDatoteka(nazivDatotekeProblema);
	if (!ulaznaDatoteka.is_open()) {
		return false;
	}
	char *linija = new char[10000];

	ulaznaDatoteka.getline(linija, 10000);
	if (strcmp(linija, "AGENTI") == 0) {
		ulaznaDatoteka.getline(linija, 10000);
		brojAgenata = atoi(linija);
		agenti = new Agent[brojAgenata];
		char broj[10];
		ulaznaDatoteka.getline(linija, 10000);
		for (int i = 0, j = 0, a = 0; i < strlen(linija); i++) {
			if (linija[i] != ',') {
				broj[j++] = linija[i];
			}
			else {
				broj[j] = 0;
				agenti[a].ID = a;
				agenti[a++].radnoVrijeme = atoi(broj);
				j = 0;
			}
		}
	}
	ulaznaDatoteka.getline(linija, 10000);
	if (strcmp(linija, "POSLOVI") == 0) {
		ulaznaDatoteka.getline(linija, 10000);
		brojPoslova = atoi(linija);
		poslovi = new Posao[brojPoslova];
		char broj[10];
		ulaznaDatoteka.getline(linija, 10000);
		for (int i = 0, j = 0, a = 0; i < strlen(linija); i++) {
			if (linija[i] != ',') {
				broj[j++] = linija[i];
			}
			else {
				broj[j] = 0;
				poslovi[a++].trajanje = atoi(broj);
				j = 0;
			}
		}
	}
	ulaznaDatoteka.getline(linija, 10000);
	if (strcmp(linija, "TROSKOVI") == 0) {
		for (int k = 0; k < brojAgenata; k++) {
			agenti[k].cijenePoslova = new int[brojPoslova];
			ulaznaDatoteka.getline(linija, 10000);
			char broj[10];
			for (int i = 0, j = 0, c = 0; i < strlen(linija); i++) {
				if (linija[i] != ',') {
					broj[j++] = linija[i];
				}
				else {
					broj[j] = 0;
					agenti[k].cijenePoslova[c++] = atoi(broj);
					j = 0;
				}
			}
		}
	}
	ulaznaDatoteka.close();
	return true;
}

void HardkodiraniProblemZaTestiranje() {
	brojAgenata = 5;
	agenti = new Agent[brojAgenata];
	agenti[0].radnoVrijeme = 45;
	agenti[1].radnoVrijeme = 33;
	agenti[2].radnoVrijeme = 21;
	agenti[3].radnoVrijeme = 16;
	agenti[4].radnoVrijeme = 17;

	brojPoslova = 10;
	poslovi = new Posao[brojPoslova];
	poslovi[0].trajanje = 6;
	poslovi[1].trajanje = 3;
	poslovi[2].trajanje = 4;
	poslovi[3].trajanje = 8;
	poslovi[4].trajanje = 9;
	poslovi[5].trajanje = 11;
	poslovi[6].trajanje = 1;
	poslovi[7].trajanje = 4;
	poslovi[8].trajanje = 3;
	poslovi[9].trajanje = 7;

	//postavljanje cijene (svaki agent ima cijene za sve poslove, isto kao što bi bilo prikazano u matrici)
	agenti[0].cijenePoslova = new int[brojPoslova];
	agenti[0].cijenePoslova[0] = 2;
	agenti[0].cijenePoslova[1] = 4;
	agenti[0].cijenePoslova[2] = 3;
	agenti[0].cijenePoslova[3] = 8;
	agenti[0].cijenePoslova[4] = 5;
	agenti[0].cijenePoslova[5] = 18;
	agenti[0].cijenePoslova[6] = 3;
	agenti[0].cijenePoslova[7] = 1;
	agenti[0].cijenePoslova[8] = 7;
	agenti[0].cijenePoslova[9] = 9;

	agenti[1].cijenePoslova = new int[brojPoslova];
	agenti[1].cijenePoslova[0] = 8;
	agenti[1].cijenePoslova[1] = 14;
	agenti[1].cijenePoslova[2] = 3;
	agenti[1].cijenePoslova[3] = 22;
	agenti[1].cijenePoslova[4] = 4;
	agenti[1].cijenePoslova[5] = 6;
	agenti[1].cijenePoslova[6] = 1;
	agenti[1].cijenePoslova[7] = 7;
	agenti[1].cijenePoslova[8] = 12;
	agenti[1].cijenePoslova[9] = 18;

	agenti[2].cijenePoslova = new int[brojPoslova];
	agenti[2].cijenePoslova[0] = 4;
	agenti[2].cijenePoslova[1] = 6;
	agenti[2].cijenePoslova[2] = 38;
	agenti[2].cijenePoslova[3] = 4;
	agenti[2].cijenePoslova[4] = 3;
	agenti[2].cijenePoslova[5] = 8;
	agenti[2].cijenePoslova[6] = 7;
	agenti[2].cijenePoslova[7] = 3;
	agenti[2].cijenePoslova[8] = 9;
	agenti[2].cijenePoslova[9] = 9;

	agenti[3].cijenePoslova = new int[brojPoslova];
	agenti[3].cijenePoslova[0] = 7;
	agenti[3].cijenePoslova[1] = 5;
	agenti[3].cijenePoslova[2] = 12;
	agenti[3].cijenePoslova[3] = 11;
	agenti[3].cijenePoslova[4] = 3;
	agenti[3].cijenePoslova[5] = 3;
	agenti[3].cijenePoslova[6] = 19;
	agenti[3].cijenePoslova[7] = 3;
	agenti[3].cijenePoslova[8] = 2;
	agenti[3].cijenePoslova[9] = 11;

	agenti[4].cijenePoslova = new int[brojPoslova];
	agenti[4].cijenePoslova[0] = 2;
	agenti[4].cijenePoslova[1] = 8;
	agenti[4].cijenePoslova[2] = 32;
	agenti[4].cijenePoslova[3] = 7;
	agenti[4].cijenePoslova[4] = 5;
	agenti[4].cijenePoslova[5] = 16;
	agenti[4].cijenePoslova[6] = 20;
	agenti[4].cijenePoslova[7] = 1;
	agenti[4].cijenePoslova[8] = 9;
	agenti[4].cijenePoslova[9] = 2;
}

void PostaviFeromoneNaTauMAX() {
	for (int i = 0; i < brojAgenata; i++) {
		for (int j = 0; j < brojPoslova; j++) {
			feromoni[i][j] = tauMAX;
		}
	}
	cout << "RESETIRAO FEROMONE" << endl;
}

void init() {
	//zasad je inicijalizacija hardkodirana, kasnije æu napraviti išèitavanje problema iz datoteke
	/*
	tauMAX = 0.7;
	tauMIN = 0.22;
	*/
	param_RO = 0.02;
	param_a = 5;
	//podaci za 5000 iteracija:
	//kada je param_a = 1  onda polovica instanci rješenja bude = 27
	//kada je param_a = 10 onda veæina instanci rješenja bude = 27
	//kada je param_a = 18 onda gotovo sve instance rješenja budu = 27
	/*
	
	*/

	feromoni = new long double*[brojAgenata];//alociramo pokazivaèe za redove
	for (int i = 0; i<brojAgenata; i++) {
		feromoni[i] = new long double[brojPoslova];//alociramo redove pomoæu pokazivaèa
		for (int j = 0; j<brojPoslova; j++)//odmah prilikom alokacije
			feromoni[i][j] = 0.99;//iniciramo sve vrijednosti matrice na proizovljno visoku vrijednost (nisam siguran kako ovo treba)
	}
	IspisiStanjeFeromona();


	broj_mrava = brojAgenata;//ZASADA, BROJ MRAVI OVISI O BROJU AGENATA
	mravi = new Mrav[broj_mrava];
	SortirajAgentePoKapacitetu();//pomaže pri ubrzanju algoritma kasnije
									 //N^k¢i (skup agenata kojima se može dodijeliti trenutni posao) je najlakše odrediti
									 //ako je polje agenata sortirano prema preostalom kapacitetu, stoga se odmah nakon uèitavanja agenata to polje sortira

	Mrav::PostaviStatickaPolja();//nakon što su agenti sortirani, možemo postaviti statièka "inicijalna (proširena) polja"
	najboljeRjesenjeDosad = new Dodjela[brojPoslova];
	//poèetna iteracija (koja služi za postavljanje tauMAX i shodno tome tauMIN)
	for (int i = 0; i < broj_mrava; i++) {
		mravi[i].GenerirajRjesenje();
	}
	int najboljiMravDosad = PronadiNajboljegMrava(PRESKOCI_NEISPLATIVA_RJESENJA);
	memcpy(najboljeRjesenjeDosad, mravi[najboljiMravDosad].rjesenje, sizeof(Dodjela)*brojPoslova);
	najmanjiTrosakDosad = mravi[najboljiMravDosad].trosakRjesenja;


	tauMAX = 1 / (param_RO * najmanjiTrosakDosad);
	tauMIN = tauMAX / param_a;

	PostaviFeromoneNaTauMAX();
	
	initTauMax = tauMAX;
	initTauMin = tauMIN;
	cout << "NAJMANI TROSAK U INIT = " << najmanjiTrosakDosad << endl;
	cout << "tauMAX = " << tauMAX << endl;
	cout << "tauMIN = " << tauMIN << endl;
	cout << "param_RO = " << param_RO << endl;
	cout << "param_a = " << param_a << endl;

	IspisiStanjeFeromona();
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
	IspisiStanjeFeromona();

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

//10-25
//%15 + 10
void GenerirajMatricuTroskova(ofstream *izlaznaDatoteka, int brojAgenata, int brojPoslova, int min, int max) {
	for (int i = 0; i < brojAgenata; i++) {
		for (int j = 0; j < brojPoslova; j++) {
			*izlaznaDatoteka << rand() % (max-min) + min << ",";
		}
		*izlaznaDatoteka << endl;
	}
}


void GeneriranjeProblema() {
	char nazivDatoteke[100];
	cout << "Naziv datoteke: ";
	cin.ignore();
	cin.getline(nazivDatoteke, 100);
	ofstream izlaznaDatoteka(nazivDatoteke);
	int brojAgenata, brojPoslova;
	cout << "Broj agenata: ";
	cin >> brojAgenata;
	cout << "Broj poslova";
	cin >> brojPoslova;
	int min, max;
	cout << "MIN za troskove: ";
	cin >> min;
	cout << "MAX za troskove";
	cin >> max;
	GenerirajMatricuTroskova(&izlaznaDatoteka, brojAgenata, brojPoslova, min, max);
	izlaznaDatoteka.close();

}

int UkupniKapacitetAgenata() {
	int sum = 0;
	for (int i = 0; i < brojAgenata; i++) {
		sum += agenti[i].radnoVrijeme;
	}
	return sum;
}

int UkupnaTezinaPoslova() {
	int sum = 0;
	for (int i = 0; i < brojPoslova; i++) {
		sum += poslovi[i].trajanje;
	}
	return sum;
}


char *UkloniEkstenziju(char *nazivDatoteke) {
	char *kopija = new char[strlen(nazivDatoteke)+5];
	int d = strlen(kopija);
	d = strlen(nazivDatoteke);
	//strcpy_s(kopija, nazivDatoteke);
	strcpy(kopija, nazivDatoteke);
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

	cout << "Vrijeme pocetka:  " << bufferPocetak << endl;
	cout << "Vrijeme kraja  " << bufferKraj << endl;

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
	strcat(nazivIzlazneDatoteke, " (");
	strcat(nazivIzlazneDatoteke, rezultatRjesenja);
	strcat(nazivIzlazneDatoteke, ")");
	strcat(nazivIzlazneDatoteke, ".txt");
	//strcat

	ofstream izlaznaDatoteka(nazivIzlazneDatoteke);

	izlaznaDatoteka << fixed << setprecision(4);
	izlaznaDatoteka << "--------------------------------------------------" << endl;
	izlaznaDatoteka << "   FOI Završni rad, mentor dr.sc. N. Ivkoviæ" << endl;
	izlaznaDatoteka << "     MMAS-GAP Algoritam, Tomislav Èivèija" << endl;
	izlaznaDatoteka << "--------------------------------------------------" << endl;
	izlaznaDatoteka << "      Instanca rješenja problema \"" << pom << "\""<< endl;
	izlaznaDatoteka << "--------------------------------------------------" << endl;
	izlaznaDatoteka << "Kratki opis problema" << endl;
	izlaznaDatoteka << "BROJ AGENATA  " << brojAgenata << endl;
	izlaznaDatoteka << "BROJ POSLOVA  " << brojPoslova << endl;
	izlaznaDatoteka << "UKUPAN KAPACITET AGENATA  " << UkupniKapacitetAgenata() <<endl;
	izlaznaDatoteka << "UKUPNA TEŽINA POSLOVA  " << UkupnaTezinaPoslova() <<endl;
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
	izlaznaDatoteka << "--------------------------------------------------" << endl;
	izlaznaDatoteka << "" << endl;

	izlaznaDatoteka << endl;






	//SortirajRjesenjePremaAgentima();
	//for (int i = 0; i < brojPoslova; i++)
	//	cout << "A " << najboljeRjesenjeDosad[i].agent << "-> P" << najboljeRjesenjeDosad[i].posao << endl;
	izlaznaDatoteka << endl << "ISPIS DODJELA: " << endl;
	int sumaKapaciteta, agent;
	for (int i = 0, k = 0; i < brojAgenata; i++) {
		sumaKapaciteta = 0;

		agent = najboljeRjesenjeDosad[k].agent;
		izlaznaDatoteka << "AGENT " << i << " : ";
		while (najboljeRjesenjeDosad[k].agent == i) {
			sumaKapaciteta += poslovi[najboljeRjesenjeDosad[k].posao].trajanje;
			izlaznaDatoteka << "P" <<najboljeRjesenjeDosad[k].posao;
			if (najboljeRjesenjeDosad[k].agent == najboljeRjesenjeDosad[k + 1].agent)
				izlaznaDatoteka << ", ";
			k++;
			//++].posao << " ";
		}
		izlaznaDatoteka << "("<< sumaKapaciteta<<"/"<<agenti[i].radnoVrijeme << ")"<< endl;
	}

	izlaznaDatoteka << "Najniži trošak (rješenje) iznosi: " << najmanjiTrosakDosad << endl;
	izlaznaDatoteka << "-----------------------------" << endl;

	izlaznaDatoteka << "matrica feromona (tau)" << endl;

	izlaznaDatoteka << "\t";
	for (int i = 0; i < brojPoslova; i++) {
		izlaznaDatoteka << "P" << i + 1 << "\t";
	}
	izlaznaDatoteka << endl << endl;
	for (int i = 0; i < brojAgenata; i++) {
		izlaznaDatoteka << "A" << i + 1 << "\t";
		for (int j = 0; j < brojPoslova; j++) {
			izlaznaDatoteka << feromoni[i][j] << "\t";
		}
		izlaznaDatoteka << endl << endl;
	}


	izlaznaDatoteka << "" << endl;

	izlaznaDatoteka.close();
	delete[] pom;
}

void PokreniAlgoritam() {

	cout << fixed << setprecision(3);
	srand(time(0) % 32768);
	rand();
	//init();
	cout << "Koliko iteracija želite: ";
	cin >> iter_max;

	time_t  pocetak;
	time(&pocetak);
	int najboljiMravIteracije;
	int frekvencija = iter_max / 7;
	for (int iter = 1; iter < iter_max; iter++) {
		if (iter%frekvencija == 0) {//resetiranje feromona tri puta kroz algoritam (UÈINKOVITO)
			PostaviFeromoneNaTauMAX();
		}
		for (int m = 0; m<broj_mrava; m++) {
			mravi[m].GenerirajRjesenje();
		}

		najboljiMravIteracije = PronadiNajboljegMrava(UKLJUCI_NEISPLATIVA_RJESENJA);
		//cout << "Najbolje rješenje iteracije: " << mravi[najboljiMravIteracije].trosakRjesenja << endl;
		
		if (!mravi[najboljiMravIteracije].nesiplativoRjesenje && mravi[najboljiMravIteracije].trosakRjesenja < najmanjiTrosakDosad) {//ako je rješenje iteracije bolje od najboljeg dosada:
			memcpy(najboljeRjesenjeDosad, mravi[najboljiMravIteracije].rjesenje, sizeof(Dodjela)*brojPoslova);//zapisujemo to rješenje
			najmanjiTrosakDosad = mravi[najboljiMravIteracije].trosakRjesenja;//bilježimo nejgov rezultat
			tauMAX = 1 / (param_RO*najmanjiTrosakDosad);//podešavamo tauMAX i tauMIN
			tauMIN = tauMAX /param_a;
			//cout << "Imamo novo najbolje dosadašnje rješenje iteracije: " << najmanjiTrosakDosad << endl;
			//IspisiStanjeFeromona();
		}
		SimulirajHlapljenjeFeromona();
		NagradiNajboljeg(najboljiMravIteracije);
	}
	SortirajAgentePoOznaci();
	time_t kraj;
	time(&kraj);

	IspisiRjesenjeProblema();

	struct tm * timeInfoPocetak = new tm;
	struct tm * timeInfoKraj = new tm;
	char bufferPocetak[80];
	char bufferKraj[80];

	localtime_s(timeInfoPocetak, &pocetak);
	localtime_s(timeInfoKraj, &kraj);
	strftime(bufferPocetak, 80, "%c.", timeInfoPocetak);
	strftime(bufferKraj, 80, "%c.", timeInfoKraj);

	int protekloVrijemeSekunde = difftime(kraj, pocetak);
	cout << "Vrijeme pocetka:  " << bufferPocetak << endl;
	cout << "Vrijeme kraja  " << bufferKraj << endl;

	int sati = protekloVrijemeSekunde / 3600;
	int minute = (protekloVrijemeSekunde % 3600) / 60;
	int sekunde = protekloVrijemeSekunde % 60;

	cout << "Trajanje algoritma: " << sati << ":" << minute << ":" << sekunde << endl;
	cout << endl << endl;


	ZapisiRjesenjeProblemaUDatoteku(&pocetak, &kraj);




	delete[] najboljeRjesenjeDosad;
	for (int i = 0; i < brojAgenata; i++) {
		delete[] agenti[i].cijenePoslova;
		delete[] feromoni[i];
	}
	delete[] feromoni;
	delete[] poslovi;
	delete[] mravi;
	delete[] Mrav::inicijalniAgenti;
	delete[] Mrav::inicijalniPoslovi;
}

int main() {
	int izbor;
	bool stanjeDatoteke = true;
	do {
		cout << "0 Izlaz iz programa" << endl;
		cout << "1 Gen. random matricu" << endl;
		cout << "2 Pokreni algoritam" << endl;
		cout << "  Izbor: ";
		cin >> izbor;
		switch (izbor) {
			case 0: break;
			case 1: GeneriranjeProblema(); break;
			case 2: 

				stanjeDatoteke = ProcitajProblemIzDatoteke();
				if (stanjeDatoteke == false) {
					cout << "Pogreska pri otvaranju datoteke" << endl;
					break;
				}
				//HardkodiraniProblemZaTestiranje();
				init();
				PokreniAlgoritam();
		}
		system("pause");
		system("cls");
	} while (izbor != 0);

	return 0;
}
