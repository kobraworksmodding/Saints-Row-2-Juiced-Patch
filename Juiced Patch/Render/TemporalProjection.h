#pragma once
#include <array>
#include <cmath>
#include <algorithm>

namespace TemporalAA::Projection
{
    using Matrix = std::array<float,16>;
    inline Matrix Multiply(const Matrix& a, const Matrix& b)
    {
        Matrix result{};
        for (unsigned r=0;r<4;++r)
            for (unsigned c=0;c<4;++c)
                for (unsigned k=0;k<4;++k)
                    result[r*4+c] += a[r*4+k]*b[k*4+c];
        return result;
    }
    // Solve current * reprojection = previous in double precision. Rounding the
    // inverse to float before multiplying large world translations creates false
    // screen motion even for an unchanged camera. Cast only the final result.
    inline bool Reproject(const Matrix& current, const Matrix& previous, Matrix& result)
    {
        for (unsigned i=0; i<16; ++i)
            if (!std::isfinite(current[i]) || !std::isfinite(previous[i])) return false;
        double a[4][8]{};
        for (unsigned r=0; r<4; ++r)
            for (unsigned c=0; c<4; ++c)
            {
                a[r][c]=current[r*4+c];
                a[r][c+4]=previous[r*4+c];
            }
        for (unsigned c=0; c<4; ++c)
        {
            unsigned pivot=c;
            for (unsigned r=c+1; r<4; ++r)
                if (std::abs(a[r][c])>std::abs(a[pivot][c])) pivot=r;
            if (std::abs(a[pivot][c])<1e-12) return false;
            if (pivot!=c) for (unsigned k=0; k<8; ++k) std::swap(a[pivot][k],a[c][k]);
            const double divisor=a[c][c];
            for (double& value:a[c]) value/=divisor;
            for (unsigned r=0; r<4; ++r)
                if (r!=c)
                {
                    const double factor=a[r][c];
                    for (unsigned k=0; k<8; ++k) a[r][k]-=factor*a[c][k];
                }
        }
        for (unsigned r=0; r<4; ++r)
            for (unsigned c=0; c<4; ++c)
            {
                result[r*4+c]=current==previous ? float(r==c) : static_cast<float>(a[r][c+4]);
                if (!std::isfinite(result[r*4+c])) return false;
            }
        return true;
    }
    // SR2 stores row-vector matrices on the CPU, transposed c4-c7 on the GPU.
    // Shift clip x/y by clip w, retaining depth and perspective unchanged.
    inline void Shift(Matrix& matrix, float x, float y)
    {
        for (unsigned r=0;r<4;++r)
        {
            matrix[r*4] += x*matrix[r*4+3];
            matrix[r*4+1] += y*matrix[r*4+3];
        }
    }
    inline void ShiftConstants(Matrix& matrix, float x, float y)
    {
        for (unsigned c=0;c<4;++c)
        {
            matrix[c] += x*matrix[12+c];
            matrix[4+c] += y*matrix[12+c];
        }
    }
    inline void RefreshCachedClip(Matrix& clip, const Matrix& world, const Matrix& worldClip)
    {
        const auto rebuilt=Multiply(world,worldClip);
        // Keep SR2's cached z/w bit-for-bit; only x/y depend on jitter.
        for (unsigned r=0;r<4;++r)
        {
            clip[r*4]=rebuilt[r*4];
            clip[r*4+1]=rebuilt[r*4+1];
        }
    }
}
