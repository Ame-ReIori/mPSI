#pragma once

#include <cryptoTools/Common/block.h>
#include <libOTe/Tools/CoeffCtx.h>
#include <libOTe/Tools/Coproto.h>

#include <cstdint>
#include <vector>

using namespace osuCrypto;

int TTPOffline(uint8_t idx, uint8_t nparty, uint8_t t, uint64_t neles,
               std::vector<Socket> &chls, std::vector<block> &inner_okvs,
               std::vector<block> &a, block &b, std::vector<block> &c);

// offline phase of mulit-party psi
// outputs correleted randomness, including meta okvs and vole correlation
// int OfflineFullThreshold(uint8_t idx, uint8_t nparty, uint8_t t, uint64_t
// neles,
//                          std::vector<Socket> &chls,
//                          std::vector<block> &inner_okvs,
//                          std::vector<CoeffCtxGF2::Vec<block>> &as,
//                          std::vector<CoeffCtxGF2::Vec<block>> &cs,
//                          std::vector<CoeffCtxGF2::Vec<block>> &bs, block &d);
int OfflineFullThreshold(uint8_t idx, uint8_t nparty, uint8_t t, uint64_t neles,
                         std::vector<Socket> &chls,
                         std::vector<block> &inner_okvs, std::vector<block> &a,
                         block &b, std::vector<block> &c);

// offline phase of mulit-party psi
// outputs correleted randomness, including meta okvs and vole correlation
// int Offline(uint8_t idx, uint8_t nparty, uint8_t t, uint64_t neles,
//             std::vector<Socket> &chls, std::vector<block> &inner_okvs,
//             std::vector<CoeffCtxGF2::Vec<block>> &as,
//             std::vector<CoeffCtxGF2::Vec<block>> &cs,
//             std::vector<CoeffCtxGF2::Vec<block>> &bs, block &d);
int Offline(uint8_t idx, uint8_t nparty, uint8_t t, uint64_t neles,
            std::vector<Socket> &chls, std::vector<block> &inner_okvs,
            std::vector<block> &a, block &b, std::vector<block> &c);

// online phase of multi-party psi
// parties input set and output intersection
// int Online(uint8_t idx, uint8_t nparty, uint8_t t, std::vector<block> &in,
//            std::vector<Socket> &chls, std::vector<block> &inner_okvs,
//            std::vector<CoeffCtxGF2::Vec<block>> &as,
//            std::vector<CoeffCtxGF2::Vec<block>> &bs,
//            std::vector<CoeffCtxGF2::Vec<block>> &cs, block d,
//            std::vector<block> &out);
int Online(uint8_t idx, uint8_t nparty, uint8_t t, std::vector<block> &in,
           std::vector<Socket> &chls, std::vector<block> &inner_okvs,
           std::vector<block> &a, block &b, std::vector<block> &c,
           std::vector<block> &out);
