#include "utils.h"

#include <coproto/Common/macoro.h>
#include <coproto/Socket/AsioSocket.h>

#include <string>

int EstablishConnection(uint8_t idx, uint8_t nparty,
                        std::vector<Socket> &chls) {
  int kBasePort = 12222;
  int kPortStep = nparty + 1;
  chls.resize(nparty);

  for (int i = 0; i < nparty; ++i) {
    if (i > idx) {
      chls[i] = coproto::asioConnect(
          "localhost:" + std::to_string(kBasePort + kPortStep * i + idx), true);
    } else if (i < idx) {
      chls[i] = coproto::asioConnect(
          "localhost:" + std::to_string(kBasePort + kPortStep * idx + i),
          false);
    }
  }
  return 1;
}

int CloseConnection(uint8_t idx, std::vector<Socket> &chls) {
  for (int i = 0; i < chls.size(); ++i) {
    if (i != idx) {
      coproto::sync_wait(chls[i].flush());
      coproto::sync_wait(chls[i].close());
    }
  }
  return 1;
}
