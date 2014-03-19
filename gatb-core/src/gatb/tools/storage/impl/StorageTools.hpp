/*****************************************************************************
 *   GATB : Genome Assembly Tool Box
 *   Copyright (C) 2014  INRIA
 *   Authors: R.Chikhi, G.Rizk, E.Drezen
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

/** \file StorageTools.hpp
 *  \date 01/03/2013
 *  \author edrezen
 *  \brief Collection interface
 *
 *  This file holds interfaces related to the Collection interface
 */

#ifndef _GATB_CORE_TOOLS_STORAGE_IMPL_STORAGE_TOOLS_HPP_
#define _GATB_CORE_TOOLS_STORAGE_IMPL_STORAGE_TOOLS_HPP_

/********************************************************************************/

#include <gatb/tools/storage/impl/Storage.hpp>
#include <gatb/tools/collections/impl/Bloom.hpp>

/********************************************************************************/
namespace gatb      {
namespace core      {
namespace tools     {
namespace storage   {
namespace impl      {
/********************************************************************************/

class StorageTools
{
public:

    /** */
    static StorageTools& singleton() { static StorageTools instance; return instance; }


    /** */
    template<typename T>  void saveContainer (Group& group, const std::string& name, collections::Collection<T>* collection)
    {
        collections::Collection<T>* storageCollection = & group.getCollection<T> (name);

        collection->iterate ([&] (const T& t)  { storageCollection->insert (t); });
        storageCollection->flush ();
    }

    /** */
    template<typename T>  collections::Container<T>*  loadContainer (Group& group, const std::string& name)
    {
        collections::Collection<T>*  storageCollection = & group.getCollection<T> (name);
        return new collections::impl::ContainerSet<T> (storageCollection->iterator());
    }

    /** */
    template<typename T>  void saveBloom (Group& group, const std::string& name, collections::impl::Bloom<T>* bloom)
    {
        collections::Collection<math::NativeInt8>* bloomCollection = & group.getCollection<math::NativeInt8> (name);

        bloomCollection->insert ((math::NativeInt8*)bloom->getArray(), bloom->getSize());

        std::stringstream ss1;  ss1 <<  bloom->getBitSize();
        std::stringstream ss2;  ss2 <<  bloom->getNbHash();

        bloomCollection->addProperty ("size",    ss1.str());
        bloomCollection->addProperty ("nb_hash", ss2.str());
        bloomCollection->addProperty ("name",    bloom->getName());
    }

    /** */
    template<typename T>  collections::impl::Bloom<T>*  loadBloom (Group& group, const std::string& name)
    {
        /** We retrieve the raw data buffer for the Bloom filter. */
        tools::collections::Collection<tools::math::NativeInt8>* bloomArray = & group.getCollection<tools::math::NativeInt8> (name);

        /** We create the Bloom fiter. */
        tools::collections::impl::Bloom<T>* bloom = tools::collections::impl::BloomFactory::singleton().createBloom<T> (
            bloomArray->getProperty("name"),
            bloomArray->getProperty("size"),
            bloomArray->getProperty("nb_hash")
        );

        /** We set the bloom with the provided array given as an iterable of NativeInt8 objects. */
        bloomArray->getItems ((tools::math::NativeInt8*&)bloom->getArray());

        /** We return the result. */
        return bloom;
    }
};

/********************************************************************************/
} } } } } /* end of namespaces. */
/********************************************************************************/

#endif /* _GATB_CORE_TOOLS_STORAGE_IMPL_STORAGE_TOOLS_HPP_ */