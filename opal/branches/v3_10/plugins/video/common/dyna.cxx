/*
 * Common Plugin code for OpenH323/OPAL
 *
 * This code is based on the following files from the OPAL project which
 * have been removed from the current build and distributions but are still
 * available in the CVS "attic"
 * 
 *    src/codecs/h263codec.cxx 
 *    include/codecs/h263codec.h 

 * The original files, and this version of the original code, are released under the same 
 * MPL 1.0 license. Substantial portions of the original code were contributed
 * by Salyens and March Networks and their right to be identified as copyright holders
 * of the original code portions and any parts now included in this new copy is asserted through 
 * their inclusion in the copyright notices below.
 *
 * Copyright (C) 2006 Post Increment
 * Copyright (C) 2005 Salyens
 * Copyright (C) 2001 March Networks Corporation
 * Copyright (C) 1999-2000 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): Guilhem Tardy (gtardy@salyens.com)
 *                 Craig Southeren (craigs@postincrement.com)
 *                 Matthias Schneider (ma30002000@yahoo.de)
 */
#include "dyna.h"
#include <cstdio>
#include <cstdarg>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/mem.h>
}

bool DynaLink::Open(const char *name)
{
  // At first we try without a path
  if (InternalOpen("", name))
    return true;

  // Try the current directory
  if (InternalOpen(".", name))
    return true;

  // try directories specified in PTLIBPLUGINDIR
  char ptlibPath[1024];
  char * env = ::getenv("PTLIBPLUGINDIR");
  if (env != NULL) 
    strcpy(ptlibPath, env);
  else
#ifdef P_DEFAULT_PLUGIN_DIR
    strcpy(ptlibPath, P_DEFAULT_PLUGIN_DIR);
#elif _WIN32
    strcpy(ptlibPath, "C:\\PTLib_Plugins");
#else
    strcpy(ptlibPath, "/usr/local/lib");
#endif
  char * p = ::strtok(ptlibPath, DIR_TOKENISER);
  while (p != NULL) {
    if (InternalOpen(p, name))
      return true;
    p = ::strtok(NULL, DIR_TOKENISER);
  }

  return false;
}

bool DynaLink::InternalOpen(const char * dir, const char *name)
{
  char path[1024];
  memset(path, 0, sizeof(path));

  // Copy the directory to "path" and add a separator if necessary
  if (strlen(dir) > 0) {
    strcpy(path, dir);
    if (path[strlen(path)-1] != DIR_SEPARATOR[0]) 
      strcat(path, DIR_SEPARATOR);
  }
  strcat(path, name);

  if (strlen(path) == 0) {
    PTRACE(1, m_codecString, "DynaLink: dir '" << (dir != NULL ? dir : "(NULL)")
           << "', name '" << (name != NULL ? name : "(NULL)") << "' resulted in empty path");
    return false;
  }

  // Load the Libary
#ifdef _WIN32
# ifdef UNICODE
  // must be called before using avcodec lib
  USES_CONVERSION;
  m_hDLL = LoadLibraryEx(A2T(path), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
# else
  // must be called before using avcodec lib
  m_hDLL = LoadLibraryEx(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
# endif /* UNICODE */
#else
  // must be called before using avcodec lib
  m_hDLL = dlopen((const char *)path, RTLD_NOW);
#endif /* _WIN32 */

  // Check for errors
  if (m_hDLL == NULL) {
#ifndef _WIN32
    const char * err = dlerror();
    if (err != NULL)
      PTRACE(1, m_codecString, "dlopen error " << err);
    else
      PTRACE(1, m_codecString, "dlopen error loading " << path);
#else /* _WIN32 */
    PTRACE(1, m_codecString, "Error loading " << path);
#endif /* _WIN32 */
    return false;
  } 

#ifdef _WIN32
  GetModuleFileName(m_hDLL, path, sizeof(path));
#endif

  PTRACE(1, m_codecString, "Successfully loaded '" << path << "'");
  return true;
}

void DynaLink::Close()
{
  if (m_hDLL != NULL) {
#ifdef _WIN32
    FreeLibrary(m_hDLL);
#else
    dlclose(m_hDLL);
#endif /* _WIN32 */
    m_hDLL = NULL;
  }
}

bool DynaLink::GetFunction(const char * name, Function & func)
{
  if (m_hDLL == NULL)
    return false;

#ifdef _WIN32

  #ifdef UNICODE
    USES_CONVERSION;
    FARPROC p = GetProcAddress(m_hDLL, A2T(name));
  #else
    FARPROC p = GetProcAddress(m_hDLL, name);
  #endif /* UNICODE */
  if (p == NULL) {
    PTRACE(1, m_codecString, "Error linking function " << name << ", error=" << GetLastError());
    return false;
  }

  func = (Function)p;

#else // _WIN32

  void * p = dlsym(m_hDLL, (const char *)name);
  if (p == NULL) {
    PTRACE(1, m_codecString, "Error linking function " << name << ", error=" << dlerror());
    return false;
  }
  func = (Function &)p;

#endif // _WIN32

  return true;
}


#if PLUGINCODEC_TRACING
static void logCallbackFFMPEG(void * avcl, int severity, const char* fmt , va_list arg)
{
  int level;
  if (severity <= AV_LOG_FATAL)
    level = 0;
  else if (severity <= AV_LOG_ERROR)
    level = 1;
  else if (severity <= AV_LOG_WARNING)
    level = 2;
  else if (severity <= AV_LOG_INFO)
    level = 3;
  else if (severity <= AV_LOG_VERBOSE)
    level = 4;
  else
    level = 5;

  if (PTRACE_CHECK(level)) {
    char buffer[512];
    int len = vsnprintf(buffer, sizeof(buffer), fmt, arg);
    if (len > 0) {
      // Drop extra trailing line feed
      --len;
      if (buffer[len] == '\n')
        buffer[len] = '\0';
      // Check for bogus errors, everything works so what does this mean?
      if (strstr(buffer, "Too many slices") == 0 && strstr(buffer, "Frame num gap") == 0) {
        PluginCodec_LogFunctionInstance(level, __FILE__, __LINE__, "FFMPEG", buffer);
      }
    }
  }
}
#endif


FFMPEGLibrary::FFMPEGLibrary(CodecID codec)
{
  m_codec = codec;
  if (m_codec==CODEC_ID_H264)
      snprintf( m_codecString, sizeof(m_codecString), "H264");
  if (m_codec==CODEC_ID_H263P)
      snprintf( m_codecString, sizeof(m_codecString), "H263+");
  if (m_codec==CODEC_ID_MPEG4)
      snprintf( m_codecString, sizeof(m_codecString), "MPEG4");
  m_isLoadedOK = false;
}

FFMPEGLibrary::~FFMPEGLibrary()
{
  m_libAvcodec.Close();
  m_libAvutil.Close();
}

bool FFMPEGLibrary::Load()
{
  WaitAndSignal m(processLock);
  if (IsLoaded())
    return true;

  // must be called before using avcodec lib

  unsigned libVer = avcodec_version();
  if (libVer != LIBAVCODEC_VERSION_INT) {
    PTRACE(2, m_codecString, "Warning: compiled against libavcodec headers from version "
           << LIBAVCODEC_VERSION_MAJOR << '.' << LIBAVCODEC_VERSION_MINOR << '.' << LIBAVCODEC_VERSION_MICRO
           << ", loaded "
           << (libVer >> 16) << ((libVer>>8) & 0xff) << (libVer & 0xff));
  }
  else {
    PTRACE(3, m_codecString, "Loaded libavcodec version "
           << (libVer >> 16) << ((libVer>>8) & 0xff) << (libVer & 0xff));
  }

  avcodec_register_all();

#if PLUGINCODEC_TRACING
  AvLogSetLevel(AV_LOG_DEBUG);
  AvLogSetCallback(&logCallbackFFMPEG);
#endif

  m_isLoadedOK = true;
  PTRACE(4, m_codecString, "Successfully loaded libavcodec library and verified functions");

  return true;
}

AVCodec *FFMPEGLibrary::AvcodecFindEncoder(enum CodecID id)
{
  return avcodec_find_encoder(id);
}

AVCodec *FFMPEGLibrary::AvcodecFindDecoder(enum CodecID id)
{
  WaitAndSignal m(processLock);

  return avcodec_find_decoder(id);
}

AVCodecContext *FFMPEGLibrary::AvcodecAllocContext(AVCodec *codec)
{
  WaitAndSignal m(processLock);

  return avcodec_alloc_context3(codec);
}

AVFrame *FFMPEGLibrary::AvcodecAllocFrame(void)
{
  WaitAndSignal m(processLock);

  return avcodec_alloc_frame();
}

int FFMPEGLibrary::AvcodecOpen(AVCodecContext *ctx, AVCodec *codec)
{
  WaitAndSignal m(processLock);

  return avcodec_open2(ctx, codec, NULL);
}

int FFMPEGLibrary::AvcodecClose(AVCodecContext *ctx)
{
  WaitAndSignal m(processLock);

  return avcodec_close(ctx);
}

int FFMPEGLibrary::AvcodecEncodeVideo(AVCodecContext *ctx, BYTE *buf, int buf_size, const AVFrame *pict)
{
  int res;

  res = avcodec_encode_video(ctx, buf, buf_size, pict);

  PTRACE(6, m_codecString, "DYNA\tEncoded into " << res << " bytes, max " << buf_size);
  return res;
}

int FFMPEGLibrary::AvcodecDecodeVideo(AVCodecContext *ctx, AVFrame *pict, int *got_picture_ptr, BYTE *buf, int buf_size)
{
  AVPacket avpkt;
  av_init_packet(&avpkt);
  avpkt.data = buf;
  avpkt.size = buf_size;

  return avcodec_decode_video2(ctx, pict, got_picture_ptr, &avpkt);
}

void FFMPEGLibrary::AvcodecFree(void * ptr)
{
  WaitAndSignal m(processLock);

  av_free(ptr);
}

void FFMPEGLibrary::AvSetDimensions(AVCodecContext *s, int width, int height)
{
  WaitAndSignal m(processLock);

  avcodec_set_dimensions(s, width, height);
}

void FFMPEGLibrary::AvLogSetLevel(int level)
{
  av_log_set_level(level);
}

void FFMPEGLibrary::AvLogSetCallback(void (*callback)(void*, int, const char*, va_list))
{
  av_log_set_callback(callback);
}

bool FFMPEGLibrary::IsLoaded()
{
  return m_isLoadedOK;
}
