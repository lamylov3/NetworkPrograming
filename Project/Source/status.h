/*
* define status 
* feature: 
*/

#ifndef __AUTHENTICATE_H__
#define __AUTHENTICATE_H__

typedef enum{
	USER_NOT_FOUND,
	USER_IS_BLOCKED,
	BLOCKED_USER,
	PASSWORD_INVALID,
	LOGIN_SUCCESS,
	USER_IN_LOGIN
} AuthenticateCode;