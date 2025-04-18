#include <coproto/Common/macoro.h>
#include <coproto/Socket/AsioSocket.h>
#include <coproto/Socket/BufferingSocket.h>
#include <cryptoTools/Common/CLP.h>
#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Common/Timer.h>
#include <cryptoTools/Common/block.h>
#include <cryptoTools/Crypto/PRNG.h>
#include <libOTe/Tools/CoeffCtx.h>
#include <libOTe/TwoChooseOne/ConfigureCode.h>
#include <libOTe/Vole/Silent/SilentVoleReceiver.h>
#include <libOTe/Vole/Silent/SilentVoleSender.h>
#include <macoro/coroutine_handle.h>
#include <macoro/thread_pool.h>
#include <unistd.h>
#include <volePSI/Paxos.h>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <thread>
#include <vector>

#include "defines.h"
#include "mpsi.h"
#include "utils.h"

using namespace osuCrypto;

void tVole(const CLP &cmd) {
  int party = cmd.getOr("id", 0);
  uint64_t n = 1 << 20;
  PRNG prng(sysRandomSeed());
  using F = block;
  using G = block;
  using Ctx = CoeffCtxGF2;

  using VecF = typename Ctx::template Vec<F>;
  using VecG = typename Ctx::template Vec<G>;

  if (!party) {
    // SilentVoleReceiver
    Socket chl = coproto::asioConnect("localhost:12123", false);
    SilentVoleReceiver<block, block, CoeffCtxGF2> recv;
    SilentVoleSender<block, block, CoeffCtxGF2> send;
    recv.mMultType = DefaultMultType;
    recv.mDebug = false;
    send.mMultType = DefaultMultType;
    send.mDebug = false;
    VecF a(n);
    VecG c(n);
    coproto::sync_wait(recv.silentReceive(c, a, prng, chl));
    std::cout << "a: " << a[n - 1] << " c:" << c[n - 1] << "\n";
    VecF b(n);
    block d = prng.get();
    coproto::sync_wait(send.silentSend(d, b, prng, chl));
    std::cout << "b: " << b[n - 1] << " d: " << d << "\n";
  } else {
    Socket chl = coproto::asioConnect("localhost:12123", true);
    SilentVoleSender<block, block, CoeffCtxGF2> send;
    SilentVoleReceiver<block, block, CoeffCtxGF2> recv;
    send.mMultType = DefaultMultType;
    send.mDebug = false;
    recv.mMultType = DefaultMultType;
    recv.mDebug = false;
    VecF b(n);
    block d = prng.get();
    coproto::sync_wait(send.silentSend(d, b, prng, chl));
    std::cout << "b: " << b[n - 1] << " d: " << d << "\n";
    VecF a(n);
    VecG c(n);
    coproto::sync_wait(recv.silentReceive(c, a, prng, chl));
    std::cout << "a: " << a[n - 1] << " c:" << c[n - 1] << "\n";
  }
}

void tOKVS() {
  Timer timer;
  uint64_t n = 1 << 20;
  PRNG prng(sysRandomSeed());
  volePSI::Baxos baxos;
  baxos.init(n, BIN_SIZE, CUCKOO_HASH_NUM, LAMBDA, volePSI::PaxosParam::GF128,
             AllOneBlock);

  PRNG prng0(sysRandomSeed());
  PRNG prng1(sysRandomSeed());
  std::vector<block> x0(n);
  std::vector<block> y0(n);
  std::vector<block> x1(n);
  std::vector<block> y1(n);

  prng0.get<block>(x0);
  prng0.get<block>(y0);

  std::vector<block> d0(baxos.size());
  std::vector<block> d1(baxos.size());
  baxos.solve<block>(x0, y0, d0);

  prng1.get<block>(x1);

  for (int i = 0; i < 50; ++i) {
    x1[i] = x0[i + 100];
  }

  baxos.decode<block>(x1, y1, d0);
  baxos.solve<block>(x1, y1, d1);

  std::vector<block> c(baxos.size());
  std::vector<block> v(n);
  std::vector<block> c_(baxos.size());
  std::vector<block> v_(n);

  for (int i = 0; i < baxos.size(); ++i) {
    c[i] = d0[i] ^ d1[i];
  }

  // test availablity of c
  baxos.decode<block>(x0, v, c);
  for (int i = 0; i < 10; ++i) {
    std::cout << c[i] << " " << v[i] << "\n";
  }

  int count = 0;
  for (int i = 0; i < v.size(); ++i) {
    if (v[i] == ZeroBlock) count++;
  }
  std::cout << count << "\n";

  // do multiplication
  block b = prng.get();

  // 1. open

  std::vector<block> o0(baxos.size());
  std::vector<block> o1(baxos.size());
  std::vector<block> o(baxos.size());
  // test availablity of c_
  
  for (int i = 0; i < baxos.size(); ++i) {
    o0[i] = d0[i].gf128Mul(b);
    o1[i] = d1[i].gf128Mul(b);
  }

  for (int i = 0; i < baxos.size(); ++i) {
    c_[i] = o0[i] ^ o1[i];
  }

  baxos.decode<block>(x0, v_, c_);

  int count_ = 0;
  for (int i = 0; i < v.size(); ++i) {
    if (((v[i].gf128Mul(b)) != v_[i])) {
      std::cout << "error\n";
      break;
    }
  }
  std::cout << count_ << "\n";
}

void tNetwork(const CLP &cmd) {
  Timer timer;
  int n = cmd.getOr("n", 15);
  int t = cmd.getOr("t", 10);
  int idx = cmd.getOr("id", n - 1);
  int threshold = n - t - 1;
  std::vector<Socket> chls;

  timer.setTimePoint("before connect");
  EstablishConnection(idx, n, chls);
  timer.setTimePoint("after connect");

  if (idx < threshold) {
    std::vector<int> rnd(t + 1);
    std::vector<std::thread> net_threads(t + 1);

    for (int i = 0; i < t + 1; ++i) {
      rnd[i] = idx * 10000 + threshold + i;
      net_threads[i] = std::thread([&, i]() {
        coproto::sync_wait(chls[threshold + i].send(rnd[i]));
        return;
      });
    }

    for (auto &thrd : net_threads) {
      thrd.join();
    }
  } else {
    std::vector<int> rnd(threshold);
    std::vector<std::thread> net_threads(threshold);

    for (int i = 0; i < threshold; ++i) {
      net_threads[i] = std::thread([&, i]() {
        coproto::sync_wait(chls[i].recv(rnd[i]));
        return;
      });
    }
    for (auto &thrd : net_threads) {
      thrd.join();
    }

    for (int i = 0; i < threshold; i++) {
      std::cout << idx << " " << rnd[i] << "\n";
    }
  }
  if (idx == 0) std::cout << timer << "\n";
  CloseConnection(idx, chls);
}

struct BlockCmp {
  bool operator()(block &a, block &b) const { return a == b; };
};

void tMPSI(const CLP &cmd) {
  Timer timer;
  int nparty = cmd.getOr("n", 15);
  int t = cmd.getOr("t", 10);
  int idx = cmd.getOr("id", nparty - 1);
  int logn = cmd.getOr("m", 20);
  int neles = 1 << logn;
  int nint = cmd.getOr("i", 1000);
  int ttp = cmd.getOr("ttp", 0);
  int threshold = nparty - t - 1;

  size_t offline_comm = 0;
  size_t offline_comm_recv = 0;
  size_t online_comm = 0;
  size_t online_comm_recv = 0;

  std::vector<Socket> chls;
  std::vector<block> inner_okvs;
  std::vector<block> in(neles);
  std::vector<block> out;
  std::vector<block> a;
  block b;
  std::vector<block> c;

  PRNG prng(sysRandomSeed());
  prng.get<block>(in);
  PRNG inter_prng(ZeroBlock);

  for (int i = 0; i < nint; ++i) {
    in[(i + idx) % neles] = inter_prng.get();
  }

  timer.setTimePoint("start");

  EstablishConnection(idx, nparty, chls);

  timer.setTimePoint("connection");

  Offline(ttp, idx, nparty, t, neles, chls, inner_okvs, a, b, c);

  if (idx == 0) {
    if (t < nparty - 1) {
      timer.setTimePoint("outer offline");
    }
  } else if (idx == nparty - 2) {
    timer.setTimePoint("inner offline");
  } else if (idx == nparty - 1) {
    timer.setTimePoint("pivot offline");
  }

  if (idx == 0) {
    for (int i = 1; i < nparty; ++i) {
      coproto::sync_wait(chls[i].flush());
      offline_comm += chls[i].bytesSent();
      offline_comm_recv += chls[i].bytesReceived();
    }
    if (t < nparty - 1) {
      std::cout << "[Offline] Outer Comm. (Sent): "
                << offline_comm / 1024.0 / 1024 << " MB\n";
      std::cout << "[Offline] Outer Comm. (Recv): "
                << offline_comm_recv / 1024.0 / 1024 << " MB\n";
    }
  } else if (idx == nparty - 1) {
    for (int i = 0; i < nparty - 1; ++i) {
      coproto::sync_wait(chls[i].flush());
      offline_comm += chls[i].bytesSent();
      offline_comm_recv += chls[i].bytesReceived();
    }
    std::cout << "[Offline] Pivot Comm. (Sent): "
              << offline_comm / 1024.0 / 1024 << " MB\n";
    std::cout << "[Offline] Pivot Comm. (Recv): "
              << offline_comm_recv / 1024.0 / 1024 << " MB\n";
  } else if (idx == nparty - 2) {
    for (int i = 0; i < nparty; ++i) {
      if (i == idx) continue;
      coproto::sync_wait(chls[i].flush());
      offline_comm += chls[i].bytesSent();
      offline_comm_recv += chls[i].bytesReceived();
    }
    std::cout << "[Offline] Inner Comm. (Sent): "
              << offline_comm / 1024.0 / 1024 << " MB\n";
    std::cout << "[Offline] Inner Comm. (Recv): "
              << offline_comm_recv / 1024.0 / 1024 << " MB\n";
  }

  // FIN sync for correct offline runtime estimate
  if (idx == 0) {
    std::vector<uint8_t> fin(2);
    std::vector<std::thread> fin_thrds(nparty - 1);
    for (int i = 0; i < nparty - 1; ++i) {
      fin_thrds[i] = std::thread([&, i]() {
        coproto::sync_wait(chls[i + 1].recv(fin));
        return;
      });
    }
    for (auto &thrd : fin_thrds) thrd.join();
  } else {
    std::vector<uint8_t> fin = {0, 0};
    std::thread fin_thrd = std::thread([&]() {
      coproto::sync_wait(chls[0].send(fin));
      return;
    });
    fin_thrd.join();
  }

  offline_comm = 0;
  offline_comm_recv = 0;
  for (int i = 0; i < chls.size(); ++i) {
    if (idx == i) continue;
    coproto::sync_wait(chls[i].flush());
    offline_comm += chls[i].bytesSent();
    offline_comm_recv += chls[i].bytesReceived();
  }

  timer.setTimePoint("finish offline statistic");

  Online(idx, nparty, t, in, chls, inner_okvs, a, b, c, out);

  if (idx == 0) {
    if (t < nparty - 1) {
      timer.setTimePoint("outer online");
    } else if (t == nparty - 1) {
      timer.setTimePoint("inner online");
    }
  } else if (idx == nparty - 2) {
    timer.setTimePoint("inner online");
  } else if (idx == nparty - 1) {
    timer.setTimePoint("pivot online");
  }

  if (idx == 0) {
    for (int i = 1; i < nparty; ++i) {
      coproto::sync_wait(chls[i].flush());
      online_comm += chls[i].bytesSent();
      online_comm_recv += chls[i].bytesReceived();
    }
    online_comm -= offline_comm;
    online_comm_recv -= offline_comm_recv;
    std::cout << "[Online] Outer Comm. (Sent): " << online_comm / 1024.0 / 1024
              << " MB\n";
    std::cout << "[Online] Outer Comm. (Recv): "
              << online_comm_recv / 1024.0 / 1024 << " MB\n";
    std::cout << timer << "\n";
  } else if (idx == nparty - 1) {
    for (int i = 0; i < nparty - 1; ++i) {
      coproto::sync_wait(chls[i].flush());
      online_comm += chls[i].bytesSent();
      online_comm_recv += chls[i].bytesReceived();
    }
    online_comm -= offline_comm;
    online_comm_recv -= offline_comm_recv;
    std::cout << "[Online] Pivot Comm. (Sent): " << online_comm / 1024.0 / 1024
              << " MB\n";
    std::cout << "[Online] Pivot Comm. (Recv): "
              << online_comm_recv / 1024.0 / 1024 << " MB\n";
    std::cout << "Intersection size: " << out.size() << "\n";
    std::cout << timer << "\n";
  } else if (idx == nparty - 2) {
    for (int i = 0; i < nparty; ++i) {
      if (i == idx) continue;
      coproto::sync_wait(chls[i].flush());
      online_comm += chls[i].bytesSent();
      online_comm_recv += chls[i].bytesReceived();
    }
    online_comm -= offline_comm;
    online_comm_recv -= offline_comm_recv;
    std::cout << "[Online] Inner Comm. (Sent): " << online_comm / 1024.0 / 1024
              << " MB\n";
    std::cout << "[Online] Inner Comm. (Recv): "
              << online_comm_recv / 1024.0 / 1024 << " MB\n";
    std::cout << timer << "\n";
  }

  // FIN sync for correct offline runtime estimate
  if (idx == 0) {
    std::vector<uint8_t> fin(2);
    std::vector<std::thread> fin_thrds(nparty - 1);
    for (int i = 0; i < nparty - 1; ++i) {
      fin_thrds[i] = std::thread([&, i]() {
        coproto::sync_wait(chls[i + 1].recv(fin));
        return;
      });
    }
    for (auto &thrd : fin_thrds) thrd.join();
  } else {
    std::vector<uint8_t> fin = {0, 0};
    std::thread fin_thrd = std::thread([&]() {
      coproto::sync_wait(chls[0].send(fin));
      return;
    });
    fin_thrd.join();
  }

  for (int i = 0; i < chls.size(); ++i) {
    if (idx == i) continue;
    coproto::sync_wait(chls[i].flush());
  }

  CloseConnection(idx, chls);
}

int main(int argc, char **argv) {
  CLP cmd(argc, argv);
  if (cmd.isSet("vole")) tVole(cmd);
  if (cmd.isSet("mpsi")) tMPSI(cmd);
  if (cmd.isSet("okvs")) tOKVS();
}
