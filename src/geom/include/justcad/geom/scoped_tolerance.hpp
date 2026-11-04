#pragma once

namespace justcad::geom {

/** Sets DoubleKernel's tolerance and restores the previous value on destruction. */
class ScopedTolerance {
public:
    explicit ScopedTolerance(double tolerance);
    ~ScopedTolerance();

    ScopedTolerance(const ScopedTolerance&) = delete;
    ScopedTolerance& operator=(const ScopedTolerance&) = delete;
    ScopedTolerance(ScopedTolerance&&) = delete;
    ScopedTolerance& operator=(ScopedTolerance&&) = delete;

private:
    double m_previous;
};

}  // namespace justcad::geom
