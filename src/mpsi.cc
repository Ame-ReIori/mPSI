#include "mpsi.h"

#include <coproto/Common/macoro.h>
#include <cryptoTools/Common/Defines.h>
#include <cryptoTools/Common/block.h>
#include <cryptoTools/Crypto/PRNG.h>
#include <libOTe/Tools/CoeffCtx.h>
#include <libOTe/Vole/Silent/SilentVoleReceiver.h>
#include <libOTe/Vole/Silent/SilentVoleSender.h>
#include <sys/socket.h>
#include <volePSI/Paxos.h>

#include <thread>
#include <vector>

#include "defines.h"

int OKVSOffline(uint8_t idx, uint8_t nparty, uint8_t t, uint64_t neles,
                std::vector<Socket> &chls, std::vector<block> &inner_okvs) {
  PRNG prng(sysRandomSeed());
  // idx < n - t - 1 sends okvs seed to minimal system
  volePSI::Baxos baxos;
  // baxos.init(neles, BIN_SIZE, CUCKOO_HASH_NUM, LAMBDA,
  //            volePSI::PaxosParam::Binary, AllOneBlock);
  baxos.init(neles, BIN_SIZE, CUCKOO_HASH_NUM, LAMBDA,
             volePSI::PaxosParam::GF128, AllOneBlock);
  inner_okvs.resize(baxos.size());
  if (t == nparty - 1) {
    // P_0 acts as TTP
    if (idx == 0) {
      std::vector<std::thread> okvs_thrds(t);
      std::vector<block> okvs_seeds(t);
      prng.get<block>(okvs_seeds);

      // send each seed to msys parties
      for (int i = 0; i < t; ++i) {
        okvs_thrds[i] = std::thread([&, i]() {
          coproto::sync_wait(chls[i + 1].send(okvs_seeds[i]));
          return;
        });
      }

      for (auto &thrd : okvs_thrds) {
        thrd.join();
      }

      // xor aggregate all okvs
      for (int i = 0; i < t; ++i) {
        PRNG okvs_prng(okvs_seeds[i]);
        std::vector<block> okvs_rnd(baxos.size());
        okvs_prng.get<block>(okvs_rnd);
        for (int j = 0; j < baxos.size(); ++j) {
          inner_okvs[j] ^= okvs_rnd[j];
        }
      }
    } else {
      block okvs_seed;
      std::thread okvs_thrd = std::thread([&]() {
        coproto::sync_wait(chls[0].recv(okvs_seed));
        return;
      });

      okvs_thrd.join();

      PRNG okvs_prng(okvs_seed);
      okvs_prng.get<block>(inner_okvs);
    }
  } else {
    int threshold = nparty - t - 1;

    if (idx < threshold) {
      // outer parties are only required to interact with t parties
      std::vector<std::thread> okvs_thrds(t);
      std::vector<block> okvs_seeds(t);
      prng.get<block>(okvs_seeds);

      // send each seed to msys parties
      for (int i = 0; i < t; ++i) {
        okvs_thrds[i] = std::thread([&, i]() {
          coproto::sync_wait(chls[threshold + i].send(okvs_seeds[i]));
          return;
        });
      }

      for (auto &thrd : okvs_thrds) {
        thrd.join();
      }

      // xor aggregate all okvs
      for (int i = 0; i < t; ++i) {
        PRNG okvs_prng(okvs_seeds[i]);
        std::vector<block> okvs_rnd(baxos.size());
        okvs_prng.get<block>(okvs_rnd);
        for (int j = 0; j < baxos.size(); ++j) {
          inner_okvs[j] ^= okvs_rnd[j];
        }
      }
    } else {
      if (idx != (nparty - 1)) {
        std::vector<std::thread> okvs_thrds(threshold);
        std::vector<block> okvs_seeds(threshold);

        for (int i = 0; i < threshold; ++i) {
          okvs_thrds[i] = std::thread([&, i]() {
            coproto::sync_wait(chls[i].recv(okvs_seeds[i]));
            return;
          });
        }

        for (auto &thrd : okvs_thrds) {
          thrd.join();
        }

        // xor aggreage all okvs
        for (int i = 0; i < threshold; ++i) {
          PRNG okvs_prng(okvs_seeds[i]);
          std::vector<block> okvs_rnd(baxos.size());
          okvs_prng.get<block>(okvs_rnd);
          for (int j = 0; j < baxos.size(); ++j) {
            inner_okvs[j] ^= okvs_rnd[j];
          }
        }
      }
    }
  }
  return 1;
}

int BeaverTTPOffline(uint8_t idx, uint8_t nparty, uint8_t t, uint64_t neles,
                     std::vector<Socket> &chls, std::vector<block> &a, block &b,
                     std::vector<block> &c) {
  int threshold = nparty - t - 1;
  volePSI::Baxos baxos;
  // baxos.init(neles, BIN_SIZE, CUCKOO_HASH_NUM, LAMBDA,
  //            volePSI::PaxosParam::Binary, AllOneBlock);
  baxos.init(neles, BIN_SIZE, CUCKOO_HASH_NUM, LAMBDA,
             volePSI::PaxosParam::GF128, AllOneBlock);
  a.resize(baxos.size());
  c.resize(baxos.size());
  // The first party in minimal system acts as TTP for Beaver Triple
  if (idx == threshold) {
    PRNG prng(sysRandomSeed());

    std::vector<std::vector<block>> as(t, std::vector<block>(baxos.size()));
    std::vector<block> bs(t);
    std::vector<std::vector<block>> cs(t, std::vector<block>(baxos.size()));

    prng.get<block>(a);
    b = prng.get();
    for (int i = 0; i < c.size(); ++i) {
      // c[i] = a[i] & b;
      c[i] = a[i].gf128Mul(b);
    }

    prng.get<block>(bs);
    for (int i = 0; i < t; ++i) {
      prng.get<block>(as[i]);
      prng.get<block>(cs[i]);
    }

    for (int i = 0; i < t; ++i) {
      for (int j = 0; j < a.size(); ++j) {
        a[j] ^= as[i][j];
      }
      b ^= bs[i];
      for (int j = 0; j < c.size(); ++j) {
        c[j] ^= cs[i][j];
      }
    }

    std::vector<std::thread> triplet_thrds(t);
    for (int i = 0; i < t; ++i) {
      triplet_thrds[i] = std::thread([&, i]() {
        coproto::sync_wait(chls[i + threshold + 1].send(as[i]));
        coproto::sync_wait(chls[i + threshold + 1].send(bs[i]));
        coproto::sync_wait(chls[i + threshold + 1].send(cs[i]));
        return;
      });
    }

    for (auto &thrd : triplet_thrds) thrd.join();

  } else if (idx > threshold) {
    std::thread triplet_thrd = std::thread([&]() {
      coproto::sync_wait(chls[threshold].recv(a));
      coproto::sync_wait(chls[threshold].recv(b));
      coproto::sync_wait(chls[threshold].recv(c));
    });
    triplet_thrd.join();
  }
  return 1;
}

int BeaverOffline(uint8_t idx, uint8_t nparty, uint8_t t, uint64_t neles,
                  std::vector<Socket> &chls, std::vector<block> &a, block &b,
                  std::vector<block> &c) {
  int threshold = nparty - t - 1;
  PRNG prng(sysRandomSeed());
  volePSI::Baxos baxos;
  // baxos.init(neles, BIN_SIZE, CUCKOO_HASH_NUM, LAMBDA,
  //            volePSI::PaxosParam::Binary, AllOneBlock);
  baxos.init(neles, BIN_SIZE, CUCKOO_HASH_NUM, LAMBDA,
             volePSI::PaxosParam::GF128, AllOneBlock);
  b = prng.get();
  // std::vector<CoeffCtxGF2::Vec<block>> as;
  // std::vector<CoeffCtxGF2::Vec<block>> bs;
  // std::vector<CoeffCtxGF2::Vec<block>> cs;
  // as.resize(t + 1, CoeffCtxGF2::Vec<block>(baxos.size()));
  // bs.resize(t + 1, CoeffCtxGF2::Vec<block>(baxos.size()));
  // cs.resize(t + 1, CoeffCtxGF2::Vec<block>(baxos.size()));
  std::vector<CoeffCtxGF128::Vec<block>> as;
  std::vector<CoeffCtxGF128::Vec<block>> bs;
  std::vector<CoeffCtxGF128::Vec<block>> cs;
  as.resize(t + 1, CoeffCtxGF128::Vec<block>(baxos.size()));
  bs.resize(t + 1, CoeffCtxGF128::Vec<block>(baxos.size()));
  cs.resize(t + 1, CoeffCtxGF128::Vec<block>(baxos.size()));

  std::vector<std::thread> vole_thrds(t + 1);
  for (int i = 0; i < t + 1; ++i) {
    int target = i + threshold;
    if (idx == target) {
      continue;
    } else if (idx < target) {
      vole_thrds[i] = std::thread([&, i]() {
        // SilentVoleSender<block, block, CoeffCtxGF2> send;
        // SilentVoleReceiver<block, block, CoeffCtxGF2> recv;
        SilentVoleSender<block, block, CoeffCtxGF128> send;
        SilentVoleReceiver<block, block, CoeffCtxGF128> recv;
        coproto::sync_wait(
            send.silentSend(b, bs[i], prng, chls[i + threshold]));
        coproto::sync_wait(
            recv.silentReceive(as[i], cs[i], prng, chls[i + threshold]));
        return;
      });
    } else {
      vole_thrds[i] = std::thread([&, i]() {
        // SilentVoleReceiver<block, block, CoeffCtxGF2> recv;
        // SilentVoleSender<block, block, CoeffCtxGF2> send;
        SilentVoleReceiver<block, block, CoeffCtxGF128> recv;
        SilentVoleSender<block, block, CoeffCtxGF128> send;
        coproto::sync_wait(
            recv.silentReceive(as[i], cs[i], prng, chls[i + threshold]));
        coproto::sync_wait(
            send.silentSend(b, bs[i], prng, chls[i + threshold]));
        return;
      });
    }
  }

  for (int i = 0; i < t + 1; ++i) {
    if (idx == i + threshold) {
      continue;
    } else {
      vole_thrds[i].join();
    }
  }

  a.resize(baxos.size());
  c.resize(baxos.size());

  prng.get<block>(a);
  std::vector<std::thread> triplet_thrds(t + 1);
  std::vector<std::vector<block>> recv_a(t + 1,
                                         std::vector<block>(baxos.size()));
  for (int i = 0; i < t + 1; ++i) {
    int target = i + threshold;
    if (idx == target) {
      continue;
    } else if (idx < target) {
      triplet_thrds[i] = std::thread([&, i]() {
        std::vector<block> masked_a(baxos.size());
        for (int j = 0; j < baxos.size(); ++j) {
          masked_a[j] = a[j] ^ as[i][j];
        }
        coproto::sync_wait(chls[i + threshold].send(masked_a));
        coproto::sync_wait(chls[i + threshold].recv(recv_a[i]));
        return;
      });
    } else {
      triplet_thrds[i] = std::thread([&, i]() {
        std::vector<block> masked_a(baxos.size());
        for (int j = 0; j < baxos.size(); ++j) {
          masked_a[j] = a[j] ^ as[i][j];
        }
        coproto::sync_wait(chls[i + threshold].recv(recv_a[i]));
        coproto::sync_wait(chls[i + threshold].send(masked_a));
        return;
      });
    }
  }
  for (int i = 0; i < t + 1; ++i) {
    if (idx == i + threshold) {
      continue;
    } else {
      triplet_thrds[i].join();
    }
  }
  for (int i = 0; i < baxos.size(); ++i) {
    for (int j = 0; j < t + 1; ++j) {
      c[i] ^= recv_a[j][i];
    }
    c[i] ^= a[i];
    // c[i] &= b;
    c[i] = c[i].gf128Mul(b);
    for (int j = 0; j < t + 1; ++j) {
      c[i] ^= bs[j][i];
    }
    for (int j = 0; j < t + 1; ++j) {
      c[i] ^= cs[j][i];
    }
  }
  return 1;
}

int Offline(uint8_t ttp, uint8_t idx, uint8_t nparty, uint8_t t, uint64_t neles,
            std::vector<Socket> &chls, std::vector<block> &inner_okvs,
            std::vector<block> &a, block &b, std::vector<block> &c) {
  PRNG prng(sysRandomSeed());
  int threshold = nparty - t - 1;
  // idx < n - t - 1 sends okvs seed to minimal system
  volePSI::Baxos baxos;
  // baxos.init(neles, BIN_SIZE, CUCKOO_HASH_NUM, LAMBDA,
  //            volePSI::PaxosParam::Binary, AllOneBlock);
  baxos.init(neles, BIN_SIZE, CUCKOO_HASH_NUM, LAMBDA,
             volePSI::PaxosParam::GF128, AllOneBlock);

  OKVSOffline(idx, nparty, t, neles, chls, inner_okvs);

  if (!ttp) {
    if (idx >= threshold) {
      BeaverOffline(idx, nparty, t, neles, chls, a, b, c);
    }
  } else {
    if (idx == nparty - 1) std::cout << "ttp 1\n";
    BeaverTTPOffline(idx, nparty, t, neles, chls, a, b, c);
  }

  return 1;
}

int Online(uint8_t idx, uint8_t nparty, uint8_t t, std::vector<block> &in,
           std::vector<Socket> &chls, std::vector<block> &inner_okvs,
           std::vector<block> &a, block &b, std::vector<block> &c,
           std::vector<block> &out) {
  if (idx == 0) std::cout << "Use GF128\n";
  int threshold = nparty - t - 1;
  volePSI::Baxos baxos;
  // baxos.init(in.size(), BIN_SIZE, CUCKOO_HASH_NUM, LAMBDA,
  //            volePSI::PaxosParam::Binary, AllOneBlock);
  baxos.init(in.size(), BIN_SIZE, CUCKOO_HASH_NUM, LAMBDA,
             volePSI::PaxosParam::GF128, AllOneBlock);

  std::vector<block> okvs(baxos.size());
  std::vector<block> values(in.size());
  if (idx < threshold) {
    // encode the input set and send to the last party
    baxos.decode<block>(in, values, inner_okvs);
    baxos.solve<block>(in, values, okvs);

    std::thread okvs_thrd = std::thread([&]() {
      coproto::sync_wait(chls[nparty - 1].send(okvs));
      return;
    });
    okvs_thrd.join();

  } else {
    if (idx == nparty - 1) {
      inner_okvs.resize(baxos.size());
      std::vector<std::vector<block>> okvs_rnds(
          threshold, std::vector<block>(baxos.size()));
      std::vector<std::thread> okvs_thrds(threshold);
      for (int i = 0; i < threshold; ++i) {
        okvs_thrds[i] = std::thread([&, i]() {
          coproto::sync_wait(chls[i].recv(okvs_rnds[i]));
          return;
        });
      }

      for (auto &thrd : okvs_thrds) {
        thrd.join();
      }

      for (int i = 0; i < threshold; ++i) {
        for (int j = 0; j < baxos.size(); ++j) {
          inner_okvs[j] ^= okvs_rnds[i][j];
        }
      }
    }

    baxos.decode<block>(in, values, inner_okvs);
    baxos.solve<block>(in, values, okvs);
    std::vector<block> open_okvs(baxos.size());

    // send masked okvs to nparty - 1
    if (idx == nparty - 1) {
      std::vector<std::vector<block>> recv_okvss(
          t, std::vector<block>(baxos.size()));
      // receive t party
      std::vector<std::thread> mask_thrds(t);
      for (int i = 0; i < t; ++i) {
        mask_thrds[i] = std::thread([&, i]() {
          coproto::sync_wait(chls[i + threshold].recv(recv_okvss[i]));
          return;
        });
      }

      for (auto &thrd : mask_thrds) thrd.join();

      // aggregate all okvs and open
      std::vector<std::thread> open_thrds(t);
      std::vector<block> aggregated_okvs(baxos.size());
      // first mask okvs with a
      for (int i = 0; i < baxos.size(); ++i) {
        open_okvs[i] = okvs[i] ^ a[i];
      }

      for (int i = 0; i < t; ++i) {
        for (int j = 0; j < baxos.size(); ++j) {
          open_okvs[j] ^= recv_okvss[i][j];
        }
      }
      for (int i = 0; i < t; ++i) {
        open_thrds[i] = std::thread([&, i]() {
          coproto::sync_wait(chls[i + threshold].send(open_okvs));
          return;
        });
      }

      for (auto &thrd : open_thrds) thrd.join();

    } else {
      std::thread mask_thrd = std::thread([&]() {
        std::vector<block> masked_okvs(baxos.size());
        for (int i = 0; i < baxos.size(); ++i) {
          masked_okvs[i] = okvs[i] ^ a[i];
        }
        coproto::sync_wait(chls[nparty - 1].send(masked_okvs));
        return;
      });
      mask_thrd.join();
      std::thread open_thrd = std::thread([&]() {
        coproto::sync_wait(chls[nparty - 1].recv(open_okvs));
        return;
      });
      open_thrd.join();
    }

    for (int i = 0; i < baxos.size(); ++i) {
      okvs[i] = open_okvs[i].gf128Mul(b);
      okvs[i] ^= c[i];
      // okvs[i] = (open_okvs[i] & b) ^ c[i];
    }

    // start the reconstruction
    if (idx < nparty - 1) {
      // coproto::sync_wait(chls[nparty - 1].send(inner_okvs));
      coproto::sync_wait(chls[nparty - 1].send(okvs));
    }

    if (idx == nparty - 1) {
      std::vector<std::vector<block>> okvss(t,
                                            std::vector<block>(baxos.size()));
      std::vector<std::thread> t_okvs_thrds(t);
      for (int i = 0; i < t; ++i) {
        t_okvs_thrds[i] = std::thread([&, i]() {
          coproto::sync_wait(chls[threshold + i].recv(okvss[i]));
          return;
        });
      }
      for (auto &thrd : t_okvs_thrds) thrd.join();
      for (int i = 0; i < t; ++i) {
        for (int j = 0; j < baxos.size(); ++j) {
          // inner_okvs[j] ^= okvss[i][j];
          okvs[j] ^= okvss[i][j];
        }
      }

      std::vector<block> tval(in.size());
      baxos.decode<block>(in, tval, okvs);
      // baxos.decode<block>(tvec, tval, inner_okvs);
      for (int i = 0; i < tval.size(); ++i) {
        if (tval[i] == ZeroBlock) {
          out.push_back(in[i]);
        }
      }
    }
  }
  return 1;
}
