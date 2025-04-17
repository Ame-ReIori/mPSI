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
#include <unistd.h>
#include <volePSI/Paxos.h>

#include <iostream>
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
  baxos.init(n, BIN_SIZE, CUCKOO_HASH_NUM, LAMBDA, volePSI::PaxosParam::Binary,
             AllOneBlock);

  PRNG prng0(ZeroBlock);
  PRNG prng1(AllOneBlock);
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
  std::vector<block> as(baxos.size());
  block b = prng.get();
  std::vector<block> cs(baxos.size());

  prng.get<block>(as);
  for (int i = 0; i < as.size(); ++i) {
    cs[i] = b & as[i];
  }

  std::vector<block> a0(baxos.size());
  std::vector<block> a1(baxos.size());
  block b0 = prng.get();
  block b1 = b ^ b0;
  std::vector<block> c0(baxos.size());
  std::vector<block> c1(baxos.size());

  prng.get<block>(a0);
  prng.get<block>(c0);

  for (int i = 0; i < a0.size(); ++i) {
    a1[i] = as[i] ^ a0[i];
    c1[i] = cs[i] ^ c0[i];
  }

  // 1. open
  std::vector<block> o0(baxos.size());
  std::vector<block> o1(baxos.size());
  std::vector<block> o(baxos.size());
  for (int i = 0; i < baxos.size(); ++i) {
    o0[i] = c[i] ^ a0[i];
    o1[i] = c_[i] ^ a1[i];
    o[i] = o0[i] ^ o1[i];
  }

  for (int i = 0; i < baxos.size(); ++i) {
    o0[i] = (o[i] & b0) ^ c0[i];
    o1[i] = (o[i] & b1) ^ c1[i];
  }

  for (int i = 0; i < c.size(); ++i) {
    // c_[i] = c[i] & b;
    c_[i] = o0[i] ^ o1[i];
  }

  // test availablity of c_
  baxos.decode<block>(x0, v_, c_);

  int count_ = 0;
  for (int i = 0; i < v.size(); ++i) {
    if ((v[i] == v_[i]) && (v[i] == ZeroBlock) && (v_[i] == ZeroBlock))
      count_++;
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

void tMPSI(const CLP &cmd) {
  Timer timer;
  int nparty = cmd.getOr("n", 15);
  int t = cmd.getOr("t", 10);
  int idx = cmd.getOr("id", nparty - 1);
  int logn = cmd.getOr("m", 20);
  int neles = 1 << logn;
  int nint = cmd.getOr("i", 1000);
  int ttp = cmd.isSet("ttp");
  std::vector<Socket> chls;
  std::vector<block> inner_okvs;
  std::vector<block> in(neles);
  std::vector<block> out;
  // std::vector<CoeffCtxGF2::Vec<block>> as;
  // std::vector<CoeffCtxGF2::Vec<block>> bs;
  // std::vector<CoeffCtxGF2::Vec<block>> cs;
  // block d;
  std::vector<block> a;
  std::vector<block> c;
  block b;
  PRNG prng(sysRandomSeed());
  prng.get<block>(in);

  for (int i = 0; i < nint; ++i) {
    in[(i + idx) % neles] = toBlock(i);
  }

  timer.setTimePoint("start");
  EstablishConnection(idx, nparty, chls);
  timer.setTimePoint("finish connection");
  // if (ttp) {
  //   TTPOffline(idx, nparty, t, neles, chls, inner_okvs, a, b, c);
  // }
  if (t != (nparty - 1)) {
    // Offline(idx, nparty, t, neles, chls, inner_okvs, as, bs, cs, d);
    Offline(idx, nparty, t, neles, chls, inner_okvs, a, b, c);
  } else {
    OfflineFullThreshold(idx, nparty, t, neles, chls, inner_okvs, a, b, c);
  }
  timer.setTimePoint("finish offline");
  // Online(idx, nparty, t, in, chls, inner_okvs, as, bs, cs, d, out);
  Online(idx, nparty, t, in, chls, inner_okvs, a, b, c, out);
  timer.setTimePoint("finish online");
  if (idx == nparty - 1) std::cout << timer << "\n";
  CloseConnection(idx, chls);
}

int main(int argc, char **argv) {
  CLP cmd(argc, argv);
  if (cmd.isSet("vole")) tVole(cmd);
  if (cmd.isSet("mpsi")) tMPSI(cmd);
  if (cmd.isSet("okvs")) tOKVS();
}
