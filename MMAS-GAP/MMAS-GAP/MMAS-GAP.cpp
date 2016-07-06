#include "stdafx.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <limits.h>
#include <random>
#include <iomanip>
using namespace std;

int M;
int* *d;
long double* *feromoni;
long double tauMAX;
long double tauMIN;
long double param_a;
int iter_max;
long double param_RO;

int brojAgenata;
int brojPoslova;

struct Dodjela {//struktura koja bilje�i dodjelu posla agentu (mo�da se mo�e matrica napraviti umjesto ovoga)
	int agent;
	int posao;
	//int cijena;
};

class Agent {
public:
	int radnoVrijeme;
	int *cijenePoslova;//alternativa matrici "D�ij", svaki agent ima popis cijena za svaki posao... po potrebi se rje�enje mo�e napraviti i sa matricom
}*agenti;

class Posao {
public:
	int trajanje;
}*poslovi;

class Mrav {
private:
	//struktura posla samo sadr�i opis posla, no kada pojedini mrav poku�ava na�i rje�enje potrebno mu je vi�e informacija
	//sa kojima �e to realizirati pa unutar klase mrava postoji pro�irena struktura za posao
	struct PosaoZaRjesenje {
		Posao *posao;
		int rbrPosla;//budu�i da se polje "pro�irenih" poslova mije�a (zbog nasumi�nog redoslijeda) moramo pamtiti o kojem se poslu radi (drugi na�in bi bio da unutar same strukture "Posao" pamtimo redni broj posla)
		bool posaoDodjeljen;//ova bool varijabla indicira jel posaoDodjeljen - kori�teno u prvim verzijama algoritma, sada je nepotrebno no mo�da zatreba
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
	int DajNasumicniPosao(PosaoZaRjesenje *posloviZaRjesenje) {//metoda slu�i da se dobije "random" NEDODIJELJENI posao kojeg �emo dodijeliti negdje
	int ind = 0;
	do {
	ind = rand() % brojPoslova;
	}while (posloviZaRjesenje[ind].posaoDodjeljen);
	return ind;
	}
	*/
	//metoda mije�a poslove (dakle simulira "nasumi�ni odabir")
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
	//metoda evidentira dodjelu, i smje�ta agenta udesno na njegovo mjesto (ovisno o preostalom kapacitetu)
	//ovo je najisplativiji na�in pomo�u kojega "tra�imo" odnosno "dobivamo" skup agenata koji imaju kapaciteta za teku�i posao
	void EvidentirajDodjelu(PosaoZaRjesenje tekuciPosao, int iteracija, AgentZaRjesenje *agentiZaRjesenje, int odabraniAgent) {
		rjesenje[iteracija].posao = tekuciPosao.rbrPosla;
		rjesenje[iteracija].agent = agentiZaRjesenje[odabraniAgent].rbrAgenta;
		int k = odabraniAgent;
		agentiZaRjesenje[k].preostaloVrijeme -= tekuciPosao.posao->trajanje;
		this->trosakRjesenja += agentiZaRjesenje[k].agent->cijenePoslova[tekuciPosao.rbrPosla];
		//POSTAVI AGENTA NA NJEGOVO MJESTO (�aljemo ga desno jer mu se kapacitet smanjio, polje treba biti sortirano od onih sa najvi�e kapaciteta prema onima sa najmanje)
		if (k == brojAgenata - 1)
			return;//ako se ve� nalazi na zadnjem (najdesnijem) mjestu nema potrebe da ga �aljemo desno (ne bi ni mogli, algoritam premje�tanja bi se ru�io)

						 //vlastiti algoritam premje�tanja, radi po uzoru na sortiranje umetanjem
		int t = k + 1;
		AgentZaRjesenje pom = agentiZaRjesenje[k];
		while (t < brojAgenata && agentiZaRjesenje[t].preostaloVrijeme > pom.preostaloVrijeme)
			agentiZaRjesenje[t - 1] = agentiZaRjesenje[t++];
		agentiZaRjesenje[t - 1] = pom;


		//ako imamo polje:
		//0.  1.  2.  3.  4.  5.
		//29  21  20  15  12  10  
		//29  21  11  15  12  10 //npr. agent na indeksu 2 dobije posao koji mu uzme 9 radnih sati pa mu ostane samo 11

		//PREMJE�TANJE:
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
		//pom = a[2] (11) (pom ostaje isti kroz cijelo premje�tanje)
		//t<brojAgenata [4<6 = TRUE]
		//	&& a[4] (12) > pom(11)  [12>11 = TRUE]
		//a[t-1] = a[t] -> a[3] = a[4]:
		//0.  1.  2.  3.  4.  5.
		//29  21  15  12  12  10

		//iter3
		//t++ -> t = 5
		//pom = a[2] (11) (pom ostaje isti kroz cijelo premje�tanje)
		//t<brojAgenata [5<6 = TRUE]
		//	&& a[5] (10) > pom(11)  [10>11 = FALSE]
		//PREKID PETLJE
		//a[t-1] = pom -> a[4] = pom(11)
		//0.  1.  2.  3.  4.  5.
		//29  21  15  12  11  10
	}

public:
	//potrebno je povezati polja nove pro�irene strukture sa 
	//poljima osnovne strukture (pomo�u pokaziva�a, moglo 
	//se mo�da i sa naslje�ivanjem al ovako se manje memorije zauzme)

	//static poslovi i agenti - da ne bi svaki put vezao redne brojeve i pokaziva�e i sl., napravim to jednom a kasnije samo kopiram po potrebi (memcpy)
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
			inicijalniAgenti[i].rbrAgenta = i;
			inicijalniAgenti[i].preostaloVrijeme = inicijalniAgenti[i].agent->radnoVrijeme;
		}
	}

	Dodjela *rjesenje;//u polje dodjela bilje�imo sve dodjele
	bool nesiplativoRjesenje;//indikator je li rje�enje isplativo ili ne
	int trosakRjesenja;//tro�ak rje�enja
	PosaoZaRjesenje *posloviZaRjesenje;//polje za poslove
	AgentZaRjesenje *agentiZaRjesenje;//polje za agente
	long double *granice;//polje za granice

											 /*
											 funkcija ra�una sumu feromona ukoliko bude potrebno ra�unati vjerojatnosti da im je zbroj = 1
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

		memcpy(posloviZaRjesenje, this->inicijalniPoslovi, sizeof(PosaoZaRjesenje)*brojPoslova);//kopiramo poslove iz stati�kih polja
		PromjesajPoslove(posloviZaRjesenje);//mije�amo poslove
		memcpy(agentiZaRjesenje, this->inicijalniAgenti, sizeof(AgentZaRjesenje)*brojAgenata);//kopiramo agente iz stati�kih polja

		long double nasumicniBroj;
		mt19937 gen(1701);//generator brojeva

											//po�etak
		for (int i = 0; i < brojPoslova; i++) {
			int j = 0;//j nam slu�i kao broja� i za granicu agenata (skup agenata koji mogu obaviti posao je od 0 do "j")
			granice[j] = 0;//budu�i da kreiranje granica zahtjeva prethodne vrijednosti, na indeks "j" odnosno "0" postavljam
			while (j < brojAgenata && agentiZaRjesenje[j].preostaloVrijeme >= posloviZaRjesenje[i].posao->trajanje) {//sve dok nismo pro�li kroz sve agente ili dok nismo do�li do prvog koji ima nedovoljan kapacitet(budu�i da su uvijek sortirani samo do njega trebamo i�i)
				granice[j + 1] = granice[j] + feromoni[agentiZaRjesenje[j].rbrAgenta][i];//ra�unamo granicu za trenutnog agenta (granice se mogu direktno ra�unati pa nema potrebe ra�unati vjerojatnosti, osim ako bi negdje to trebalo bilje�iti za kasniju analizu)
				j++;
			}

			if (j == 0) {//ukoliko se nijednom agentu ne mo�e dodijeliti ovaj posao (jer nemaju kapaciteta):
				EvidentirajDodjelu(posloviZaRjesenje[i], i, agentiZaRjesenje, rand() % brojAgenata);//onda evidentiramo dodjelu ali "nasumi�nom" agentu
				this->nesiplativoRjesenje = true;//te ozna�avamo rje�enje kao neisplativo/nemogu�e
			}

			uniform_real_distribution<> distr(0, granice[j]);//postavljamo granice generiranja broja (od 0 do zadnje granice)
			nasumicniBroj = distr(gen);//generiramo nasumi�ni broj

			for (int k = 0; k < j; k++) {//tra�imo kojeg smo agenta dobili sa nasumi�nim brojem
				if (nasumicniBroj <= granice[k + 1]) {//granice[0] je prazno pa stoga gledamo k+1 od granica
					EvidentirajDodjelu(posloviZaRjesenje[i], i, agentiZaRjesenje, k);//kada ga prona�emo, dodjelimo mu posao
					break;
				}
			}
		}
	}

	Mrav() {
		rjesenje = new Dodjela[brojPoslova];
		granice = new long double[brojAgenata + 1];//brojAgenata + 1, jer nam nultni indeks slu�i samo za zbrajanje
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

Mrav::PosaoZaRjesenje *Mrav::inicijalniPoslovi;//deklaracija stati�kih atributa (pokaziva�a) klase Mrav
Mrav::AgentZaRjesenje *Mrav::inicijalniAgenti;
Mrav *mravi;

void IspisiStanjeFeromona() {
	cout << endl;
	cout << "    ";
	for (int i = 0; i<brojPoslova; i++)
		cout << "P" << i + 1 << "   ";
	cout << endl << endl;;
	for (int i = 0; i<brojAgenata; i++) {
		cout << "A" << i + 1 << "  ";
		for (int j = 0; j<brojPoslova; j++)
			cout << feromoni[i][j] << "  ";
		cout << endl;
	}
	cout << endl;
}

void SortirajAgente() {//sortira agente od onog sa najmanjim radnim vremenom prema onom sa najve�im radnim vremenom (bubble sort)
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

int PronadiNajboljegMrava() {
	int max = 0;
	for (int i = 1; i<M; i++)
		if (mravi[i].trosakRjesenja < mravi[max].trosakRjesenja) {
			max = i;
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

void init() {
	//zasad je inicijalizacija hardkodirana, kasnije �u napraviti i��itavanje problema iz datoteke
	/*
	tauMAX = 0.7;
	tauMIN = 0.22;
	*/
	param_RO = 0.02;
	param_a = 1;

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

	//postavljanje cijene (svaki agent ima cijene za sve poslove, isto kao �to bi bilo prikazano u matrici)
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

	feromoni = new long double*[brojAgenata];//alociramo pokaziva�e za redove
	for (int i = 0; i<brojAgenata; i++) {
		feromoni[i] = new long double[brojPoslova];//alociramo redove pomo�u pokaziva�a
		for (int j = 0; j<brojPoslova; j++)//odmah prilikom alokacije
			feromoni[i][j] = 0.99;//iniciramo sve vrijednosti matrice na proizovljno visoku vrijednost (nisam siguran kako ovo treba)
	}
	IspisiStanjeFeromona();


	M = 15;
	mravi = new Mrav[M];

	SortirajAgente();//poma�e pri ubrzanju algoritma kasnije
									 //N^k�i (skup agenata kojima se mo�e dodijeliti trenutni posao) je najlak�e odrediti
									 //ako je polje agenata sortirano prema preostalom kapacitetu, stoga se odmah nakon u�itavanja agenata to polje sortira

	Mrav::PostaviStatickaPolja();//nakon �to su agenti sortirani, mo�emo postaviti stati�ka "inicijalna (pro�irena) polja"
	najboljeRjesenjeDosad = new Dodjela[brojPoslova];
	//po�etna iteracija (koja slu�i za postavljanje tauMAX i shodno tome tauMIN)
	for (int i = 0; i < M; i++) {
		mravi[i].GenerirajRjesenje();
	}
	int najboljiMravDosad = PronadiNajboljegMrava();
	memcpy(najboljeRjesenjeDosad, mravi[najboljiMravDosad].rjesenje, sizeof(Dodjela)*brojPoslova);
	najmanjiTrosakDosad = mravi[najboljiMravDosad].trosakRjesenja;

	tauMAX = 1 / (param_RO * najmanjiTrosakDosad);
	tauMIN = 1 / (tauMAX*param_a);
	IspisiStanjeFeromona();
	for (int i = 0; i < brojAgenata; i++) {
		for (int j = 0; j < brojPoslova; j++) {
			feromoni[i][j] = tauMAX;
		}
	}

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
	cout << "ALGORITAM ZAVR�EN" << endl;
	cout << "MATRICA FEROMONSKIH TRAGOVA:" << endl;
	IspisiStanjeFeromona();

	SortirajRjesenjePremaAgentima();
	//for (int i = 0; i < brojPoslova; i++)
	//	cout << "A " << najboljeRjesenjeDosad[i].agent << "-> P" << najboljeRjesenjeDosad[i].posao << endl;
	cout << endl << "ISPIS RJE�ENJA: " << endl;
	for (int i = 0, k = 0; i < brojAgenata; i++) {
		cout << "AGENT " << i << " : ";
		while (najboljeRjesenjeDosad[k].agent == i) {
			cout << najboljeRjesenjeDosad[k++].posao << " ";
		}
		cout << endl;
	}

	cout << "Najni�i tro�ak (rje�enje) iznosi: " << najmanjiTrosakDosad << endl;
	cout << "-----------------------------" << endl;
}

int main() {
	cout << fixed << setprecision(3);
	srand(time(0) % 32768);
	rand();
	init();
	cout << "Koliko iteracija �elite: ";
	cin >> iter_max;

	int najboljiMravIteracije;
	for (int iter = 1; iter<iter_max; iter++) {
		for (int m = 0; m<M; m++) {
			mravi[m].GenerirajRjesenje();
		}

		najboljiMravIteracije = PronadiNajboljegMrava();
		cout << "Najbolje rje�enje iteracije: " << mravi[najboljiMravIteracije].trosakRjesenja << endl;

		if (mravi[najboljiMravIteracije].trosakRjesenja < najmanjiTrosakDosad) {//ako je rje�enje iteracije bolje od najboljeg dosada:
			memcpy(najboljeRjesenjeDosad, mravi[najboljiMravIteracije].rjesenje, sizeof(Dodjela)*brojPoslova);//zapisujemo to rje�enje
			najmanjiTrosakDosad = mravi[najboljiMravIteracije].trosakRjesenja;//bilje�imo nejgov rezultat
			tauMAX = 1 / (param_RO*najmanjiTrosakDosad);//pode�avamo tauMAX i tauMIN
			tauMIN = 1 / (tauMAX*param_a);
			cout << "Imamo novo najbolje dosada�nje rje�enje iteracije: " << najmanjiTrosakDosad << endl;
			IspisiStanjeFeromona();
		}
		SimulirajHlapljenjeFeromona();
		NagradiNajboljeg(najboljiMravIteracije);
	}
	IspisiRjesenjeProblema();
	cout << endl << endl;

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
	system("pause");
	return 0;
}
