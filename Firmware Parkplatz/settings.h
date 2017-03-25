#ifndef SETTINGS_SUPPR
#define SETTINGS_SUPPR

/**
*	\brief Internal structure to store settings
*/
typedef struct {
	// Has to start with these five fields in exactly that order:
	unsigned long networkAddress;				/**< Network address */
	unsigned char majorVersion;					/**< Major version */
	unsigned char minorVersion;					/**< Minor version */
	unsigned char revision;						/**< Revision */
	unsigned short int setting_structLen;		/**< Internal length */
	// Off we go:
	unsigned char modified;						/**< Have the settings been modified? */
	unsigned char preSharedKey[16];				/**< Pre shared key */
	unsigned char maxKnownSequence[6]; 			/**< Maximum sequence number known. Sequence number space will be depleted after ~ 30991 years! (20 Byte/Message @ 5760 Byte/Sec w/ 6 byte SeqNum space) */
	
	
	// DASS HIER MUSS GANZ AM ENDE STEHEN. Wird nur intern verwendet um die CRC abzulegen.
	unsigned int crc16;							/**< CRC of settings in EEPROM */
} SETTING_t;

extern SETTING_t settings;

/**
*	\brief Check and correct checksum in settings
*	@param settingsCompatibleVersion Lowest compatible settings structure
*	@param sizeOfSettings Byte size of setting struct
*	@param settingStruct Pointer to setting structure
*	@returns	2: Settings have been updated to current version
*				1: Settings have been verified correctly
*				0: Settings have been reset
*/
unsigned char set_checkSettingChecksum(unsigned long int settingsCompatibleVersion, unsigned short int sizeOfSetting, void * settingStruct);

/**
*	\brief Write checksum to settings and settings to EEPROM
*	@param MAJOR Major version number
*	@param MINOR Minor version number
*	@param REVISION Revision version number
*	@param sizeOfSettings Byte size of setting struct
*	@param settingStruct Pointer to setting structure
*/
void set_writeSettingChecksum(unsigned char MAJOR, unsigned char MINOR, unsigned char REVISION, unsigned int sizeOfSetting, void * settingStruct);

/**
*	\brief Initialize and initially load settings:
*/
void set_init();

#endif
