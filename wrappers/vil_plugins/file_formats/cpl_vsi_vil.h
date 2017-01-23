/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */
#ifndef CPL_VSI_VIL_H_
#define CPL_VSI_VIL_H_
#include <cpl_vsi_virtual.h>
#include <vil/vil_stream.h>

#include <string>
#include <vector>

void VSIInstallVILFileHandler(void);
void VSIRegisterVILStream(const char *pszFilename, vil_stream *stream);
void VSIUnRegisterVILStream(const char *pszFilename);

class VSIVILFilesystemHandler;

class VSIVILHandle : public VSIVirtualHandle
{
public:
  VSIVILHandle(vil_stream* stream);
  virtual ~VSIVILHandle(void);

  virtual int Seek(vsi_l_offset nOffset, int nWhence);
  virtual vsi_l_offset Tell();
  virtual size_t Read(void *pBuffer, size_t nSize, size_t nCount);
  virtual size_t Write(const void *pBuffer, size_t nSize, size_t nMemb);
  virtual int Eof();
  virtual int Close() { return 0; }

private:
  vil_streampos size;
  vil_stream* stream;

  friend class VSIVILFilesystemHandler;
};

class VSIVILFilesystemHandler : public VSIFilesystemHandler
{
public:
  VSIVILFilesystemHandler();
  virtual ~VSIVILFilesystemHandler();
  virtual VSIVirtualHandle *Open(const char *pszFilename,
    const char *pszAccess);
  virtual int Stat(const char *pszFilename, VSIStatBufL *pStatBuf, int nFlags);
  virtual int Unlink(const char *pszFilename);
  virtual int Mkdir(const char *pszDirname, long nMode);
  virtual int Rmdir(const char *pszDirname);
  virtual char **ReadDir(const char *pszDirname);
  virtual int Rename(const char *oldpath, const char *newpath);
  virtual int IsCaseSensitive(const char* pszFilename) { return 0; }

  void Insert(const char *pszFilename, vil_stream *stream);

private:
  void* mutex;
  struct VILDirTree;
  VILDirTree *tree;

  int Mkdir(const char *pszDirname, VILDirTree* &dir);;
  int Mkdir(const std::vector<std::string> &pathSplit, VILDirTree* &dir,
    size_t numIgnore = 0);
};
#endif
