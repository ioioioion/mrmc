#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <atomic>
#include <queue>
#include <vector>
#include <memory>

#include "DVDVideoCodec.h"
#include "DVDStreamInfo.h"
#include "threads/Thread.h"
#include "threads/SingleLock.h"
#include "platform/android/activity/JNIXBMCVideoView.h"
#include <androidjni/Surface.h>
#include "guilib/Geometry.h"

#include <media/NdkMediaCodec.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

class CJNISurface;
class CJNISurfaceTexture;
class CJNIMediaCodec;
class CJNIMediaFormat;
class CDVDMediaCodecOnFrameAvailable;
class CJNIByteBuffer;
class CBitstreamConverter;

typedef struct amc_demux {
  uint8_t  *pData;
  int       iSize;
  double    dts;
  double    pts;
} amc_demux;

class CDVDMediaCodecInfo
{
  friend class CDVDVideoCodecAndroidMediaCodec;

public:
  CDVDMediaCodecInfo( ssize_t index,
                      unsigned int texture,
                      int64_t timestamp,
                      double duration,
                      AMediaCodec* codec,
                      std::shared_ptr<CJNISurfaceTexture> &surfacetexture,
                      std::shared_ptr<CDVDMediaCodecOnFrameAvailable> &frameready,
                      std::shared_ptr<CJNIXBMCVideoView> &videoview);

  // reference counting
  CDVDMediaCodecInfo* Retain();
  long                Release();

  // meat and potatos
  void                Validate(bool state);
  // MediaCodec related
  void                ReleaseOutputBuffer(int64_t timestamp);
  // SurfaceTexture released
  bool                IsValid() const { return m_valid && !m_isReleased; }
  ssize_t             GetIndex() const;
  int                 GetTextureID() const;
  void                GetTransformMatrix(float *textureMatrix);
  double              GetDuration() const { return m_duration; }
  int64_t             GetTimestamp() const { return m_timestamp; }
  void                UpdateTexImage();
  void                RenderUpdate(const CRect &SrcRect, const CRect &DestRect);

protected:
  // private because we are reference counted
  virtual            ~CDVDMediaCodecInfo();

  std::atomic<long>   m_refs;
  bool                m_valid;
  bool                m_isReleased;
  ssize_t             m_index;
  unsigned int        m_texture;
  int64_t             m_timestamp;
  double              m_duration;
  CCriticalSection    m_section;
  // shared_ptr bits, shared between
  // CDVDVideoCodecAndroidMediaCodec and LinuxRenderGLES.
  AMediaCodec* m_codec;
  std::shared_ptr<CJNISurfaceTexture> m_surfacetexture;
  std::shared_ptr<CDVDMediaCodecOnFrameAvailable> m_frameready;
  std::shared_ptr<CJNIXBMCVideoView> m_videoview;

  int                 m_displayWidth;
  int                 m_displayHeight;
};

class CDVDVideoCodecAndroidMediaCodec : public CDVDVideoCodec, public CJNISurfaceHolderCallback
{
public:
  CDVDVideoCodecAndroidMediaCodec(bool surface_render = false, bool render_interlaced = false);
  virtual ~CDVDVideoCodecAndroidMediaCodec();

  // required overrides
  virtual bool    Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void    Dispose();
  virtual int     Decode(uint8_t *pData, int iSize, double dts, double pts);
  virtual void    Reset();
  virtual bool    GetPicture(DVDVideoPicture *pDvdVideoPicture);
  virtual bool    ClearPicture(DVDVideoPicture* pDvdVideoPicture);
  virtual void    SetDropState(bool bDrop);
  virtual void    SetCodecControl(int flags);
  virtual int     GetDataSize(void);
  virtual double  GetTimeSize(void);
  virtual const char* GetName(void) { return m_formatname.c_str(); }
  virtual unsigned GetAllowedReferences();

protected:
  void            FlushInternal(void);
  bool            ConfigureMediaCodec(void);
  int             GetOutputPicture(void);
  void            ConfigureOutputFormat(AMediaFormat* mediaformat);

  // surface handling functions
  static void     CallbackInitSurfaceTexture(void*);
  void            InitSurfaceTexture(void);
  void            ReleaseSurfaceTexture(void);

  CDVDStreamInfo  m_hints;
  std::string     m_mime;
  std::string     m_codecname;
  int             m_colorFormat;
  std::string     m_formatname;
  bool            m_opened;
  bool            m_drop;
  int             m_codecControlFlags;
  int             m_state;
  double          m_lastpts;

  std::shared_ptr<CJNIXBMCVideoView> m_jnivideoview;
  CJNISurface*    m_jnisurface;
  CJNISurface     m_jnivideosurface;
  unsigned int    m_textureId;
  AMediaCodec*    m_codec;
  ANativeWindow*  m_surface;
  std::shared_ptr<CJNISurfaceTexture> m_surfaceTexture;
  std::shared_ptr<CDVDMediaCodecOnFrameAvailable> m_frameAvailable;

  std::queue<amc_demux> m_demux;
  std::vector<CDVDMediaCodecInfo*> m_inflight;

  CBitstreamConverter *m_bitstream;
  DVDVideoPicture m_videobuffer;

  bool            m_render_sw;
  bool            m_render_surface;
  bool            m_render_interlaced;
  int             m_src_offset[4];
  int             m_src_stride[4];

  // CJNISurfaceHolderCallback interface
public:
  virtual void surfaceChanged(CJNISurfaceHolder holder, int format, int width, int height) override;
  virtual void surfaceCreated(CJNISurfaceHolder holder) override;
  virtual void surfaceDestroyed(CJNISurfaceHolder holder) override;
};
