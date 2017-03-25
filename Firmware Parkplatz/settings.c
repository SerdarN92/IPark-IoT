#include "settings.h"
#include "crc.h"
#include "hardware.h"
#include "eeprom.h"
#include "systimer.h"

/**
*	\brief EEPROM setting structure
*/
typedef struct {
	unsigned long networkAddress;									/**< Network address for device */
	unsigned char majorVersion;										/**< Major version of settings */
	unsigned char minorVersion;										/**< Minor version of settings */
	unsigned char revision;											/**< Revision of settings */
	unsigned short int setting_structLen;							/**< Internal length information of struct */
} MS_SETTINGS_SUPPL_t;

SETTING_t settings;

#define EEPROM_SETTINGS_ADDR	0x10

// Macht ein Update der Struktur und prüft gleichzeitig, ob die Einstellungen zurückgesetzt werden müssen:
unsigned char set_checkSettingChecksum(unsigned long int settingsCompatibleVersion, unsigned short int sizeOfSetting, void * settingStruct) {
	unsigned short int crc16;
	unsigned char * curr = (unsigned char*)settingStruct;
	MS_SETTINGS_SUPPL_t * settings = (MS_SETTINGS_SUPPL_t*)settingStruct;
	unsigned short int len;
	unsigned char crc_ok = 0;
	unsigned long int ltmp = 0;
	unsigned char testResult = 0;
	
	// Sicherstellen, dass diese Informationen so mit einbezogen werden..
	// Das MUSS hier eingetragen werden, damit die Prüfsummen übereinstimmen KÖNNEN:
	//SETTING_NAME.moduleID = (SETTING_NAME.moduleID & 0x7fffff) | (I_AM_DEVICE_TYPE << 20); // Modul-ID besteht aus allem außer dem obersten Bit!
	
	// Länge wird nicht mehr mit einberechnet:
	crc16 = 0xffff;
	
	len = settings->setting_structLen;
	
	// Maximal so lang wie die Struktur überhaupt ist (ohne CRC am Ende)
	if (len > (sizeOfSetting - 2)) {
		len = sizeOfSetting - 2;
	}
	
	// Bisherige Daten verarbeiten, Länge wie im EEPROM abgelegt.
	while (len--)
		crc16 = crc_calcCRC16r(crc16, *curr++);
	
	// Gucken, ob die CRC an dieser Stelle passt (evtl erst die halbe Struktur gelesen)
	if (crc16 == *((unsigned short int *)curr)) {
		// CRC passt
		crc_ok = 1;
	} else {
		crc_ok = 0;
	}
	
	if ((sizeOfSetting - 2) < settings->setting_structLen) {
		// Definitiv ungültig und nicht mehr zu übernehmen, die Struktur ist _kleiner_ geworden. Wir wissen nicht, welches Feld hier fehlt!
		crc_ok = 0;
		return 0;
//		crc16 = SETTING_NAME.crc16 - 1;					...ALT...
	} else {
		// Ist noch was übrig..?
		len = (sizeOfSetting - 2) - settings->setting_structLen; // Wie viele Bytes hat die neue Version mehr als die alte..?
		
		// Wir gucken mal, ob da noch was zu tun ist:
		while (len--)
			crc16 = crc_calcCRC16r(crc16, *curr++);
	}
	
	unsigned short int * structCrc = (unsigned short int*)(settingStruct + sizeOfSetting - 2);
	
	// Wenn die CRC jetzt stimmt UND die Länge identisch ist, ist alles in Ordnung:
	if ((crc16 == *structCrc) && 
		(sizeOfSetting == (settings->setting_structLen + 2))
		) {
		
		// Prüfen, ob wir einen Versionskonflikt haben:
		ltmp = (unsigned long int)settings->majorVersion << 16 | (unsigned long int)settings->minorVersion << 8 | settings->revision;
		if (ltmp < settingsCompatibleVersion) {
			//console_print_P(PSTR("Version incompatible 1. Reset!"));
			//while (1)
			//	wait(1000);
			testResult = 0;
		} else {
			//console_print_P(PSTR("SETTINGS OK"));
			//while (1)
			//	wait(1000);
			testResult = 1; // Alles okay, Daten sind valide
		}
	} else {
		// Auf die Anfangsposition gehen, an der die neuen Daten kommen müssen:
		
		// Mal gucken, ob die erste (halbe) Prüfung der Strukturdaten erfolgreich war und die Struktur gewachsen ist:
		if (crc_ok && ((sizeOfSetting - 2) > settings->setting_structLen)) {
			
			// Okay, dann versuchen wir jetzt ein Update der Struktur von Version x.y.z..
			// .. und setzen dafür erstmal den Rest der Struktur auf 0 (die neuen Variablen):
			curr = ((unsigned char*)settings) + settings->setting_structLen; // Ab hier müssen wir nullen:
			len = sizeOfSetting - settings->setting_structLen; // Und zwar für so viele Zeichen
			
			// Alles Nullen wenn wir in der Prüf-Phase sind:
			while (len--)
				*curr++ = 0;
			
			ltmp = (unsigned long int)settings->majorVersion << 16 | (unsigned long int)settings->minorVersion << 8 | settings->revision;
			
			// Wenn die Struktur grundlegend geändert wurde, muss SETTINGS_COMPATIBLE_VERSION auf die neue Versionsnummer gesetzt werden:
			if (ltmp < settingsCompatibleVersion) {
				//console_print_P(PSTR("Version incompatible 2. Reset!"));
				testResult = 0;
				crc_ok = 0;
			} else {
				//console_print_P(PSTR("Structural update OK"));
				testResult = 2; // Update vorgenommen
			}
			//while (1)
			//	wait(1000);
		} else {
			// Weder die eine noch die andere Prüfsumme sind korrekt. Komplett neu anfangen.
			//console_print_P(PSTR("RESET!!!"));
			//while (1)
			//	wait(1000);
			testResult = 0;
		}
	}
	return testResult;
}

void set_writeSettingChecksum(unsigned char MAJOR, unsigned char MINOR, unsigned char REVISION, unsigned int sizeOfSetting, void * settingStruct) {
	unsigned short int crc16;
	unsigned char * curr = (unsigned char*)settingStruct;
	unsigned short int len;
	MS_SETTINGS_SUPPL_t * settings = (MS_SETTINGS_SUPPL_t*)settingStruct;
	
	// Okay, die Struktur muss auf jeden Fall schonmal die aktuellen Daten enthalten:
	len = sizeOfSetting - 2; // Ohne Prüfsumme am Ende
	settings->setting_structLen = len;
	
	// Korrekte Daten in Struktur setzen:
	settings->majorVersion = MAJOR;
	settings->minorVersion = MINOR;
	settings->revision = REVISION;
	
	// Länge wird nicht mehr mit einberechnet:
	crc16 = 0xffff;
	
	// Bisherige Daten verarbeiten, Länge wie im EEPROM abgelegt.
	while (len--)
		crc16 = crc_calcCRC16r(crc16, *curr++);
	
	// Daten sind per Definition jetzt valide:
	unsigned short int * savedCRC = (unsigned short int*)(settingStruct + sizeOfSetting - 2);
	*savedCRC = crc16;
}

void set_saveSettings(void * p) {
	if (settings.modified) {
		// Das müssen wir zuerst machen, sonst speichern wir das Dirty-Flag als 1!
		settings.modified = 0;
		
		// Prüfsumme ergänzen:
		set_writeSettingChecksum(settings.majorVersion, settings.minorVersion, settings.revision, sizeof(SETTING_t), &settings);
		
		// Und ins EEPROM damit:
		if (ee_saveDataEx(EEPROM_SETTINGS_ADDR, &settings, sizeof(SETTING_t), 0)) {
			
			ON(LED3);
		} else {
			// Okay, nochmal versuchen. EEPROM hat gerade zu tun.
			settings.modified = 1;
		}
	}
}

void set_init() {
	ee_init();
	
	// Einstellungen aus dem EEPROM laden:
	ee_loadData(EEPROM_SETTINGS_ADDR, &settings, sizeof(SETTING_t));
	
	// .. und überprüfen:
	if (!set_checkSettingChecksum(1, sizeof(SETTING_t), &settings)) {
		unsigned char i;
		// Gespeicherte Einstellungen sind ungültig - auf Standardwerte zurücksetzen und gleich abspeichern:
		settings.majorVersion = 1;
		settings.minorVersion = 0;
		settings.revision = 0;
		settings.networkAddress = 0x01010101; // 1.1.1.1
		ON(LED2);
		i = 16;
		while (i--)
			settings.preSharedKey[i] = 0;
		settings.modified = 1;
	}
	// Make sure correct settings are saved when they have been modified:
	timer_addCallback(60000, 1, set_saveSettings, 0); // 60s interval
}
