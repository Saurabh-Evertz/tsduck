//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//
//  TSUnit test suite for CRC32.
//
//----------------------------------------------------------------------------

#include "tsCRC32.h"
#include "tsunit.h"
#include "utestTSUnitBenchmark.h"


//----------------------------------------------------------------------------
// The test fixture
//----------------------------------------------------------------------------

class CRC32Test: public tsunit::Test
{
public:
    virtual void beforeTest() override;
    virtual void afterTest() override;

    void testCRC();

    TSUNIT_TEST_BEGIN(CRC32Test);
    TSUNIT_TEST(testCRC);
    TSUNIT_TEST_END();
};

TSUNIT_REGISTER(CRC32Test);


//----------------------------------------------------------------------------
// Initialization.
//----------------------------------------------------------------------------

// Test suite initialization method.
void CRC32Test::beforeTest()
{
}

// Test suite cleanup method.
void CRC32Test::afterTest()
{
}


//----------------------------------------------------------------------------
// Unitary tests.
//----------------------------------------------------------------------------

namespace {
    struct TestData {
        uint32_t crc;
        size_t   data_size;
        uint8_t  data[512];
    };
    const TestData all_data[] = {
        {
            0x38B4E5A5, 2,
            {
                0xA9, 0x79,
            }
        },
        {
            0x365917CA, 3,
            {
                0x78, 0x75, 0x43,
            }
        },
        {
            0x7B6D7B6B, 47,
            {
                0x80, 0x93, 0x71, 0x0D, 0x02, 0x3B, 0xC0, 0x30, 0x39, 0x79, 0x3D, 0xFD, 0xC8, 0x7A, 0x80, 0x92,
                0x1C, 0xFE, 0x81, 0x5B, 0x21, 0xD5, 0x65, 0xEE, 0x8B, 0x65, 0xB7, 0xFA, 0x21, 0x5A, 0xDD, 0x02,
                0xF8, 0xF1, 0xBF, 0xD2, 0x4D, 0x2D, 0x34, 0x3B, 0x5F, 0x5C, 0x73, 0x34, 0x2D, 0x5C, 0x44,
            }
        },
        {
            0xAD8A57E1, 127,
            {
                0xE3, 0x46, 0x3F, 0xB8, 0x85, 0xE1, 0x83, 0xD1, 0x4A, 0xA9, 0x6A, 0x17, 0x82, 0xD5, 0x25, 0x52,
                0x24, 0x60, 0x4C, 0x45, 0x5D, 0x55, 0xBC, 0xE0, 0x40, 0x59, 0xAC, 0x3E, 0x9A, 0xDC, 0x3E, 0xD7,
                0x03, 0x1E, 0x5D, 0xCB, 0x64, 0x06, 0xB5, 0x02, 0x94, 0xD6, 0xB4, 0x33, 0xE1, 0xA0, 0x29, 0xFF,
                0xFF, 0x69, 0x77, 0xB2, 0x86, 0x3C, 0x2D, 0xAB, 0x6E, 0x8C, 0xA6, 0xDB, 0xFE, 0xCB, 0x30, 0x69,
                0x72, 0x0A, 0x0D, 0xED, 0xD0, 0x8F, 0x98, 0x6C, 0x86, 0x2E, 0x61, 0x88, 0x78, 0x68, 0xA3, 0xAB,
                0xFC, 0x04, 0x72, 0x7B, 0xC8, 0xF0, 0xED, 0xEE, 0xA1, 0x3A, 0x52, 0x0C, 0x47, 0x8B, 0xE7, 0xC3,
                0x00, 0x5C, 0xCD, 0x34, 0x13, 0x5C, 0x29, 0x15, 0x04, 0x29, 0xF3, 0x5B, 0xDF, 0xB0, 0x26, 0x92,
                0xAA, 0x2D, 0x1C, 0x7F, 0x5D, 0xCC, 0xF1, 0x76, 0xB5, 0x40, 0xF5, 0x22, 0xDC, 0x59, 0xD6,
            }
        },
        {
            0x1778E1E3, 256,
            {
                0x2F, 0xFE, 0x37, 0x30, 0x24, 0x79, 0xE2, 0x5A, 0x5B, 0xB9, 0x66, 0xB6, 0x70, 0x0D, 0x5E, 0x9A,
                0xEE, 0x70, 0x3C, 0x3F, 0xDA, 0x53, 0x28, 0x92, 0x93, 0x0C, 0x75, 0x11, 0x8B, 0x9F, 0xCB, 0xA7,
                0x0F, 0xF1, 0xFD, 0x4F, 0x59, 0xD0, 0xB2, 0x63, 0x15, 0x44, 0xBF, 0xAC, 0x78, 0xF7, 0x57, 0x21,
                0x58, 0x12, 0x0E, 0xDA, 0x63, 0xD5, 0x70, 0x47, 0x6C, 0x62, 0x58, 0xA2, 0xEE, 0x6A, 0xDB, 0x03,
                0x44, 0xF0, 0x54, 0xBC, 0x33, 0x2F, 0xC4, 0x5C, 0xC0, 0x84, 0x9D, 0xDF, 0x54, 0x18, 0x4A, 0xA6,
                0xD7, 0x1A, 0x46, 0x19, 0xAA, 0x5F, 0x7F, 0xEA, 0x92, 0x46, 0xF2, 0xED, 0xE5, 0x0F, 0x26, 0xB7,
                0xF9, 0x06, 0x37, 0x68, 0xD4, 0x79, 0xF0, 0x90, 0x9E, 0x2C, 0x3D, 0x94, 0x1F, 0xE1, 0xBB, 0x62,
                0x26, 0x6B, 0x1D, 0xEE, 0xA3, 0xC9, 0xA3, 0xCB, 0x9E, 0x7E, 0xDC, 0xCE, 0x66, 0xC3, 0x6A, 0x27,
                0xB2, 0x7A, 0x09, 0x82, 0x6C, 0xD2, 0xCD, 0xEA, 0x35, 0x9B, 0x06, 0xC8, 0xAA, 0xDC, 0x2B, 0xAF,
                0xAB, 0xBD, 0xF0, 0xD9, 0xA2, 0x7F, 0x9B, 0x4A, 0xFE, 0xB8, 0xDB, 0xCB, 0xF9, 0x12, 0xFF, 0xA2,
                0x2B, 0xE4, 0xF6, 0x03, 0x75, 0xBB, 0x6C, 0x43, 0x6C, 0x8E, 0x0B, 0x55, 0xD2, 0xCD, 0x25, 0x7F,
                0xAB, 0x2F, 0x4F, 0x09, 0x83, 0xC0, 0xE7, 0xAA, 0xAF, 0x06, 0xC0, 0xC7, 0x7E, 0x46, 0xF4, 0x6B,
                0xB2, 0x8D, 0xF2, 0xAA, 0xDC, 0xB8, 0x59, 0xA6, 0x29, 0x3E, 0xEA, 0xB7, 0x51, 0x95, 0x0D, 0xED,
                0x9D, 0x3A, 0x3C, 0xA7, 0x97, 0xFD, 0x4C, 0xEE, 0xBD, 0xA0, 0x55, 0xB2, 0xD8, 0x28, 0x75, 0x25,
                0xA2, 0x9E, 0xA1, 0x0C, 0x7B, 0x8B, 0x12, 0x9E, 0xDC, 0xC2, 0xD3, 0xA7, 0xA1, 0x23, 0x8A, 0x13,
                0x0E, 0xFF, 0x42, 0x70, 0xAF, 0x5B, 0xEA, 0x2E, 0xA0, 0x6A, 0xBE, 0xB5, 0x69, 0x87, 0xEF, 0x3E,
            }
        },
        {
            0xC404A838, 450,
            {
                0x1D, 0xEF, 0xC5, 0x56, 0xC0, 0xFD, 0x56, 0xFF, 0x6D, 0xEC, 0x54, 0x20, 0x6F, 0xD8, 0x99, 0x98,
                0xC6, 0x07, 0xC7, 0x2B, 0xF5, 0x95, 0x3E, 0x06, 0x1C, 0x0C, 0x94, 0x23, 0x75, 0x39, 0x16, 0xF1,
                0xC7, 0xF1, 0x0A, 0xF7, 0x25, 0x23, 0x1D, 0x5D, 0xA7, 0x62, 0x2D, 0x9C, 0x4F, 0xE3, 0x96, 0x40,
                0xB8, 0x42, 0x5C, 0xA2, 0xA6, 0xC6, 0x21, 0x1F, 0x40, 0x2D, 0xC1, 0xA6, 0x15, 0x09, 0x3B, 0x01,
                0x1B, 0xC7, 0x47, 0xA5, 0xF6, 0xE0, 0x23, 0xAA, 0x82, 0x8D, 0x93, 0x41, 0xD2, 0x81, 0x4D, 0x5C,
                0xBF, 0x11, 0x50, 0x84, 0xAF, 0x44, 0x3A, 0xBD, 0x80, 0x4B, 0xB9, 0xD7, 0xD6, 0x0B, 0xD5, 0xDD,
                0xCF, 0x65, 0xE6, 0x8D, 0xAC, 0xF4, 0xE1, 0x2D, 0xD2, 0x4D, 0xF7, 0x62, 0x2D, 0xB7, 0x82, 0xA5,
                0x9F, 0x4C, 0xFC, 0xDF, 0x82, 0x0E, 0x52, 0x69, 0xC8, 0x70, 0x74, 0xF9, 0x02, 0x64, 0x02, 0xC1,
                0x16, 0x5C, 0x08, 0x1F, 0x97, 0xD1, 0x37, 0x5F, 0x34, 0xEE, 0x30, 0x30, 0x4D, 0xBD, 0x35, 0xC0,
                0xD0, 0x2D, 0x9A, 0x29, 0xC3, 0xC1, 0xC9, 0x74, 0x74, 0xE7, 0xD9, 0x86, 0x02, 0x21, 0x57, 0x2B,
                0x43, 0x32, 0xE3, 0xF6, 0xB1, 0xF1, 0x19, 0xA2, 0x68, 0xF4, 0x5D, 0xA8, 0x88, 0xED, 0xA2, 0xED,
                0x5F, 0x36, 0x77, 0x0F, 0x29, 0x09, 0x95, 0x17, 0xA7, 0xEB, 0x92, 0xFF, 0xBB, 0xA2, 0x85, 0x87,
                0x3E, 0xD1, 0x3B, 0xE5, 0x74, 0x2B, 0xDE, 0x4F, 0x0C, 0x8E, 0xD2, 0x91, 0x8D, 0x08, 0xF1, 0x1E,
                0x37, 0x90, 0xC2, 0xA5, 0xA4, 0x4E, 0x2E, 0x06, 0x48, 0x0E, 0xBB, 0x31, 0x8D, 0x9C, 0xC7, 0x48,
                0xE7, 0xE4, 0xB6, 0xAF, 0x1E, 0xA4, 0x1E, 0x57, 0xC1, 0x65, 0x46, 0xF8, 0xE2, 0x80, 0xBA, 0xE9,
                0x05, 0x0C, 0xB1, 0xF5, 0xA1, 0x7F, 0x13, 0x22, 0xCB, 0x30, 0x8D, 0x13, 0x07, 0xAA, 0x23, 0x43,
                0x34, 0xD8, 0x93, 0xF3, 0xB7, 0xC7, 0x2D, 0xA0, 0x2C, 0x5B, 0xC4, 0x43, 0x7A, 0x57, 0x26, 0x13,
                0xCB, 0xBE, 0xD8, 0x3F, 0x11, 0x77, 0xC6, 0xA6, 0xC4, 0xC2, 0xDC, 0x61, 0xC4, 0x96, 0xBE, 0xA0,
                0xA2, 0xBF, 0xF2, 0x01, 0x95, 0xB4, 0xF1, 0xFD, 0x5C, 0x9D, 0x4F, 0xAF, 0xE3, 0xE1, 0x40, 0x04,
                0xCE, 0x33, 0x36, 0xC5, 0xBC, 0x76, 0xE6, 0xD9, 0x37, 0x8E, 0xE8, 0x9A, 0x14, 0x65, 0x45, 0xA7,
                0x6F, 0xEA, 0x7F, 0xCD, 0x13, 0x25, 0x1D, 0x71, 0x5D, 0xA8, 0x3A, 0x66, 0xB7, 0x22, 0xCF, 0x14,
                0xC4, 0x04, 0x97, 0x39, 0x1E, 0xD7, 0x24, 0x95, 0x3E, 0x8F, 0xDE, 0x87, 0x5F, 0x6B, 0xEF, 0x36,
                0x42, 0xF4, 0xA9, 0x9D, 0x3C, 0x94, 0xBE, 0x84, 0xF9, 0xA8, 0xBB, 0xF0, 0x0B, 0x89, 0x57, 0x78,
                0xB6, 0x9B, 0xE8, 0xF5, 0x8C, 0xAC, 0x30, 0x93, 0x11, 0xAD, 0xA1, 0x1D, 0x4B, 0xF2, 0x39, 0x5D,
                0x86, 0x0B, 0x1C, 0xBB, 0x0A, 0x75, 0x59, 0xC9, 0x48, 0x34, 0x1B, 0x15, 0x65, 0xE2, 0xB1, 0x1F,
                0xAA, 0x40, 0x72, 0x87, 0xA2, 0x6C, 0x4B, 0x11, 0xF8, 0xB5, 0xAB, 0x4D, 0x86, 0x32, 0xE7, 0x40,
                0x55, 0xE7, 0x77, 0x5E, 0xED, 0xF1, 0x5C, 0xB0, 0x86, 0xFA, 0xF1, 0xBC, 0xF9, 0x74, 0xDA, 0xEF,
                0x8F, 0x39, 0xA5, 0xF1, 0x0A, 0x5E, 0x7E, 0xA3, 0xD2, 0x52, 0x5B, 0xAF, 0xC6, 0x8C, 0xF3, 0xF0,
                0x7A, 0x82,
            }
        },
        {
            0, 0, {}
        }
    };
}

void CRC32Test::testCRC()
{
    // Support for benchmarking.
    utest::TSUnitBenchmark bench(u"TSUNIT_CRC32_ITERATIONS");

    for (const auto* data = all_data; data->data_size != 0; ++data) {

        // Test in one chunk.
        ts::CRC32 c;
        bench.start();
        for (size_t iter = 0; iter < bench.iterations; ++iter) {
            c.reset();
            c.add(data->data, data->data_size);
        }
        bench.stop();
        TSUNIT_EQUAL(data->crc, c.value());

        // Test in 3 chunks.
        const size_t chunk_size = data->data_size / 3;
        c.reset();
        c.add(data->data, chunk_size);
        c.add(data->data + chunk_size, chunk_size);
        c.add(data->data + 2 * chunk_size, data->data_size - 2 * chunk_size);
        TSUNIT_EQUAL(data->crc, c.value());
    }

    bench.report(u"CRC32Test::testCRC");
}
