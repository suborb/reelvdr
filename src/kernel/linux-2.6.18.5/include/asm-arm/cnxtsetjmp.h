/******************************************************************************/
/*                   CONEXANT PROPRIETARY AND CONFIDENTIAL                    */
/*                        SOFTWARE FILE/MODULE HEADER                         */
/*                 Copyright Conexant Systems Inc. 2007                       */
/*                                 Hyderabad                                  */
/*                            All Rights Reserved                             */
/******************************************************************************/
/*
 * Filename:        cnxtsetjmp.h
 *
 *
 * Description:     Prototypes for setjmp and longjmp
 *
 *
 * Author:          Kaushik Bar
 *
 ******************************************************************************/
/* $Id: cnxtsetjmp.h,v 1.1, 2007-06-05 07:00:24Z, Kaushik Bar$
 ******************************************************************************/
#ifndef _CNXTSETJMP_H_
#define _CNXTSETJMP_H_

typedef int __jmp_buf[64];

typedef struct __jmp_buf_tag    /* C++ doesn't like tagless structs.  */
{
   __jmp_buf __jmpbuf;         /* Calling environment.  */
} jmp_buf[1];

int _setjmp (jmp_buf __env);

void longjmp (jmp_buf __env, int __val);

#endif /* _CNXTSETJMP_H_ */

/*******************************************************************************
 * Modifications:
 * $Log:
 *
 ******************************************************************************/

