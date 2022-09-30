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

#define GDTKernelBaseSelector 0x08
#define GDTUserBaseSelector 0x18
#define GDTTSSSegment 0x30

#define GDTAccessKernelCode (GDTAccessFlag::ReadWrite | GDTAccessFlag::Execute | GDTAccessFlag::Segments | GDTAccessFlag::Present)
#define GDTAccessKernelData (GDTAccessFlag::ReadWrite | GDTAccessFlag::Segments | GDTAccessFlag::Present)
#define GDTAccessUserCode (GDTAccessFlag::ReadWrite | GDTAccessFlag::Execute | GDTAccessFlag::Segments | GDTAccessDPL(3) | GDTAccessFlag::Present)
#define GDTAccessUserData (GDTAccessFlag::ReadWrite | GDTAccessFlag::Segments | GDTAccessDPL(3) | GDTAccessFlag::Present)