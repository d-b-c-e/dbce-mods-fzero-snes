#pragma once

#include "fzero_video.h"
#include "fzero_camera.h"
#include "snes/ppu.h"

/* Each camera owns its capture history, interpolation and worker scratch.
 * A view is used by one host thread at a time; completed draws join all its
 * workers before returning. Assets passed to capture are copied, never held
 * by reference to the running guest. NULL is rejected by the view interface. */
typedef struct FzeroRenderView FzeroRenderView;
/* Set on the host thread between completed draws; shared by both views. */
FzeroRenderView *FzeroRenderViewCreate(void);
void FzeroRenderViewDestroy(FzeroRenderView *view);
void FzeroRenderViewReset(FzeroRenderView *view);
unsigned FzeroRenderViewWorkerCount(const FzeroRenderView *view);
void FzeroRenderViewBeginFrame(FzeroRenderView *view,
                               const uint8_t ram[0x20000], unsigned frame);
void FzeroRenderViewCaptureLine(FzeroRenderView *view, const Ppu *ppu, unsigned line);
void FzeroRenderViewEndFrame(FzeroRenderView *view, const Ppu *ppu,
                             const uint32_t stock[256 * 224]);
bool FzeroRenderViewDraw(FzeroRenderView *view, uint32_t *output,
                         FzeroViewport viewport, double alpha);
bool FzeroRenderViewDrawPresentation(FzeroRenderView *view, uint32_t *native,
                                     uint32_t *hd, size_t hd_capacity,
                                     FzeroViewport viewport, double alpha, unsigned scale);
/* Owned native-sized GPU jobs. Native output remains the rewind/debug image;
 * the caller seals after adding any native overlays, then owns the frame. */
SnesHdFrame *FzeroRenderViewPrepareHd(FzeroRenderView *view, uint32_t *native,
                                     FzeroViewport viewport, double alpha);
SnesHdFrame *FzeroRendererPrepareHd(uint32_t *native, FzeroViewport viewport, double alpha);
bool FzeroRenderViewHasFrame(const FzeroRenderView *view);
/* Build scenery for one versus camera from a completed stock race asset
 * capture and immutable shared world. NULL assets selects the legacy view.
 * Guest HUD/OBJ are excluded: the versus display owns both racers and HUDs.
 * This never draws or advances a guest. The asset view must be distinct. */
bool FzeroRenderViewCaptureRace(FzeroRenderView *view, const FzeroRenderView *assets,
                                const uint8_t world[0x20000], const FzeroActor *actor,
                                const FzeroCamera *camera, unsigned frame);
bool FzeroRenderViewLoadCapture(FzeroRenderView *view, const char *path);
const uint32_t *FzeroRenderViewStockFrame(const FzeroRenderView *view);

/* The original interface is a wrapper around the single-player view. */
void FzeroRendererReset(void);
bool FzeroRendererSetSupersample(unsigned samples);
unsigned FzeroRendererSupersample(void);
/* Join rendering workers on host shutdown. Reset/state loads reuse the pool. */
void FzeroRendererShutdown(void);
unsigned FzeroRendererWorkerCount(void);
void FzeroRendererBeginFrame(const uint8_t ram[0x20000], unsigned frame);
void FzeroRendererCaptureLine(const Ppu *ppu, unsigned line);
void FzeroRendererEndFrame(const Ppu *ppu, const uint32_t stock[256 * 224]);
bool FzeroRendererDraw(uint32_t *output, FzeroViewport viewport, double alpha);
/* capacity is in pixels. Native UI/OBJ stay crisp; only Mode 7 is resampled. */
bool FzeroRendererDrawHd(uint32_t *output, size_t capacity,
                         FzeroViewport viewport, double alpha, unsigned scale);
#define FZERO_RENDERER_COMBINED_PRESENTATION 1
/* Produce the native thumbnail/rewind frame and HD presentation together.
 * native may be NULL; otherwise it holds width*224 pixels. Buffers must not
 * overlap. hd_capacity is in pixels. Native composition stays exact. */
bool FzeroRendererDrawPresentation(uint32_t *native, uint32_t *hd, size_t hd_capacity,
                                   FzeroViewport viewport, double alpha, unsigned scale);
bool FzeroRendererHasFrame(void);
/* Wheelkit three screens (this fork's wheelkit-runtime branch): the two side screens in a race (on the render
 * workers, once per emulated frame), each in the eye's frame of
 * the centre screen (x right, y up, -z ahead, the eye at 1 from the centre). The ray through pixel (x, y) is
 * origin + (x + 0.5) right + (y + 0.5) down. tan_x and tan_y are the game picture's half-width and half-height on
 * the centre screen over the eye distance, panel_tan_y the screens' own half-height; logical_width the picture's
 * width in game pixels. Black outside a race (returns false). */
typedef struct FzeroSideView {
  double origin[3], right[3], down[3];
  double tan_x, tan_y, panel_tan_y;
  int logical_width;
} FzeroSideView;
bool FzeroRendererDrawSides(const FzeroSideView views[2], uint32_t *out[2], int width, int height);
/* Rival pixels drawn on the side screens so far (each rival's own sprite, as a billboard at its place). */
unsigned long long FzeroRendererSideOpponentPixels(void);
bool FzeroRendererLoadCapture(const char *path);
const uint32_t *FzeroRendererStockFrame(void);
