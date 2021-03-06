#include <Arduino.h>
#include <EEPROM.h>

#include "Satellite.h"
#include "Objet.h"
#include "Detecteur.h"
#include "CANBus.h"

Detecteur::Detecteur() : Objet(DETECT)
{
	this->intervalle = 100;
	this->remanence = 1000;
	this->etatDetecte = HIGH;
	this->etatPrecedent = HIGH;
	this->precedentTest = 0;
	this->perteDetection = 0;
}

void Detecteur::begin(uint8_t inPin, uint8_t inNumber)
{
	this->pin = inPin;
	this->number = inNumber;
	pinMode(this->pin, INPUT_PULLUP);
	this->estDetecte = digitalRead(this->pin) == this->etatDetecte;
}

void Detecteur::loop(Satellite *inpSat)
{
	if (millis() - this->precedentTest < this->intervalle)
		return;

	this->precedentTest = millis();
	int etat = digitalRead(this->pin);        // Occupation=LOW, liberation=HIGH

	bool activ = etat == this->etatDetecte;

	if (activ && activ != this->estDetecte)
	{
		if (this->perteDetection == 0)
		{
			this->perteDetection = millis();
			return;
		}

		if (millis() - this->perteDetection < this->remanence)
			return;

		this->perteDetection = 0;
	}

	if (activ != this->etatPrecedent)
	{
		if (activ)
		{
			inpSat->Bus.StatusMessage.setDetection(inpSat->Bus.TxBuf, this->number, false);
			inpSat->Bus.messageTx();
			Serial.println(F("Lib"));
		}
		else
		{
			inpSat->Bus.StatusMessage.setDetection(inpSat->Bus.TxBuf, this->number, true);
			inpSat->Bus.messageTx();
			Serial.println(F("Occ"));
		}
		this->etatPrecedent = activ;
	}
	this->estDetecte = activ;
}

#ifdef DEBUG_MODE
void Detecteur::printObjet()
{
	Serial.print(F("Detecteur "));
	Objet::printObjet();
	Serial.print(F("/ intervalle: "));
	Serial.print(this->intervalle);
	Serial.print(F("/ remanence: "));
	Serial.print(this->remanence);
	Serial.print(F("/ etatDetecte: "));
	Serial.print(this->etatDetecte);
}
#endif

uint8_t Detecteur::EEPROM_sauvegarde(int inAddr)
{
	int addr = Objet::EEPROM_sauvegarde(inAddr);

	EEPROMPUT(addr, this->intervalle, sizeof(unsigned long));
	addr += sizeof(unsigned long);
	EEPROMPUT(addr, this->remanence, sizeof(unsigned long));
	addr += sizeof(unsigned long);
	EEPROMPUT(addr, this->etatDetecte, sizeof(byte));
	addr += sizeof(byte);

	return addr;
}

uint8_t Detecteur::EEPROM_chargement(int inAddr)
{
	int addr = Objet::EEPROM_chargement(inAddr);

	EEPROMGET(addr, this->intervalle, sizeof(unsigned long));
  Serial.print(F("dInt ")); Serial.print(this->intervalle); // 100
	addr += sizeof(unsigned long);
	EEPROMGET(addr, this->remanence, sizeof(unsigned long));
  Serial.print(F(" dRem ")); Serial.print(this->remanence); // 1000
	addr += sizeof(unsigned long);
	EEPROMGET(addr, this->etatDetecte, sizeof(byte));
  Serial.print(F(" dEta ")); Serial.println(this->etatDetecte); // 1
	addr += sizeof(byte);

	return addr;
}

