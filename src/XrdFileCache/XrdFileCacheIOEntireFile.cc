//----------------------------------------------------------------------------------
// Copyright (c) 2014 by Board of Trustees of the Leland Stanford, Jr., University
// Author: Alja Mrak-Tadel, Matevz Tadel, Brian Bockelman
//----------------------------------------------------------------------------------
// XRootD is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// XRootD is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with XRootD.  If not, see <http://www.gnu.org/licenses/>.
//----------------------------------------------------------------------------------

#include <stdio.h>

//#include "XrdClient/XrdClientConst.hh"
#include "XrdSys/XrdSysError.hh"
#include "XrdSfs/XrdSfsInterface.hh"
#include "XrdSys/XrdSysPthread.hh"

#include "XrdFileCacheIOEntireFile.hh"
#include "XrdFileCacheStats.hh"

using namespace XrdFileCache;

//______________________________________________________________________________


IOEntireFile::IOEntireFile(XrdOucCacheIO2 &io, XrdOucCacheStats &stats, Cache & cache)
   : IO(io, stats, cache),
     m_file(0)
{
   clLog()->Info(XrdCl::AppMsg, "IO::IO() [%p] %s", this, m_io.Path());
   
   XrdCl::URL url(io.Path());
   std::string fname = Cache::GetInstance().RefConfiguration().m_cache_dir + url.GetPath();

   m_file = new File(io, fname, 0, io.FSize());
}

IOEntireFile::~IOEntireFile()
{}

bool IOEntireFile::ioActive()
{
   return m_file->InitiateClose();
}

XrdOucCacheIO *IOEntireFile::Detach()
{
   m_statsGlobal.Add(m_file->GetStats());

   XrdOucCacheIO * io = &m_io;

   delete m_file;
   m_file = 0;

   // This will delete us!
   m_cache.Detach(this);
   return io;
}

int IOEntireFile::Read (char *buff, long long off, int size)
{
   clLog()->Debug(XrdCl::AppMsg, "IOEntireFile::Read() [%p]  %lld@%d %s", this, off, size, m_io.Path());

   // protect from reads over the file size
   if (off >= m_io.FSize())
      return 0;
   if (off < 0)
   {
      errno = EINVAL;
      return -1;
   }
   if (off + size > m_io.FSize())
      size = m_io.FSize() - off;

   ssize_t bytes_read = 0;
   ssize_t retval = 0;

   retval = m_file->Read(buff, off, size);
   clLog()->Debug(XrdCl::AppMsg, "IOEntireFile::Read() read from File retval =  %d %s", retval, m_io.Path());
   if (retval >= 0)
   {
      bytes_read += retval;
      buff += retval;
      size -= retval;

      if (size > 0)
        clLog()->Warning(XrdCl::AppMsg, "IOEntireFile::Read() missed %d bytes %s", size, m_io.Path());
   }      
   else
   {
      clLog()->Error(XrdCl::AppMsg, "IOEntireFile::Read(), origin bytes read %d %s", retval, m_io.Path());
   }

   return (retval < 0) ? retval : bytes_read;
}


/*
 * Perform a readv from the cache
 */
int IOEntireFile::ReadV (const XrdOucIOVec *readV, int n)
{
   clLog()->Warning(XrdCl::AppMsg, "IOEntireFile::ReadV(), get %d requests %s", n, m_io.Path());


   return m_file->ReadV(readV, n);
}
