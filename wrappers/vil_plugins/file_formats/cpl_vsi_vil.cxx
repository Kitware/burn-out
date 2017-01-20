/*ckwg +5
 * Copyright 2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */
#include <cstring>

#include <string>
#include <map>
#include <vector>
#include <utility>

#include <gdal.h>

#include <cpl_vsi_virtual.h>
#include <cpl_multiproc.h>
#include <vil/vil_stream.h>

#include "cpl_vsi_vil.h"


//*********************************************************************
// Register the VIL classes with GDAL
//*********************************************************************

void VSIInstallVILFileHandler(void)
{
  VSIFileManager::InstallHandler("/vsivil/", new VSIVILFilesystemHandler);
}


//*********************************************************************
// Insert the vil_stream into the virtual directory tree
//*********************************************************************
void VSIRegisterVILStream(const char *pszFilename, vil_stream *stream)
{
  if(VSIFileManager::GetHandler("") == VSIFileManager::GetHandler("/vsivil/"))
  {
    VSIInstallMemFileHandler();
  }

  VSIVILFilesystemHandler *poHandler = static_cast<VSIVILFilesystemHandler *>(
    VSIFileManager::GetHandler("/vsivil/"));

  poHandler->Insert(pszFilename, stream);
}

void VSIUnRegisterVILStream(const char *pszFilename)
{
  if(VSIFileManager::GetHandler("") == VSIFileManager::GetHandler("/vsivil/"))
  {
    VSIInstallMemFileHandler();
  }

  VSIVILFilesystemHandler *poHandler = static_cast<VSIVILFilesystemHandler *>(
    VSIFileManager::GetHandler("/vsivil/"));

  poHandler->Unlink(pszFilename);
}

//*********************************************************************
// Wrap the basic stream operations
//*********************************************************************

VSIVILHandle::VSIVILHandle(vil_stream* stream)
  : VSIVirtualHandle(), stream(stream), size(stream->file_size())
{
  this->stream->ref();
  this->stream->seek(0);
}

VSIVILHandle::~VSIVILHandle(void)
{
  this->stream->unref();
}

int VSIVILHandle::Seek(vsi_l_offset nOffset, int nWhence)
{
  vil_streampos posCur = this->stream->tell();
  vil_streampos posNext;

  switch(nWhence)
  {
  case SEEK_CUR: posNext = this->stream->tell() + nOffset;  break;
  case SEEK_SET: posNext = nOffset;  break;
  case SEEK_END: posNext = this->size + nOffset;  break;
  default: return -1;
  }
  if(posNext < 0 || posNext > this->size)
  {
    return -1;
  }
  this->stream->seek(posNext);
  return 0;
}

vsi_l_offset VSIVILHandle::Tell()
{
  return this->stream->tell();
}

size_t VSIVILHandle::Read( void *pBuffer, size_t nSize, size_t nCount)
{
  return this->stream->read(pBuffer, nSize*nCount);
}

size_t VSIVILHandle::Write(const void *pBuffer, size_t nSize, size_t nCount)
{
  return this->stream->write(pBuffer, nSize*nCount);
}

int VSIVILHandle::Eof()
{
  return this->stream->tell() == this->size ? 0 : 1;
}


//*********************************************************************
// Split a path into it's comonents
//*********************************************************************
void tokenize(const std::string& str, std::vector<std::string> &tok, char d='/')
{
  tok.clear();
  if(str.empty())
  {
    return;
  }

  size_t posPrev;
  for(posPrev = -1; str[posPrev+1] == d; ++posPrev); // Ignore leading delims
  size_t posCur;
  while((posCur = str.find(d, posPrev+1)) != std::string::npos)
  {
    if(posCur - posPrev > 1) // Only acknowledge non-empty path components
    {
      tok.push_back(str.substr(posPrev+1, posCur-posPrev-1));
    }
    posPrev = posCur;
  }

  if(posPrev != str.size()-1) // Only add teh last component if it's non-empty
  {
    tok.push_back(str.substr(posPrev+1, str.size()-posPrev-1));
  }
}


//*********************************************************************
// Directory tree used for the virtual filesystem
//*********************************************************************
struct VSIVILFilesystemHandler::VILDirTree
{
  std::map<std::string, vil_stream*> files;
  std::map<std::string, VILDirTree*> subDirs;

  ~VILDirTree(void)
  {
    for(std::map<std::string, VILDirTree*>::iterator i = this->subDirs.begin();
        i != this->subDirs.end(); ++i)
    {
      delete i->second;
    }
  }

  VILDirTree* GetBaseDir(const std::vector<std::string>& path)
  {
    VILDirTree *curDir = this;
    for(size_t i = 0; i < path.size()-1; ++i)
    {
      std::map<std::string, VILDirTree*>::iterator d =
        curDir->subDirs.find(path[i]);
      if(d == curDir->subDirs.end())
      {
        return NULL;
      }
      curDir = d->second;
    }
    return curDir;
  }
};

//*********************************************************************
// Create a virtual filesystem to handle the vil_stream objects
//*********************************************************************
VSIVILFilesystemHandler::VSIVILFilesystemHandler()
  : mutex(NULL), tree(new VILDirTree)
{ }

VSIVILFilesystemHandler::~VSIVILFilesystemHandler()
{
  delete this->tree;
  if(this->mutex)
  {
    CPLDestroyMutex(this->mutex);
    this->mutex = NULL;
  }
}

VSIVirtualHandle *VSIVILFilesystemHandler::Open(const char *pszFilename,
                                                const char *pszAccess)
{
  std::vector<std::string> pathSplit;
  tokenize(pszFilename, pathSplit, '/');

  {
    CPLMutexHolder mtxHolder(&this->mutex);
    VILDirTree *dir = this->tree->GetBaseDir(pathSplit);
    if(!dir)
    {
      return NULL;
    }

    std::map<std::string, vil_stream*>::iterator f =
      dir->files.find(*pathSplit.rbegin());
    if(f == dir->files.end())
    {
      return NULL;
    }

    // Ignore file mode for now

    return new VSIVILHandle(f->second);
  }
}


int VSIVILFilesystemHandler::Stat(const char *pszFilename,
                                  VSIStatBufL *pStatBuf, int nFlags)
{
  std::memset(pStatBuf, 0, sizeof(VSIStatBufL));

  std::vector<std::string> pathSplit;
  tokenize(pszFilename, pathSplit, '/');
  std::string &name = *pathSplit.rbegin();

  {
    CPLMutexHolder mtxHolder(&this->mutex);
    VILDirTree *dir = this->tree->GetBaseDir(pathSplit);
    if(!dir)
    {
      errno = ENOENT;
      return -1;
    }

    // Check if directory
    std::map<std::string, VILDirTree*>::iterator di = dir->subDirs.find(name);
    if(di != dir->subDirs.end())
    {
      pStatBuf->st_size = 0;
      pStatBuf->st_mode = S_IFDIR;
      return 0;
    }

    // Check if file
    std::map<std::string, vil_stream*>::iterator fi = dir->files.find(name);
    if(fi != dir->files.end())
    {
      pStatBuf->st_size = fi->second->file_size();
      pStatBuf->st_mode = S_IFREG;
      return 0;
    }
  }

  errno = ENOENT;
  return -1;
}

int VSIVILFilesystemHandler::Unlink(const char *pszFilename)
{
  std::vector<std::string> pathSplit;
  tokenize(pszFilename, pathSplit, '/');
  std::string &name = *pathSplit.rbegin();

  {
    CPLMutexHolder mtxHolder(&this->mutex);
    VILDirTree *dir = this->tree->GetBaseDir(pathSplit);
    if(!dir)
    {
      errno = ENOENT;
      return -1;
    }

    // Check if file
    std::map<std::string, vil_stream*>::iterator fi = dir->files.find(name);
    if(fi == dir->files.end())
    {
      errno = ENOENT;
      return -1;
    }

    fi->second->unref();
    dir->files.erase(fi);
  }
  return 0;
}


int VSIVILFilesystemHandler::Mkdir(const char *pszDirname, long nMode)
{
  VILDirTree* dir;
  std::vector<std::string> pathSplit;
  tokenize(pszDirname, pathSplit, '/');

  return this->Mkdir(pathSplit, dir);
}

int VSIVILFilesystemHandler::Mkdir(const std::vector<std::string> &pathSplit,
                                   VILDirTree* &dir, size_t numIgnore)
{
  {
    CPLMutexHolder mtxHolder(&this->mutex);
    dir = this->tree;
    for(size_t i = 0; i < pathSplit.size() - numIgnore; ++i)
    {
      const std::string &name = pathSplit[i];

      // Make sure we don't hit a file
      std::map<std::string, vil_stream*>::iterator fi = dir->files.find(name);
      if(fi != dir->files.end())
      {
        dir = NULL;
        return -1;
      }

      std::map<std::string, VILDirTree*>::iterator di = dir->subDirs.find(name);
      if(di == dir->subDirs.end())
      {
        dir = dir->subDirs.insert(
          std::make_pair(name, new VILDirTree)).first->second;
      }
      else
      {
        dir = di->second;
      }
    }
  }
  return 0;
}

int VSIVILFilesystemHandler::Rmdir(const char *pszDirname)
{
  std::vector<std::string> pathSplit;
  tokenize(pszDirname, pathSplit, '/');
  std::string &name = *pathSplit.rbegin();

  {
    CPLMutexHolder mtxHolder(&this->mutex);
    VILDirTree *dir = this->tree->GetBaseDir(pathSplit);
    if(!dir)
    {
      errno = ENOENT;
      return -1;
    }

    // Check if directory
    std::map<std::string, VILDirTree*>::iterator di = dir->subDirs.find(name);
    if(di == dir->subDirs.end())
    {
      errno = ENOENT;
      return -1;
    }

    delete di->second;
    dir->subDirs.erase(di);
  }
  return 0;
}

char ** VSIVILFilesystemHandler::ReadDir(const char *pszDirname)
{
  // What is this even supposed to do???
  return NULL;
}

int VSIVILFilesystemHandler::Rename(const char *oldPath, const char *newPath)
{
  std::vector<std::string> oldPathSplit;
  tokenize(oldPath, oldPathSplit, '/');
  std::string &oldName = *oldPathSplit.rbegin();
  vil_stream *f;

  {
    CPLMutexHolder mtxHolder(&this->mutex);
    VILDirTree *oldDir = this->tree->GetBaseDir(oldPathSplit);
    if(!oldDir)
    {
      errno = ENOENT;
      return -1;
    }

    // Check if file
    std::map<std::string, vil_stream*>::iterator fi = oldDir->files.find(oldName);
    if(fi == oldDir->files.end())
    {
      errno = ENOENT;
      return -1;
    }

    f = fi->second;
    oldDir->files.erase(fi);
  }


  VILDirTree *newDir;
  std::vector<std::string> newPathSplit;
  tokenize(newPath, newPathSplit, '/');
  std::string &newName = *newPathSplit.rbegin();
  this->Mkdir(newPathSplit, newDir, 1);
  {
    CPLMutexHolder mtxHolder(&this->mutex);
    newDir->files.insert(std::make_pair(std::string(newName), f));
  }
  return 0;
}

void VSIVILFilesystemHandler::Insert(const char *pszFilename,
                                     vil_stream *stream)
{
  VILDirTree *dir;
  std::vector<std::string> pathSplit;
  tokenize(pszFilename, pathSplit, '/');
  std::string &name = *pathSplit.rbegin();

  this->Mkdir(pathSplit, dir, 1);
  {
    CPLMutexHolder mtxHolder(&this->mutex);
    dir->files.insert(std::make_pair(std::string(name), stream));
    stream->ref();
  }
}
