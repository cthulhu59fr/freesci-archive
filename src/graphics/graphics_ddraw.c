/***************************************************************************
 graphics_sdl.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


 This program may be modified and copied freely according to the terms of
 the GNU general public license (GPL), as long as the above copyright
 notice and the licensing information contained herein are preserved.

 Please refer to www.gnu.org for licensing details.

 This work is provided AS IS, without warranty of any kind, expressed or
 implied, including but not limited to the warranties of merchantibility,
 noninfringement, and fitness for a specific purpose. The author will not
 be held liable for any damage caused by this work or derivatives of it.

 By using this source code, you agree to the licensing terms as stated
 above.


 Please contact the maintainer for bug reports or inquiries.

 Current Maintainer:

    Christoph Reichenbach (CJR) [creichen@rbg.informatik.tu-darmstadt.de]

 History:

    990819 - created [DJ]

***************************************************************************/

#include <config.h>
#ifdef HAVE_DDRAW

#include <graphics_ddraw.h>
#include <uinput.h>
#include <engine.h>
#include <vm.h>

#include <Hermes.h>

static HWND hMainWnd;
static IDirectDraw *pDD=NULL;
static IDirectDrawSurface *pPrimary=NULL;
static IDirectDrawSurface *pBuffer=NULL;
static IDirectDrawClipper *pClipper=NULL;
static IDirectDrawPalette *pPalette=NULL;
static int WndXStart, WndYStart;

static HermesHandle hhPalette;
static HermesHandle hhConverter;
static HermesFormat *hfSrc;
static HermesFormat *hfDest;

static RGBQUAD color_table [256];

/*** Graphics driver ***/

gfx_driver_t gfx_driver_ddraw = 
{
  "ddraw",
  ddraw_init,
  ddraw_shutdown,
  ddraw_redraw
};

/*** Initialization and window stuff ***/

void initColors()
{
  int i;
  RGBQUAD vcal [16];

  for (i=0; i<16; i++) {
    vcal[i].rgbRed = (i & 0x04) ? 0xaaaa : 0;
    vcal[i].rgbGreen = (i & 0x02) ? 0xaaaa : 0;
    vcal[i].rgbBlue = (i & 0x01) ? 0xaaaa : 0;
    if (i & 0x08) {
      vcal[i].rgbRed += 0x5555;
      vcal[i].rgbGreen += 0x5555;
      vcal[i].rgbBlue += 0x5555;
    }
    if (i == 6) { /* Srgbcial exception for brown */
      vcal[i].rgbGreen >>= 1;
    }
  }
  
  for (i=0; i< 256; i++) {
    color_table [i].rgbRed = (vcal[i & 0xf].rgbRed / 5)*3
      + (vcal[i >> 4].rgbRed / 5)*2;
    color_table [i].rgbGreen = (vcal[i & 0xf].rgbGreen / 5)*3
      + (vcal[i >> 4].rgbGreen / 5)*2;
    color_table [i].rgbBlue = (vcal[i & 0xf].rgbBlue / 5)*3
      + (vcal[i >> 4].rgbBlue / 5)*2;
    color_table [i].rgbReserved = 0;
  }
}

long FAR PASCAL WndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_MOVE:
  case WM_SIZE:
    {
      POINT pnt;
      pnt.x = 0; pnt.y = 0;
      ClientToScreen (hMainWnd, &pnt);
      WndXStart = pnt.x;
      WndYStart = pnt.y;
    }
    break;

  case WM_DESTROY:
    script_abort_flag = 1;
    PostQuitMessage (0);
    break;
  }

  return DefWindowProc (hWnd, message, wParam, lParam);
}

#define SPRINTF_DDERR(x,y) case x : sprintf (buf, y); break
#define TRACE_DDERR(x) SPRINTF_DDERR(x,#x)

static void TraceLastDDrawError (HRESULT hResult, char *buf)
{
  switch (hResult)
  {
    TRACE_DDERR (DDERR_ALREADYINITIALIZED);
    TRACE_DDERR (DDERR_CANNOTATTACHSURFACE);
    TRACE_DDERR (DDERR_CANNOTDETACHSURFACE);
    TRACE_DDERR (DDERR_CURRENTLYNOTAVAIL);
    TRACE_DDERR (DDERR_EXCEPTION);
    TRACE_DDERR (DDERR_GENERIC);
    TRACE_DDERR (DDERR_HEIGHTALIGN);
    TRACE_DDERR (DDERR_INCOMPATIBLEPRIMARY);
    TRACE_DDERR (DDERR_INVALIDCAPS);
    TRACE_DDERR (DDERR_INVALIDCLIPLIST);
    TRACE_DDERR (DDERR_INVALIDMODE);
    TRACE_DDERR (DDERR_INVALIDOBJECT);
    TRACE_DDERR (DDERR_INVALIDPARAMS);
    TRACE_DDERR (DDERR_INVALIDPIXELFORMAT);
    TRACE_DDERR (DDERR_INVALIDRECT);
    TRACE_DDERR (DDERR_LOCKEDSURFACES);
    TRACE_DDERR (DDERR_NO3D);
    TRACE_DDERR (DDERR_NOALPHAHW);
    TRACE_DDERR (DDERR_NOCLIPLIST);
    TRACE_DDERR (DDERR_NOCOLORCONVHW);
    TRACE_DDERR (DDERR_NOCOOPERATIVELEVELSET);
    TRACE_DDERR (DDERR_NOCOLORKEY);
    TRACE_DDERR (DDERR_NOCOLORKEYHW);
    TRACE_DDERR (DDERR_NODIRECTDRAWSUPPORT);
    TRACE_DDERR (DDERR_NOEXCLUSIVEMODE);
    TRACE_DDERR (DDERR_NOFLIPHW);
    TRACE_DDERR (DDERR_NOGDI);
    TRACE_DDERR (DDERR_NOMIRRORHW);
    TRACE_DDERR (DDERR_NOTFOUND);
    TRACE_DDERR (DDERR_NOOVERLAYHW);
    TRACE_DDERR (DDERR_NORASTEROPHW);
    TRACE_DDERR (DDERR_NOROTATIONHW);
    TRACE_DDERR (DDERR_NOSTRETCHHW);
    TRACE_DDERR (DDERR_NOT4BITCOLOR);
    TRACE_DDERR (DDERR_NOT4BITCOLORINDEX);
    TRACE_DDERR (DDERR_NOT8BITCOLOR);
    TRACE_DDERR (DDERR_NOTEXTUREHW);
    TRACE_DDERR (DDERR_NOVSYNCHW);
    TRACE_DDERR (DDERR_NOZBUFFERHW);
    TRACE_DDERR (DDERR_NOZOVERLAYHW);
    TRACE_DDERR (DDERR_OUTOFCAPS);
    TRACE_DDERR (DDERR_OUTOFMEMORY);
    TRACE_DDERR (DDERR_OUTOFVIDEOMEMORY);
    TRACE_DDERR (DDERR_OVERLAYCANTCLIP);
    TRACE_DDERR (DDERR_OVERLAYCOLORKEYONLYONEACTIVE);
    TRACE_DDERR (DDERR_PALETTEBUSY);
    TRACE_DDERR (DDERR_COLORKEYNOTSET);
    TRACE_DDERR (DDERR_SURFACEALREADYATTACHED);
    TRACE_DDERR (DDERR_SURFACEALREADYDEPENDENT);
    TRACE_DDERR (DDERR_SURFACEBUSY);
    TRACE_DDERR (DDERR_CANTLOCKSURFACE);
    TRACE_DDERR (DDERR_SURFACEISOBSCURED);
    TRACE_DDERR (DDERR_SURFACELOST);
    TRACE_DDERR (DDERR_SURFACENOTATTACHED);
    TRACE_DDERR (DDERR_TOOBIGHEIGHT);
    TRACE_DDERR (DDERR_TOOBIGSIZE);
    TRACE_DDERR (DDERR_TOOBIGWIDTH);
    TRACE_DDERR (DDERR_UNSUPPORTED);
    TRACE_DDERR (DDERR_UNSUPPORTEDFORMAT);
    TRACE_DDERR (DDERR_UNSUPPORTEDMASK);
    TRACE_DDERR (DDERR_VERTICALBLANKINPROGRESS);
    TRACE_DDERR (DDERR_WASSTILLDRAWING);
    TRACE_DDERR (DDERR_XALIGN);
    TRACE_DDERR (DDERR_INVALIDDIRECTDRAWGUID);
    TRACE_DDERR (DDERR_DIRECTDRAWALREADYCREATED);
    TRACE_DDERR (DDERR_NODIRECTDRAWHW);
    TRACE_DDERR (DDERR_PRIMARYSURFACEALREADYEXISTS);
    TRACE_DDERR (DDERR_NOEMULATION);
    TRACE_DDERR (DDERR_REGIONTOOSMALL);
    TRACE_DDERR (DDERR_CLIPPERISUSINGHWND);
    TRACE_DDERR (DDERR_NOCLIPPERATTACHED);
    TRACE_DDERR (DDERR_NOHWND);
    TRACE_DDERR (DDERR_HWNDSUBCLASSED);
    TRACE_DDERR (DDERR_HWNDALREADYSET);
    TRACE_DDERR (DDERR_NOPALETTEATTACHED);
    TRACE_DDERR (DDERR_NOPALETTEHW);
    TRACE_DDERR (DDERR_BLTFASTCANTCLIP);
    TRACE_DDERR (DDERR_NOBLTHW);
    TRACE_DDERR (DDERR_NODDROPSHW);
    TRACE_DDERR (DDERR_OVERLAYNOTVISIBLE);
    TRACE_DDERR (DDERR_NOOVERLAYDEST);
    TRACE_DDERR (DDERR_INVALIDPOSITION);
    TRACE_DDERR (DDERR_NOTAOVERLAYSURFACE);
    TRACE_DDERR (DDERR_EXCLUSIVEMODEALREADYSET);
    TRACE_DDERR (DDERR_NOTFLIPPABLE);
    TRACE_DDERR (DDERR_CANTDUPLICATE);
    TRACE_DDERR (DDERR_NOTLOCKED);
    TRACE_DDERR (DDERR_CANTCREATEDC);
    TRACE_DDERR (DDERR_NODC);
    TRACE_DDERR (DDERR_WRONGMODE);
    TRACE_DDERR (DDERR_IMPLICITLYCREATED);
    TRACE_DDERR (DDERR_NOTPALETTIZED);
    TRACE_DDERR (DDERR_UNSUPPORTEDMODE);
    TRACE_DDERR (DDERR_NOMIPMAPHW);
    TRACE_DDERR (DDERR_INVALIDSURFACETYPE);
    TRACE_DDERR (DDERR_DCALREADYCREATED);
    TRACE_DDERR (DDERR_CANTPAGELOCK);
    TRACE_DDERR (DDERR_CANTPAGEUNLOCK);
    TRACE_DDERR (DDERR_NOTPAGELOCKED);
    TRACE_DDERR (DDERR_NOTINITIALIZED);
    default : sprintf (buf, "Unknown DirectDraw error %i\n", hResult);
  } 
}

int
DDrawFailure (HRESULT hr)
{
  char buf [128];
  TraceLastDDrawError (hr, buf);
  printf ("DirectDraw operation failed: %s\n", buf);
  return 1;
}

int
ddraw_init(state_t *s, struct _picture *pic)
{
  WNDCLASS wc;
  HRESULT hr;
  DDSURFACEDESC ddsd;
  DDPIXELFORMAT ddpf;
  MSG msg;
  RECT rc;
  char buf [128];
  int32* pPal;

  /* Register window class */
  wc.style         = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc   = WndProc;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = 0;
  wc.hInstance     = NULL;
  wc.hIcon         = LoadIcon (NULL, IDI_APPLICATION);
  wc.hCursor       = LoadCursor (NULL, IDC_ARROW);
  wc.hbrBackground = GetStockObject (BLACK_BRUSH);
  wc.lpszMenuName  = NULL;
  wc.lpszClassName = "freesci.WndClass";
  RegisterClass (&wc);

  /* Create and show window */
  SetRect (&rc, 0, 0, SCI_SCREEN_WIDTH*2, SCI_SCREEN_HEIGHT*2);
  AdjustWindowRectEx (&rc, WS_OVERLAPPEDWINDOW, FALSE, 0);

  sprintf (buf, "%s - %s %s", s->game_name, PACKAGE, VERSION);

  hMainWnd = CreateWindowEx (
    0,
    "freesci.WndClass",
    buf,
    WS_OVERLAPPEDWINDOW,
    0,
    0,
    rc.right-rc.left,
    rc.bottom-rc.top,
    NULL,
    NULL,
    NULL,
    NULL
  );
                              
  if (!hMainWnd) return 1;

  ShowWindow (hMainWnd, SW_SHOW);
  UpdateWindow (hMainWnd);
  SetFocus (hMainWnd);

  /* Initialize DirectDraw for windowed mode, create the surface and 
     attach cliprgbr */
  if (!pDD)
  {
    hr=DirectDrawCreate (NULL, &pDD, NULL);
    if (!pDD) return DDrawFailure (hr);
  }

  hr=IDirectDraw_SetCooperativeLevel (pDD, hMainWnd, DDSCL_NORMAL);
  if (hr != DD_OK) return DDrawFailure (hr);

  ddsd.dwSize = sizeof (DDSURFACEDESC);
  ddsd.dwFlags = DDSD_CAPS;
  ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

  hr=IDirectDraw_CreateSurface (pDD, &ddsd, &pPrimary, NULL);
  if (hr != DD_OK) return DDrawFailure (hr);

  memset (&ddsd, 0, sizeof (DDSURFACEDESC));
  ddsd.dwSize = sizeof (DDSURFACEDESC);
  ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
  ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
  ddsd.dwWidth = SCI_SCREEN_WIDTH*2;
  ddsd.dwHeight = SCI_SCREEN_HEIGHT*2;

  hr=IDirectDraw_CreateSurface (pDD, &ddsd, &pBuffer, NULL);
  if (hr != DD_OK) return DDrawFailure (hr);

  hr=IDirectDraw_CreateClipper (pDD, 0, &pClipper, NULL);
  if (hr != DD_OK) return DDrawFailure (hr);

  hr=IDirectDrawClipper_SetHWnd (pClipper, 0, hMainWnd);
  if (hr != DD_OK) return DDrawFailure (hr);

  hr=IDirectDrawSurface_SetClipper (pPrimary, pClipper);
  if (hr != DD_OK) return DDrawFailure (hr);

  /* Find out the pixel format of the primary surface */
  ddpf.dwSize = sizeof (DDPIXELFORMAT);
  IDirectDrawSurface_GetPixelFormat (pBuffer, &ddpf);

  initColors();

  if (ddpf.dwFlags & DDPF_PALETTEINDEXED8)
  {
    /* The RGBQUAD and PALETTEENTRY structure have different order of RGB
       bytes. The order in RGBQUAD corresponds to what Hermes understands,
       but for DirectDraw we need to do a conversion. */
    PALETTEENTRY pe [256];
    int i;

    for (i=0; i<256; i++)
    {
      pe [i].peBlue  = color_table [i].rgbBlue;
      pe [i].peGreen = color_table [i].rgbGreen;
      pe [i].peRed   = color_table [i].rgbRed;
      pe [i].peFlags = 0;
    }

    hr=IDirectDraw_CreatePalette (pDD, DDPCAPS_8BIT | DDPCAPS_ALLOW256, pe, &pPalette, NULL);
    if (hr != DD_OK) return DDrawFailure (hr);
    
    hr=IDirectDrawSurface_SetPalette (pPrimary, pPalette);
    if (hr != DD_OK) return DDrawFailure (hr);
  }

  /* Initialize Hermes */
  Hermes_Init();
  hhPalette = Hermes_PaletteInstance();
  if (!hhPalette) return 1;
  pPal=Hermes_PaletteGet (hhPalette);
  memcpy (pPal, color_table, 4*256);
  Hermes_PaletteInvalidateCache (hhPalette);
  hhConverter = Hermes_ConverterInstance (0);
  if (!hhConverter) return 1;
  hfSrc = Hermes_FormatNew (8, 0, 0, 0, 0, 1);
  
  /* Create Hermes format corresponding to the DirectDraw format of the
     primary surface */
  hfDest = Hermes_FormatNew (ddpf.dwRGBBitCount,
    ddpf.dwRBitMask, ddpf.dwGBitMask, ddpf.dwBBitMask, 0, 
    ((ddpf.dwFlags & DDPF_PALETTEINDEXED8) != 0));
 
  /* Set graphics callback */
  /*s->graphics_callback = graphics_callback_ddraw;*/

  /* Process initial messages */
  while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
  {
    if (!GetMessage (&msg, NULL, 0, 0)) return 0;
    TranslateMessage (&msg);
    DispatchMessage(&msg);
  }

  return 0;
}

void
ddraw_shutdown(state_t *s)
{
  printf ("Closing DirectDraw\n");
  /* Deinitialize Hermes */
  Hermes_ConverterReturn (hhConverter);
  Hermes_PaletteReturn (hhPalette);
  Hermes_Done();

  /* Deinitialize DirectDraw */
  if (pDD)
  {
    if (pClipper)
    {
      IDirectDrawClipper_Release (pClipper);
      pClipper = NULL;
    }

    if (pPalette)
    {
      IDirectDrawPalette_Release (pPalette);
      pPalette = NULL;
    }

    if (pBuffer)
    {
      IDirectDrawSurface_Release (pBuffer);
      pBuffer = NULL;
    }

    if (pPrimary)
    {
      IDirectDrawSurface_Release (pPrimary);
      pPrimary = NULL;
    }
    IDirectDraw_Release (pDD);
    pDD = NULL;
  }
}

/*** Input and message handling stuff ***/

void
MsgWait (int WaitTime)
{
  MSG msg;
  DWORD dwRet=0;
  long dwWait;
  DWORD StartTime=timeGetTime();

  while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
  {
    if (!GetMessage (&msg, NULL, 0, 0)) return;
    TranslateMessage (&msg);
    DispatchMessage(&msg);
  }

  if (WaitTime > 0) 
  {
    dwRet=MsgWaitForMultipleObjects (0, NULL, FALSE, WaitTime, QS_ALLINPUT);
  
    while (dwRet != WAIT_TIMEOUT)
    {
      while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
      {
        if (!GetMessage (&msg, NULL, 0, 0)) return;
        TranslateMessage (&msg);
        DispatchMessage(&msg);
      }
      dwWait=WaitTime-(timeGetTime()-StartTime);
      if (dwWait <= 0) break;
      dwRet=MsgWaitForMultipleObjects (0, NULL, FALSE, dwWait, QS_ALLINPUT);
    }
  }
}

void
Win32_usleep (long usec)
{
  /* [DJ] I don't think that message pumping should be done in a function
     which is supposed to work with microsecond precision. */
  LARGE_INTEGER lFrequency;
  LARGE_INTEGER lEndTime;
  LARGE_INTEGER lCurTime;

  QueryPerformanceFrequency (&lFrequency);
  if (lFrequency.QuadPart)
  {
    QueryPerformanceCounter (&lEndTime);
    lEndTime.QuadPart += (LONGLONG) usec * lFrequency.QuadPart / 1000000;
    do
    {
      QueryPerformanceCounter (&lCurTime);
    } while (lCurTime.QuadPart < lEndTime.QuadPart);
  }
}

/*** Graphics callback ***/

void
graphics_draw_region_ddraw(byte *data,
                           int sx, int sy,
                           int pitch,
                           int *color_key,
			   int x, int y, int xl, int yl)
{
  DDSURFACEDESC ddsd;
  RECT rcDest, rcSrc;
  HRESULT hr;
  DWORD cvt_color_key;
  DDBLTFX bltfx;

  /* adjust coordinates */
  if (x < 0) {
    xl += x;
    x = 0;
  }
  if (x > SCI_SCREEN_WIDTH) {
    xl -= (x - SCI_SCREEN_WIDTH);
    x = SCI_SCREEN_WIDTH;
  }

  if (y < 0) {
    yl += y;
    y = 0;
  }
  if (y > SCI_SCREEN_HEIGHT) {
    yl -= (y - SCI_SCREEN_HEIGHT);
    y = SCI_SCREEN_HEIGHT;
  }

  if (sx < 0)
    sx = 0;
  if (sx > SCI_SCREEN_WIDTH)
    sx = SCI_SCREEN_WIDTH;
  if (sy < 0)
    sy = 0;
  if (sy > SCI_SCREEN_HEIGHT)
    sy = SCI_SCREEN_HEIGHT;

  if (sx+xl > SCI_SCREEN_WIDTH)
    xl = SCI_SCREEN_WIDTH-sx;
  if (sy+yl > SCI_SCREEN_HEIGHT)
    yl = SCI_SCREEN_HEIGHT-sy;

  if (IDirectDrawSurface_IsLost (pPrimary) == DDERR_SURFACELOST)
    IDirectDrawSurface_Restore (pPrimary);

  /* In the window mode, locking the primary surface gives us a pointer
     to the entire desktop space. However, it is not a good idea to draw
     over other windows. In order to clip the output correctly, we can
     either obtain the clip list from IDirectDrawClipper and process it
     ourselves, or do a blit from a secondary buffer, letting DirectDraw
     use the clipper by itself. The second approach is faster and easier
     to implement, despite the need of using a secondary buffer. */
    
  ddsd.dwSize = sizeof (DDSURFACEDESC);
  IDirectDrawSurface_Lock (pBuffer, NULL, &ddsd, DDLOCK_WAIT, NULL);

  Hermes_ConverterRequest (hhConverter, hfSrc, hfDest);
  Hermes_ConverterPalette (hhConverter, hhPalette, hhPalette);

  if (color_key)
  {
    /* Convert a single pixel with color_key color and find out the color
       it got converted to. We use it later for color-keyed blitting */
    Hermes_ConverterCopy (hhConverter,
      color_key, 0, 0, 1, 1, 1,
      ddsd.lpSurface,
      0,0,1,1,
      ddsd.lPitch);
    
    cvt_color_key=0;
    memcpy (&cvt_color_key, ddsd.lpSurface, ddsd.ddpfPixelFormat.dwRGBBitCount);
  }
  
  if (!Hermes_ConverterCopy (hhConverter,
    data,
    sx, sy, xl, yl,
    pitch,
    ddsd.lpSurface,
    0, 0, xl*2, yl*2,
    ddsd.lPitch))
    printf ("Hermes copy failed\n");
  
  IDirectDrawSurface_Unlock (pBuffer, NULL);

  SetRect (&rcDest, WndXStart+x*2, WndYStart+y*2, WndXStart+(x+xl)*2, WndYStart+(y+yl)*2);
  SetRect (&rcSrc, 0, 0, xl*2, yl*2);
  if (!color_key)
    hr=IDirectDrawSurface_Blt (pPrimary, &rcDest, pBuffer, &rcSrc, DDBLT_WAIT, NULL);
  else
  {
    bltfx.dwSize = sizeof (DDBLTFX);
    bltfx.ddckSrcColorkey.dwColorSpaceLowValue = cvt_color_key;
    bltfx.ddckSrcColorkey.dwColorSpaceHighValue = cvt_color_key;
    hr=IDirectDrawSurface_Blt (pPrimary, &rcDest, pBuffer, &rcSrc, 
      DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE, &bltfx);
  }

  if (hr != DD_OK) 
  {
    DDrawFailure (hr);
    printf ("src rect (%d,%d)-(%d,%d)\n", rcSrc.left, rcSrc.top, rcSrc.right, rcSrc.bottom);
    printf ("dest rect (%d,%d)-(%d,%d)\n", rcDest.left, rcDest.top, rcDest.right, rcDest.bottom);
  }
}

void
ddraw_redraw (struct _state *s, int command, int x, int y, int xl, int yl)
{
  int mp_x, mp_y, mp_size_x, mp_size_y;

  if (s->mouse_pointer) {
    mp_x = s->pointer_x - s->mouse_pointer->hot_x;
    mp_y = s->pointer_y - s->mouse_pointer->hot_y;
    mp_size_x = s->mouse_pointer->size_x;
    mp_size_y = s->mouse_pointer->size_y;
  } else { /* No mouse pointer */
    mp_x = s->pointer_x;
    mp_y = s->pointer_y;
    mp_size_x = mp_size_y = 0;
  }

  switch (command) {
  case GRAPHICS_CALLBACK_REDRAW_ALL:
    graphics_draw_region_ddraw(s->pic->view,
			       0, 0, SCI_SCREEN_WIDTH, NULL,
                               0, 0, SCI_SCREEN_WIDTH, SCI_SCREEN_HEIGHT);
    break;

  case GRAPHICS_CALLBACK_REDRAW_BOX:
    if (xl > 0 && yl > 0)
      graphics_draw_region_ddraw(s->pic->view, /* Draw box */
                                 x, y, SCI_SCREEN_WIDTH, NULL,
                                 x, y, xl, yl);
    break;

  case GRAPHICS_CALLBACK_REDRAW_POINTER:
    if (s->last_pointer_size_x > 0 && s->last_pointer_size_y > 0)
      graphics_draw_region_ddraw(s->pic->view, /* Remove old pointer */
                                 s->last_pointer_x,s->last_pointer_y, SCI_SCREEN_WIDTH, NULL,
			         s->last_pointer_x,s->last_pointer_y,
			         s->last_pointer_size_x, s->last_pointer_size_y);
    break;
default:
    fprintf(stderr,"graphics_callback_ddraw: Invalid command %d\n", command);
    return;
  }

  /* Redraw mouse pointer, if present */
  if (s->mouse_pointer)
    graphics_draw_region_ddraw(s->mouse_pointer->bitmap,
                               0, 0, s->mouse_pointer->size_x, &s->mouse_pointer->color_key,
                               s->pointer_x, s->pointer_y,
                               s->mouse_pointer->size_x, s->mouse_pointer->size_y);

  s->last_pointer_size_x = mp_size_x;
  s->last_pointer_size_y = mp_size_y;
  s->last_pointer_x = mp_x;
  s->last_pointer_y = mp_y; /* Update mouse pointer status */
}

#endif /* HAVE_DDRAW */