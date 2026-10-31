#pragma once

#include <stdbool.h>

/* Native adapter for the DBCE v1 physical rig. Coordinates match the toolkit:
 * eye at (0,0,0), center panel in negative Z, +X right and +Y up. Each panel
 * is placed by its visible chord (right = chord); with a curve radius the side
 * panels' pixels lie on the arc through that chord, bowing away from the eye
 * (toolkit panel_point). The center keeps radius 0: it shows the game's own
 * planar picture, so its curve is a presentation warp, not part of this model.
 * eye_distance_mm is to the center CHORD plane; FzeroTripleChordEyeDistance
 * converts the measured eye-to-glass-center distance once, at rig build. */
typedef struct FzeroTripleVec3 { double x, y, z; } FzeroTripleVec3;
typedef struct FzeroTripleSurface {
  FzeroTripleVec3 lower_left, right, up;
  double curve_radius_mm;
} FzeroTripleSurface;
typedef struct FzeroTripleRig {
  double width_mm, height_mm, eye_distance_mm, eye_height_mm;
  double left_yaw_deg, right_yaw_deg, bezel_gap_mm;
  int panel_width_px, panel_height_px;
  double curve_radius_mm;
} FzeroTripleRig;

/* Toolkit chord_eye_distance (dbce-triple-screen-toolkit 09e6c4f): the eye to
 * the glass center less the panel's sagitta; radius 0 returns it unchanged. */
bool FzeroTripleChordEyeDistance(double glass_eye_mm, double chord_mm,
                                 double curve_radius_mm, double *out);
bool FzeroTripleBuild(const FzeroTripleRig *rig, FzeroTripleSurface panels[3]);
/* The physical point at panel coordinates u (0 left .. 1 right, along the arc
 * when curved) and v (0 bottom .. 1 top). */
bool FzeroTriplePanelPoint(const FzeroTripleSurface *panel, double u, double v,
                           FzeroTripleVec3 *point);
bool FzeroTripleRay(const FzeroTripleSurface *panel, int x, int y,
                    int width, int height, FzeroTripleVec3 *ray);
/* Project a camera-space direction onto one physical panel. Coordinates may
 * lie beyond panel bounds; the caller clips them if needed. */
bool FzeroTripleProjectDirection(const FzeroTripleSurface *panel,
                                  FzeroTripleVec3 direction,
                                  int width, int height,
                                  double *pixel_x, double *pixel_y);
/* The center's curve as a presentation warp: for each vertex of a
 * (columns+1) x (rows+1) grid over the curved center glass (u left..right
 * along the arc, rows top..bottom), the point of the game's planar center
 * picture the eye sees through it, as uv pairs (0..1, y down). False for a
 * flat or invalid rig (draw the picture unchanged). */
bool FzeroTripleCenterWarp(const FzeroTripleRig *rig, int columns, int rows,
                           float *uv);
