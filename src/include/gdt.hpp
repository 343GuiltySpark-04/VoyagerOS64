#pragma once

#define GDTAccessDPL(n) (n << 5)

namespace GDTAccessFlag
{

    enum GDTAccessFlag
    {

        ReadWrite = (2 << 1),
        DC = (1 << 2),
        Execute = (1 << 3),
        Segments = (1 << 4),
        Present = (1 << 7)

    };

}