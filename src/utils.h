#pragma once

#include <libOTe/Tools/Coproto.h>

#include <cstdint>
#include <vector>

using namespace osuCrypto;

int EstablishConnection(uint8_t idx, uint8_t nparty, std::vector<Socket> &chls);

int CloseConnection(uint8_t idx, std::vector<Socket> &chls);
