#pragma once
// Colour science for Bloob's calibration (CALIBRATION-PLAN.md §6): XYZ and
// chromaticity, CIELAB and CIEDE2000, the video transfer functions (BT.1886,
// power, PQ), and ICtCp with ΔE ITP (BT.2124) for HDR.
// All luminance is absolute (cd/m²) unless a function says otherwise.

namespace cal {

struct Vec3 {
  double x = 0, y = 0, z = 0;
  double &operator[](int i) { return i == 0 ? x : i == 1 ? y : z; }
  double operator[](int i) const { return i == 0 ? x : i == 1 ? y : z; }
};
inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator*(Vec3 a, double k) { return {a.x * k, a.y * k, a.z * k}; }
inline Vec3 operator*(double k, Vec3 a) { return a * k; }
inline Vec3 operator/(Vec3 a, double k) { return {a.x / k, a.y / k, a.z / k}; }
inline bool operator==(const Vec3 &a, const Vec3 &b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
inline bool operator!=(const Vec3 &a, const Vec3 &b) { return !(a == b); }

struct Mat3 {
  double m[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
  static Mat3 columns(Vec3 a, Vec3 b, Vec3 c);
  Vec3 operator*(const Vec3 &v) const;
  Mat3 operator*(const Mat3 &o) const;
  double det() const;
  Mat3 inverse() const;  // identity when singular
};

// Chromaticities of a colour space: red, green, blue, white.
struct Primaries {
  double rx, ry, gx, gy, bx, by, wx, wy;
};
extern const Primaries kRec709, kBt2020, kP3D65;
constexpr double kD65x = 0.3127, kD65y = 0.3290;

Vec3 xyY(double x, double y, double Y);
void chromaticity(const Vec3 &XYZ, double *x, double *y);
// Distance between two colours' chromaticities in CIE 1976 u′v′.
double duv(const Vec3 &a, const Vec3 &b);
// Linear RGB (white = 1,1,1) to XYZ with the white's Y = 1.
Mat3 rgb_to_xyz(const Primaries &p);

Vec3 lab(const Vec3 &XYZ, const Vec3 &white);
double de2000(const Vec3 &lab1, const Vec3 &lab2);
// ΔE2000 between two absolute XYZ colours, relative to `white` (the target white).
double de2000_xyz(const Vec3 &a, const Vec3 &b, const Vec3 &white);

// Transfer functions. Signals are in [0, 1].
double bt1886(double v, double Lw, double Lb);
double power_eotf(double v, double gamma, double Lw, double Lb);
double pq_eotf(double e);       // signal → cd/m²
double pq_inverse(double L);    // cd/m² → signal

// ICtCp (BT.2100) of an absolute XYZ colour, and ΔE ITP (BT.2124) between two.
Vec3 ictcp(const Vec3 &XYZ);
double de_itp(const Vec3 &a, const Vec3 &b);

}  // namespace cal
