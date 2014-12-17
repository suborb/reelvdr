/*
 * svdrp.h: Simple Video Disk Recorder Protocol
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: svdrp.h 1.28 2006/08/06 08:51:09 kls Exp $
 */

#ifndef __SVDRP_H
#define __SVDRP_H

#include "recording.h"
#include "tools.h"
#include "thread.h"

class cSocket {
private:
  int port;
  int sock;
  int queue;
  void Close(void);
public:
  cSocket(int Port, int Queue = 1);
  ~cSocket();
  bool Open(void);
  int Accept(void);
  };

class cPUTEhandler {
private:
  FILE *f;
  int status;
  const char *message;
public:
  cPUTEhandler(void);
  ~cPUTEhandler();
  bool Process(const char *s);
  int Status(void) { return status; }
  const char *Message(void) { return message; }
  };

#define MAXCONS 64

#ifndef SVDRP_THREAD
class cSVDRP {
#else
class cSVDRP : public cThread {
#endif
private:
#ifdef SVDRP_THREAD
  bool OpenSock(int *sock);
  int AcceptSock(int *sock);
  int port;
  int listener;
  /* master file descriptor list */
  fd_set master;
  /* temp file descriptor list for select() */
  fd_set read_fds;
  /* maximum file descriptor number */
  int fdmax;
  int newConnection[MAXCONS];
  time_t lastActivity[MAXCONS];
#else
  cSocket socket;
  cFile file;
  time_t lastActivity;
#endif
  cPUTEhandler *PUTEhandler;
  bool inExecute;
  int numChars;
  int length;
  char *cmdLine;
  static char *grabImageDir;
  void Close(int Sock, bool SendReply = false, bool Timeout = false);
  bool Send(int Sock, const char *s, int length = -1);
  void Reply(int Sock, int Code, const char *fmt, ...) __attribute__ ((format (printf, 4, 5)));
  void PrintHelpTopics(int Sock, const char **hp);
  void CmdCHAN(int Sock, const char *Option);
  void CmdCLRE(int Sock, const char *Option);
  void CmdDELC(int Sock, const char *Option);
  void CmdDELR(int Sock, const char *Option);
  void CmdDELT(int Sock, const char *Option);
  void CmdEDIT(int Sock, const char *Option);
  void CmdGRAB(int Sock, const char *Option);
  void CmdHELP(int Sock, const char *Option);
  void CmdHITK(int Sock, const char *Option);
  void CmdLSTC(int Sock, const char *Option);
  void CmdLSTE(int Sock, const char *Option);
  void CmdLSTR(int Sock, const char *Option);
  void CmdLSTT(int Sock, const char *Option);
  void CmdMESG(int Sock, const char *Option);
  void CmdMODC(int Sock, const char *Option);
  void CmdMODT(int Sock, const char *Option);
  void CmdMOVC(int Sock, const char *Option);
  void CmdMOVT(int Sock, const char *Option);
  void CmdNEWC(int Sock, const char *Option);
  void CmdNEWT(int Sock, const char *Option);
  void CmdNEXT(int Sock, const char *Option);
  void CmdPLAY(int Sock, const char *Option);
  void CmdPLUG(int Sock, const char *Option);
  void CmdPUTE(int Sock, const char *Option);
  void CmdSCAN(int Sock, const char *Option);
  void CmdSTAT(int Sock, const char *Option);
  void CmdUPDT(int Sock, const char *Option);
  void CmdVOLU(int Sock, const char *Option);
  void Execute(char *Cmd);
  void Execute(char *Cmd, int Sock);
public:
  cSVDRP(int Port);
  ~cSVDRP();
#ifndef SVDRP_THREAD
  bool HasConnection(void) { return file.IsOpen(); }
  bool Process(void);
#else
  bool HasConnection(void) { return false; }
  virtual void Action(void);
#endif
  static void SetGrabImageDir(const char *GrabImageDir);
  };

#endif //__SVDRP_H
