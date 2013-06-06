/*****************************************************************************
 *   GATB : Genome Assembly Tool Box                                         *
 *   Copyright (c) 2013                                                      *
 *                                                                           *
 *   GATB is free software; you can redistribute it and/or modify it under   *
 *   the CECILL version 2 License, that is compatible with the GNU General   *
 *   Public License                                                          *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
 *   CECILL version 2 License for more details.                              *
 *****************************************************************************/

/** \file Range.hpp
 *  \date 01/03/2013
 *  \author edrezen
 *  \brief Data structure
 */

#ifndef _GATB_CORE_TOOLS_MISC_DATA_HPP_
#define _GATB_CORE_TOOLS_MISC_DATA_HPP_

/********************************************************************************/

#include <gatb/tools/designpattern/api/SmartPointer.hpp>
#include <gatb/system/impl/System.hpp>
#include <gatb/tools/misc/api/Range.hpp>
#include <gatb/tools/misc/api/Vector.hpp>

#include <iostream>
#include <string.h>

/********************************************************************************/
namespace gatb      {
namespace core      {
namespace tools     {
namespace misc      {
/********************************************************************************/

/** \brief Definition of a data chunk
 *
 * A data is defined by:
 *      - an encoding format
 *      - a buffer holding the actual data
 *      - the size of the data
 *
 * It is implemented as a subclass of the Vector class, which allows to define Data
 * as a sub part of a referred Data instance.
 */
class Data : public Vector<char>
{
public:

    /** Define how data is encoded. */
    enum Encoding_e
    {
        /** data encoded as ASCII codes (so one byte per data unit) */
        ASCII,
        /** one byte per data as integer value (for instance: A=0, C=1, T=2, G=3) */
        INTEGER,
        /** 4 nucleotides compressed in one byte */
        BINARY
    };

    /** Default constructor. */
    Data (Encoding_e encode = BINARY)  : encoding(encode) {}

    /** Constructor. */
    Data (size_t len)  : Vector<char>(len), encoding(BINARY)  {}

    /** \copydoc Vector<char>::setRef */
    void setRef (Data* ref, size_t offset, size_t length)
    {
        /** We call the parent method. */
        Vector<char>::setRef (ref, offset, length);

        /** We set the encoding. */
        encoding = ref->getEncoding();
    }

    /** \return format of the data. */
    Encoding_e getEncoding ()  const  { return encoding; }

    /** Conversion from one encoding scheme to another.
     *  TO BE IMPROVED (support only one kind of conversion, from binary to ascii)
     * \param[in] in  : input data
     * \param[in] out : output data */
    static void convert (Data& in, Data& out)
    {
        size_t nchar = (in.size()+3)/4;
        size_t j=0;
        for (size_t i=0; i<nchar; i++)
        {
            char fournt = in[i];
            out[j+3] = fournt & 3; fournt = fournt >> 2;
            out[j+2] = fournt & 3; fournt = fournt >> 2;
            out[j+1] = fournt & 3; fournt = fournt >> 2;
            out[j+0] = fournt & 3;
            j+=4;
        }

        out.encoding = Data::INTEGER;
        out.setSize (in.size());
    }

private:

    /** Encoding scheme of the data instance. */
    Encoding_e  encoding;
};

/********************************************************************************/
} } } } /* end of namespaces. */
/********************************************************************************/

#endif /* _GATB_CORE_TOOLS_MISC_DATA_HPP_ */