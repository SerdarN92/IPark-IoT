
#ifndef _CALLBACK_DEFS_H_
#define _CALLBACK_DEFS_H_

typedef void (*Void_Function)( void );

typedef void (*Void_Function_Void)(void);
typedef void (*Void_Function_uChar)(unsigned char);
typedef void (*Void_Function_uInt)(unsigned int);
typedef void (*Void_Function_uInt_uInt)(unsigned int, unsigned int);
typedef void (*Void_Function_uInt_uInt_uChar)(unsigned int, unsigned int, unsigned char);


typedef unsigned char (*uChar_Function_Void)(void);
typedef unsigned char (*uChar_Function_uChar)(unsigned char);
typedef unsigned char (*uChar_Function_uChar_uInt)(unsigned char, unsigned int);
typedef unsigned char (*uChar_Function_uChar_uChar)(unsigned char, unsigned char);
typedef unsigned char (*uChar_Function_uInt)(unsigned int);

typedef unsigned int (*uInt_Function_Void)(void);
typedef unsigned int (*uInt_Function_uChar)(unsigned char);
typedef unsigned int (*uInt_Function_uInt)(unsigned int);


typedef void (*Void_Function_VoidP)(void*);
typedef void (*Void_Function_VoidP_uChar)(void*, unsigned char);
typedef void (*Void_Function_VoidP_uInt)(void*, unsigned int);
typedef void (*Void_Function_VoidP_uInt_uInt)(void*, unsigned int, unsigned int);
typedef void (*Void_Function_VoidP_uInt_uInt_uChar)(void*, unsigned int, unsigned int, unsigned char);
typedef void (*Void_Function_VoidP_uInt_uInt_uInt)(void*, unsigned int, unsigned int, unsigned int);
typedef void (*Void_Function_VoidP_uInt_uInt_uInt_uChar)(void*, unsigned int, unsigned int, unsigned int, unsigned char);


typedef unsigned char (*uChar_Function_VoidP)(void*);
typedef unsigned char (*uChar_Function_uCharP)(unsigned char *);
typedef unsigned char (*uChar_Function_VoidP_uChar)(void*, unsigned char);
typedef unsigned char (*uChar_Function_VoidP_uChar_uInt)(void*, unsigned char, unsigned int);
typedef unsigned char (*uChar_Function_VoidP_uInt)(void*, unsigned int);

typedef unsigned int (*uInt_Function_VoidP)(void*);
typedef unsigned int (*uInt_Function_VoidP_uChar)(void*, unsigned char);
typedef unsigned int (*uInt_Function_VoidP_uInt)(void*, unsigned int);



typedef unsigned char (*uChar_Function_VoidP_uInt_uChar)(void*, unsigned int, unsigned char);
typedef unsigned char (*uChar_Function_VoidP_uInt_uChar_uInt)(void*, unsigned int, unsigned char, unsigned int);
typedef unsigned char (*uChar_Function_VoidP_uInt_uInt)(void*, unsigned int, unsigned int);

typedef unsigned int (*uInt_Function_VoidP_uInt_uChar)(void*, unsigned int, unsigned char);
typedef unsigned int (*uInt_Function_VoidP_uInt_uInt)(void*, unsigned int, unsigned int);

#endif
