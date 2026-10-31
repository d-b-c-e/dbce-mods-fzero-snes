#include "fzero_triple_geometry.h"

#include <math.h>

static FzeroTripleVec3 add(FzeroTripleVec3 a, FzeroTripleVec3 b) {
  return (FzeroTripleVec3){a.x + b.x, a.y + b.y, a.z + b.z};
}
static FzeroTripleVec3 scale(FzeroTripleVec3 a, double s) {
  return (FzeroTripleVec3){a.x * s, a.y * s, a.z * s};
}
static double dot3(FzeroTripleVec3 a, FzeroTripleVec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

bool FzeroTripleChordEyeDistance(double glass_eye_mm, double chord_mm,
                                 double curve_radius_mm, double *out) {
  if (!out || !isfinite(glass_eye_mm) || !isfinite(chord_mm) ||
      !isfinite(curve_radius_mm) || glass_eye_mm <= 0 || chord_mm <= 0 ||
      curve_radius_mm < 0 ||
      (curve_radius_mm > 0 && chord_mm > 2 * curve_radius_mm)) return false;
  const double h = chord_mm / 2;
  /* Sagitta in the cancellation-free form the toolkit uses. */
  const double sag = curve_radius_mm > 0 ? h * h / (curve_radius_mm +
      sqrt(curve_radius_mm * curve_radius_mm - h * h)) : 0;
  if (glass_eye_mm - sag <= 0) return false;
  *out = glass_eye_mm - sag;
  return true;
}

bool FzeroTripleBuild(const FzeroTripleRig *rig, FzeroTripleSurface panels[3]) {
  if (!rig || !panels || !(rig->width_mm > 0 && rig->height_mm > 0 &&
      rig->eye_distance_mm > 0 && rig->bezel_gap_mm >= 0 &&
      rig->left_yaw_deg >= 0 && rig->left_yaw_deg < 90 &&
      rig->right_yaw_deg >= 0 && rig->right_yaw_deg < 90 &&
      rig->panel_width_px > 0 && rig->panel_height_px > 0 &&
      isfinite(rig->curve_radius_mm) && rig->curve_radius_mm >= 0 &&
      (rig->curve_radius_mm == 0 ||
       rig->width_mm <= 2 * rig->curve_radius_mm))) return false;
  const double half = rig->width_mm / 2;
  const double radians = 0.017453292519943295769;
  const FzeroTripleVec3 center = {0, -rig->eye_height_mm,
                                  -rig->eye_distance_mm};
  const FzeroTripleVec3 up = {0, rig->height_mm, 0};
  const FzeroTripleVec3 center_span = {rig->width_mm, 0, 0};
  panels[1] = (FzeroTripleSurface){add(center, (FzeroTripleVec3){-half,
      -rig->height_mm / 2, 0}), center_span, up};

  double left_angle = rig->left_yaw_deg * radians;
  FzeroTripleVec3 left_span = {rig->width_mm * cos(left_angle), 0,
                                -rig->width_mm * sin(left_angle)};
  FzeroTripleVec3 left_hinge = add(center,
      (FzeroTripleVec3){-half - rig->bezel_gap_mm, 0, 0});
  panels[0] = (FzeroTripleSurface){add(left_hinge,
      add(scale(left_span, -1), scale(up, -0.5))), left_span, up};

  double right_angle = rig->right_yaw_deg * radians;
  FzeroTripleVec3 right_span = {rig->width_mm * cos(right_angle), 0,
                                 rig->width_mm * sin(right_angle)};
  FzeroTripleVec3 right_hinge = add(center,
      (FzeroTripleVec3){half + rig->bezel_gap_mm, 0, 0});
  panels[2] = (FzeroTripleSurface){add(right_hinge, scale(up, -0.5)),
                                    right_span, up};
  panels[0].curve_radius_mm = panels[2].curve_radius_mm = rig->curve_radius_mm;
  panels[1].curve_radius_mm = 0;
  return true;
}

/* Unit chord direction and the unit normal toward the eye (right x up). */
static bool panel_basis(const FzeroTripleSurface *panel, double *chord,
                        FzeroTripleVec3 *unit_right, FzeroTripleVec3 *normal) {
  FzeroTripleVec3 n = {
      panel->right.y * panel->up.z - panel->right.z * panel->up.y,
      panel->right.z * panel->up.x - panel->right.x * panel->up.z,
      panel->right.x * panel->up.y - panel->right.y * panel->up.x};
  double r = sqrt(dot3(panel->right, panel->right)), nl = sqrt(dot3(n, n));
  if (!(r > 0 && nl > 0)) return false;
  *chord = r;
  *unit_right = scale(panel->right, 1 / r);
  *normal = scale(n, 1 / nl);
  return true;
}

bool FzeroTriplePanelPoint(const FzeroTripleSurface *panel, double u, double v,
                           FzeroTripleVec3 *point) {
  if (!panel || !point || !isfinite(u) || !isfinite(v)) return false;
  FzeroTripleVec3 lift = scale(panel->up, v);
  double radius = panel->curve_radius_mm, chord;
  if (radius == 0) {
    *point = add(panel->lower_left, add(scale(panel->right, u), lift));
    return true;
  }
  FzeroTripleVec3 right, normal;
  if (!panel_basis(panel, &chord, &right, &normal) || chord > 2 * radius)
    return false;
  double half = asin(chord / (2 * radius)), phi = (2 * u - 1) * half;
  /* cos(half) - cos(phi) without cancellation (toolkit panel_point). */
  double sag = -2 * sin((half + phi) / 2) * sin((half - phi) / 2) * radius;
  *point = add(add(add(panel->lower_left, scale(right, chord / 2 +
      radius * sin(phi))), scale(normal, sag)), lift);
  return true;
}

bool FzeroTripleRay(const FzeroTripleSurface *panel, int x, int y,
                    int width, int height, FzeroTripleVec3 *ray) {
  if (!panel || !ray || width <= 0 || height <= 0 || x < 0 || x >= width ||
      y < 0 || y >= height) return false;
  double u = (x + 0.5) / width;
  double v = 1.0 - (y + 0.5) / height;
  FzeroTripleVec3 point;
  if (!FzeroTriplePanelPoint(panel, u, v, &point)) return false;
  double length = sqrt(point.x * point.x + point.y * point.y +
                       point.z * point.z);
  if (!(length > 0) || !isfinite(length)) return false;
  *ray = scale(point, 1.0 / length);
  return true;
}

bool FzeroTripleProjectDirection(const FzeroTripleSurface *panel,
                                  FzeroTripleVec3 direction,
                                  int width, int height,
                                  double *pixel_x, double *pixel_y) {
  if (!panel || !pixel_x || !pixel_y || width < 1 || height < 1) return false;
  if (panel->curve_radius_mm > 0) {
    /* Where the eye ray meets the panel's cylinder (vertical axis through
     * O = chord middle + normal * R cos(half), the arc's far side), then the
     * arc angle gives u and the height along up gives v. */
    double chord, radius = panel->curve_radius_mm;
    FzeroTripleVec3 right, normal;
    double up_length = dot3(panel->up, panel->up);
    if (!panel_basis(panel, &chord, &right, &normal) || chord > 2 * radius ||
        !(up_length > 0)) return false;
    double half = asin(chord / (2 * radius));
    FzeroTripleVec3 centre = add(add(panel->lower_left,
        scale(panel->right, 0.5)), scale(normal, radius * cos(half)));
    double da = dot3(direction, right), db = dot3(direction, normal);
    double oa = -dot3(centre, right), ob = -dot3(centre, normal);
    double a2 = da * da + db * db, b1 = da * oa + db * ob;
    double c0 = oa * oa + ob * ob - radius * radius;
    double disc = b1 * b1 - a2 * c0;
    if (!(a2 > 1e-18) || !(disc >= 0)) return false;
    double t = (-b1 + sqrt(disc)) / a2;
    if (!(t > 0) || !isfinite(t)) return false;
    FzeroTripleVec3 hit = scale(direction, t);
    double along = dot3(hit, right) + oa, toward = dot3(hit, normal) + ob;
    double phi = atan2(along, -toward);
    FzeroTripleVec3 relative = {hit.x - panel->lower_left.x,
        hit.y - panel->lower_left.y, hit.z - panel->lower_left.z};
    double u = phi / (2 * half) + 0.5;
    double v = dot3(relative, panel->up) / up_length;
    *pixel_x = u * width - 0.5;
    *pixel_y = (1.0 - v) * height - 0.5;
    return isfinite(*pixel_x) && isfinite(*pixel_y);
  }
  FzeroTripleVec3 normal = {
      panel->right.y * panel->up.z - panel->right.z * panel->up.y,
      panel->right.z * panel->up.x - panel->right.x * panel->up.z,
      panel->right.x * panel->up.y - panel->right.y * panel->up.x};
  double denominator = dot3(normal, direction);
  double right_length = dot3(panel->right, panel->right);
  double up_length = dot3(panel->up, panel->up);
  if (!(fabs(denominator) > 1e-9 && right_length > 0 && up_length > 0))
    return false;
  double distance = dot3(normal, panel->lower_left) / denominator;
  if (!(distance > 0) || !isfinite(distance)) return false;
  FzeroTripleVec3 relative = {
      distance * direction.x - panel->lower_left.x,
      distance * direction.y - panel->lower_left.y,
      distance * direction.z - panel->lower_left.z};
  double u = dot3(relative, panel->right) / right_length;
  double v = dot3(relative, panel->up) / up_length;
  *pixel_x = u * width - 0.5;
  *pixel_y = (1.0 - v) * height - 0.5;
  return isfinite(*pixel_x) && isfinite(*pixel_y);
}

bool FzeroTripleCenterWarp(const FzeroTripleRig *rig, int columns, int rows,
                           float *uv) {
  FzeroTripleSurface panels[3];
  if (!rig || !uv || columns < 1 || rows < 1 || !(rig->curve_radius_mm > 0) ||
      !FzeroTripleBuild(rig, panels)) return false;
  FzeroTripleSurface glass = panels[1];
  glass.curve_radius_mm = rig->curve_radius_mm;
  for (int row = 0; row <= rows; ++row)
    for (int column = 0; column <= columns; ++column) {
      FzeroTripleVec3 point;
      double x, y;
      if (!FzeroTriplePanelPoint(&glass, (double)column / columns,
                                 1.0 - (double)row / rows, &point) ||
          !FzeroTripleProjectDirection(&panels[1], point, 1, 1, &x, &y))
        return false;
      uv[2 * (row * (columns + 1) + column)] = (float)(x + 0.5);
      uv[2 * (row * (columns + 1) + column) + 1] = (float)(y + 0.5);
    }
  return true;
}
