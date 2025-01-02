/**
 * @file
 *
 * @brief Dynamic Bit Array to store and access bits with indices.
 *        Prefer using this over std::vector<bool> for space efficiency.
 *
 * @copyright Unassemblize is free software: you can redistribute it and/or
 *            modify it under the terms of the GNU General Public License
 *            as published by the Free Software Foundation, either version
 *            3 of the License, or (at your option) any later version.
 *            A full copy of the GNU General Public License can be found in
 *            LICENSE
 */
#pragma once

#include <assert.h>
#include <cstdint>
#include <memory>

class BitArray
{
public:
    struct BitIndexer
    {
        friend class BitArray;

    private:
        BitIndexer(uint32_t bitIndex, uint8_t bitMask) : m_bitIndex(bitIndex), m_bitMask(bitMask) {}

        uint32_t m_bitIndex : 24;
        uint32_t m_bitMask : 8;
    };

    BitArray() = default;

    BitArray(uint32_t size, bool defaultValue = false) : m_size(size)
    {
        assert(m_size < (1 << 24)); // Has this limit to offer compact BitIndexer.
        const uint32_t bitsSize = m_size / 8 + 1;
        m_bitArray = std::make_unique<uint8_t[]>(bitsSize);
        std::fill_n(m_bitArray.get(), bitsSize, defaultValue);
    }

    bool is_set(BitIndexer indexer) const { return (m_bitArray[indexer.m_bitIndex] & indexer.m_bitMask) != uint8_t(0); }

    void set(BitIndexer indexer) { m_bitArray[indexer.m_bitIndex] |= indexer.m_bitMask; }

    void unset(BitIndexer indexer) { m_bitArray[indexer.m_bitIndex] &= ~indexer.m_bitMask; }

    BitIndexer get_indexer(uint32_t index) const { return BitIndexer(index / 8, 1 << (index % 8)); }

    uint32_t size() const { return m_size; }

private:
    std::unique_ptr<uint8_t[]> m_bitArray;
    uint32_t m_size = 0;
};
