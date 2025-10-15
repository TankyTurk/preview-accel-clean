// Dependency-free line rasterizer (no Qt); returns ARGB32 premultiplied bytes
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <vector>
#include <array>
#include <cstdint>
#include <cstdlib>

namespace py = pybind11;

// clamp int to [lo, hi]
static inline int clampi(int v, int lo, int hi){ return v < lo ? lo : (v > hi ? hi : v); }

// 1px Bresenham with simple "over" blend into ARGB32 premultiplied buffer
static void draw_line(std::uint8_t* buf, int W, int H, int x0, int y0, int x1, int y1,
                      std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    std::uint8_t pr = (std::uint8_t)((r * a + 127) / 255);
    std::uint8_t pg = (std::uint8_t)((g * a + 127) / 255);
    std::uint8_t pb = (std::uint8_t)((b * a + 127) / 255);

    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        if ((unsigned)x0 < (unsigned)W && (unsigned)y0 < (unsigned)H) {
            std::uint8_t* p = buf + ((size_t)y0 * (size_t)W + (size_t)x0) * 4;
            // dst already premul; src premul = (pr,pg,pb,a)
            // over: out = src + dst*(1-a)
            p[0] = (std::uint8_t)(pb + (p[0] * (255 - a)) / 255);
            p[1] = (std::uint8_t)(pg + (p[1] * (255 - a)) / 255);
            p[2] = (std::uint8_t)(pr + (p[2] * (255 - a)) / 255);
            p[3] = (std::uint8_t)(a  + (p[3] * (255 - a)) / 255);
        }
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

struct Seg { int x0, y0, x1, y1; };
struct FileSegments {
    std::vector<Seg> lines;
    std::array<int,4> rgba; // r,g,b,a (0..255)
    bool visible = true;
};

py::bytes rasterize_geometry(int view_w, int view_h,
                             const std::vector<FileSegments>& files) {
    if (view_w <= 0 || view_h <= 0) return py::bytes();
    std::vector<std::uint8_t> img((size_t)view_w * (size_t)view_h * 4, 0);

    for (const auto& f : files) {
        if (!f.visible || f.lines.empty()) continue;
        std::uint8_t r = (std::uint8_t)clampi(f.rgba[0],0,255);
        std::uint8_t g = (std::uint8_t)clampi(f.rgba[1],0,255);
        std::uint8_t b = (std::uint8_t)clampi(f.rgba[2],0,255);
        std::uint8_t a = (std::uint8_t)clampi(f.rgba[3],0,255);
        for (const auto& s : f.lines) {
            draw_line(img.data(), view_w, view_h, s.x0, s.y0, s.x1, s.y1, r, g, b, a);
        }
    }
    return py::bytes(reinterpret_cast<const char*>(img.data()), img.size());
}

PYBIND11_MODULE(preview_accel, m) {
    m.doc() = "Headless line rasterizer (ARGB32 premul) — no Qt dependency";
    py::class_<Seg>(m, "Seg")
        .def(py::init<>())
        .def_readwrite("x0",&Seg::x0).def_readwrite("y0",&Seg::y0)
        .def_readwrite("x1",&Seg::x1).def_readwrite("y1",&Seg::y1);
    py::class_<FileSegments>(m, "FileSegments")
        .def(py::init<>())
        .def_readwrite("lines",&FileSegments::lines)
        .def_readwrite("rgba",&FileSegments::rgba)
        .def_readwrite("visible",&FileSegments::visible);
    m.def("rasterize_geometry", &rasterize_geometry,
          py::arg("view_w"), py::arg("view_h"), py::arg("files"));
}
