/* httpreply.cpp
   Copyright (C) 2003-2005 Tommi Maekitalo

This file is part of tntnet.

Tntnet is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

Tntnet is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with tntnet; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330,
Boston, MA  02111-1307  USA
*/

#include <tnt/httpreply.h>
#include <tnt/http.h>
#include <cxxtools/log.h>
#include <cxxtools/md5stream.h>

namespace tnt
{
  log_define("tntnet.http");

  ////////////////////////////////////////////////////////////////////////
  // httpReply
  //
  httpReply::httpReply(std::ostream& s)
    : contentType("text/html"),
      socket(s),
      current_outstream(&outstream)
  { }

  void httpReply::sendReply(unsigned ret)
  {
    if (!isDirectMode())
    {
      log_debug("HTTP/" << getMajorVersion() << '.' << getMinorVersion() << ' ' << ret << " OK");
      socket << "HTTP/" << getMajorVersion() << '.' << getMinorVersion() << ' ' << ret << " OK" << "\r\n";

      sendHeaders(keepAlive());

      socket << "\r\n";
      if (getMethod() == "HEAD")
        log_debug("HEAD-request - empty body");
      else
      {
        log_debug("send " << outstream.str().size() << " bytes body, method=" << getMethod());
        socket << outstream.str();
      }
    }

    socket.flush();
  }

  void httpReply::sendHeaders(bool keepAlive)
  {
    if (header.find(Date) == header.end())
      setHeader(Date, htdate(time(0)));

    if (header.find(Server) == header.end())
      setHeader(Server, ServerName);

    if (keepAlive)
    {
      if (header.find(Connection) == header.end())
        setHeader(Connection, Connection_Keep_Alive);

      if (header.find(KeepAlive) == header.end())
        setHeader(KeepAlive, KeepAliveParam);

      if (header.find(Content_Length) == header.end())
        setContentLengthHeader(outstream.str().size());
    }

    if (header.find(Content_Type) == header.end())
      setHeader(Content_Type, contentType);

    for (header_type::const_iterator it = header.begin();
         it != header.end(); ++it)
    {
      log_debug(it->first << ' ' << it->second);
      socket << it->first << ' ' << it->second << "\r\n";
    }

    if (hasCookies())
    {
      log_debug(SetCookie << ' ' << httpcookies);
      socket << SetCookie << ' ' << httpcookies << "\r\n";
    }
  }

  void httpReply::setMd5Sum()
  {
    cxxtools::md5stream md5;
    md5 << outstream.str().size();
    setHeader(Content_MD5, md5.getHexDigest());
  }

  void httpReply::throwError(unsigned errorCode, const std::string& errorMessage) const
  {
    throw httpError(errorCode, errorMessage);
  }

  void httpReply::throwError(const std::string& errorMessage) const
  {
    throw httpError(errorMessage);
  }

  void httpReply::throwNotFound(const std::string& errorMessage) const
  {
    throw notFoundException(errorMessage);
  }

  unsigned httpReply::redirect(const std::string& newLocation)
  {
    setHeader(Location, newLocation);
    return HTTP_MOVED_TEMPORARILY;
  }

  void httpReply::setDirectMode(bool keepAlive)
  {
    if (!isDirectMode())
    {
      log_debug("HTTP/" << getMajorVersion() << '.' << getMinorVersion()
             << " 200 OK");

      socket << "HTTP/" << getMajorVersion() << '.' << getMinorVersion()
             << " 200 OK\r\n";

      sendHeaders(keepAlive);

      socket << "\r\n";
      if (getMethod() == "HEAD")
        log_debug("HEAD-request - empty body");
      else
      {
        log_debug("send " << outstream.str().size() << " bytes body");
        socket << outstream.str();
        current_outstream = &socket;
      }
    }
  }

  void httpReply::setDirectModeNoFlush()
  {
    current_outstream = &socket;
  }

  void httpReply::setCookie(const std::string& name, const cookie& value)
  {
    log_debug("setCookie(\"" << name << "\",\"" << value.getValue() << "\")");
    httpcookies.setCookie(name, value);
  }

}
