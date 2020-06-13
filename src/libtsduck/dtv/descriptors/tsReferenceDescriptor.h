//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2020, Thierry Lelegard
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Representation of an ISDB reference_descriptor.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsAbstractDescriptor.h"

namespace ts {
    //!
    //! Representation of an ISDB reference_descriptor.
    //! @see ARIB STD-B10, Part 3, 5.2.2
    //! @ingroup descriptor
    //!
    class TSDUCKDLL ReferenceDescriptor : public AbstractDescriptor
    {
    public:
        //!
        //! Reference entry.
        //!
        struct TSDUCKDLL Reference
        {
            Reference();                     //!< Constructor.
            uint16_t reference_node_id;      //!< Reference node id.
            uint8_t  reference_number;       //!< Reference number.
            uint8_t  last_reference_number;  //!< Last reference number.
        };

        typedef std::list<Reference> ReferenceList;  //!< List of references.

        // ReferenceDescriptor public members:
        uint16_t      information_provider_id;  //!< Information provider id.
        uint16_t      event_relation_id;        //!< Event relation id.
        ReferenceList references;               //!< List of references.

        //!
        //! Default constructor.
        //!
        ReferenceDescriptor();

        //!
        //! Constructor from a binary descriptor
        //! @param [in,out] duck TSDuck execution context.
        //! @param [in] bin A binary descriptor to deserialize.
        //!
        ReferenceDescriptor(DuckContext& duck, const Descriptor& bin);

        // Inherited methods
        virtual void serialize(DuckContext&, Descriptor&) const override;
        virtual void deserialize(DuckContext&, const Descriptor&) override;
        virtual void fromXML(DuckContext&, const xml::Element*) override;
        DeclareDisplayDescriptor();

    protected:
        // Inherited methods
        virtual void buildXML(DuckContext&, xml::Element*) const override;
    };
}
