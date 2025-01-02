#pragma once

struct NoCopy
{
    NoCopy() = default;
    NoCopy(const NoCopy &) = delete;
    NoCopy &operator=(const NoCopy &) = delete;
};

struct NoCopyNoMove
{
    NoCopyNoMove() = default;
    NoCopyNoMove(const NoCopyNoMove &) = delete;
    NoCopyNoMove(NoCopyNoMove &&) = delete;
    NoCopyNoMove &operator=(const NoCopyNoMove &) = delete;
    NoCopyNoMove &operator=(NoCopyNoMove &&) = delete;
};
