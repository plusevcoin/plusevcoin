// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2011-2012 Litecoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/foreach.hpp>

#include "checkpoints.h"

#include "main.h"
#include "uint256.h"

namespace Checkpoints
{
    typedef std::map<int, uint256> MapCheckpoints;

    //
    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    //
    static MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of // Yo dawg, this is the secret. Checkpoint 0 hash == Genesis block hash.
        (         0, uint256("0xbdedb5b8c4e03e7b2603c1b156e2146a2b3920970b76c863b509b17e140c0c38"))
        (        42, uint256("0xa8943ceff56ad4dc52dad6d3a75e21230013d0f87a9dacc7d2a57b6946f11c68"))
        (       303, uint256("0xaec4e9b724e8a198f2f89cc6c1439f9c63607792ff71ecc20a13d61f5fb892f5"))
        (      1337, uint256("0x2140574f6730adbb11575fa554437a6989af71626bb0cbb2fc4bab8708fa2926"))
        (     11111, uint256("0x468f4ae0639f654312286edaabd59acb63c189c561532044bc68c6c98ff18ee9"))
        (     33333, uint256("0xba729b906fde5e8eacb3a2c8ec88b6e5983b35da95854bb4489884a8cfc59a45"))
        (     66666, uint256("0xeac8957764264c470bc77b9113c26a4bdfdb0bf7828e2f0e2ce0eaee42d71ede"))
        ;

    bool CheckBlock(int nHeight, const uint256& hash)
    {
        if (fTestNet) return true; // Testnet has no checkpoints

        MapCheckpoints::const_iterator i = mapCheckpoints.find(nHeight);
        if (i == mapCheckpoints.end()) return true;
        return hash == i->second;
    }

    int GetTotalBlocksEstimate()
    {
        if (fTestNet) return 0;
        return mapCheckpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const std::map<uint256, CBlockIndex*>& mapBlockIndex)
    {
        if (fTestNet) return NULL;

        BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type& i, mapCheckpoints)
        {
            const uint256& hash = i.second;
            std::map<uint256, CBlockIndex*>::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return NULL;
    }
}
