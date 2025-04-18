import getopt
import itertools
import subprocess
import sys
import time

import numpy as np
import pandas as pd

np.set_printoptions(suppress=True)

header = np.array(
    [
        "outer_offline_rt",
        "inner_offline_rt",
        "pivot_offline_rt",
        "outer_online_rt",
        "inner_online_rt",
        "pivot_online_rt",
        "outer_offline_comm",
        "outer_offline_comm_recv",
        "outer_offline_comm_total",
        "inner_offline_comm",
        "inner_offline_comm_recv",
        "inner_offline_comm_total",
        "pivot_offline_comm",
        "pivot_offline_comm_recv",
        "pivot_offline_comm_total",
        "outer_online_comm",
        "outer_online_comm_recv",
        "outer_online_comm_total",
        "inner_online_comm",
        "inner_online_comm_recv",
        "inner_online_comm_total",
        "pivot_online_comm",
        "pivot_online_comm_recv",
        "pivot_online_comm_total",
        "total_offline_comm",
        "total_offline_comm_recv",
        "total_offline_comm_total",
        "total_online_comm",
        "total_online_comm_recv",
        "total_online_comm_total",
        "ttp",
    ]
)


# [offline_outer/inner/pivot_time, online_outer/inner/pivot_time,offline_outer/inner/pivot_comm, online_outer/inner/pivot_comm]
# aux = (n, t)
def aggregate(dg, aux):
    res = np.array([0.0] * len(header), dtype=np.float64)
    times = np.array([0] * len(header), dtype=np.int64)
    times[30] = 1
    for d in dg:
        lines = d.split("\n")
        for line in lines:
            if line.startswith("outer offline"):
                res[0] += float(line.split()[3])
                times[0] += 1
            if line.startswith("inner offline"):
                res[1] += float(line.split()[3])
                times[1] += 1
            if line.startswith("pivot offline"):
                res[2] += float(line.split()[3])
                times[2] += 1
            if line.startswith("outer online"):
                res[3] += float(line.split()[3])
                times[3] += 1
            if line.startswith("inner online"):
                res[4] += float(line.split()[3])
                times[4] += 1
            if line.startswith("pivot online"):
                res[5] += float(line.split()[3])
                times[5] += 1
            if line.startswith("[Offline] Outer Comm. (Sent):"):
                res[6] += float(line.split()[4])
                times[6] += 1
            if line.startswith("[Offline] Outer Comm. (Recv):"):
                res[7] += float(line.split()[4])
                times[7] += 1
            if line.startswith("[Offline] Inner Comm. (Sent):"):
                res[9] += float(line.split()[4])
                times[9] += 1
            if line.startswith("[Offline] Inner Comm. (Recv):"):
                res[10] += float(line.split()[4])
                times[10] += 1
            if line.startswith("[Offline] Pivot Comm. (Sent):"):
                res[12] += float(line.split()[4])
                times[12] += 1
            if line.startswith("[Offline] Pivot Comm. (Recv):"):
                res[13] += float(line.split()[4])
                times[13] += 1
            if line.startswith("[Online] Outer Comm. (Sent):"):
                res[15] += float(line.split()[4])
                times[15] += 1
            if line.startswith("[Online] Outer Comm. (Recv):"):
                res[16] += float(line.split()[4])
                times[16] += 1
            if line.startswith("[Online] Inner Comm. (Sent):"):
                res[18] += float(line.split()[4])
                times[18] += 1
            if line.startswith("[Online] Inner Comm. (Recv):"):
                res[19] += float(line.split()[4])
                times[19] += 1
            if line.startswith("[Online] Pivot Comm. (Sent):"):
                res[21] += float(line.split()[4])
                times[21] += 1
            if line.startswith("[Online] Pivot Comm. (Recv):"):
                res[22] += float(line.split()[4])
                times[22] += 1
            if line.startswith("ttp"):
                res[30] = 1
                times[30] = 1
    res = np.divide(res, times, where=times != 0)
    # compute total communication
    n, t = aux
    nouter = n - t - 1
    ninner = t  # ignore pivot
    res[8] = res[6] + res[7]
    res[11] = res[9] + res[10]
    res[14] = res[12] + res[13]
    res[17] = res[15] + res[16]
    res[20] = res[18] + res[19]
    res[23] = res[21] + res[22]
    res[24] = nouter * res[6] + ninner * res[9] + res[12]
    res[25] = nouter * res[7] + ninner * res[10] + res[13]
    res[26] = res[24] + res[25]
    res[27] = nouter * res[15] + ninner * res[18] + res[21]
    res[28] = nouter * res[16] + ninner * res[19] + res[22]
    res[29] = res[27] + res[28]
    print(res)
    res.round(4)
    return res


def process(data, index, path=None):
    assert len(data) == len(index)
    res = []

    df = pd.DataFrame()
    if path != None:
        df = pd.read_csv(path, index_col=0)
        # process data

    for i in range(len(data)):
        dg = data[i]
        tmp = index[i].split("_")
        aux = (int(tmp[0]), int(tmp[1]))
        res.append(aggregate(dg, aux))

    res = np.array(res)
    ndf = pd.DataFrame(res, index=index, columns=header)

    if path != None:
        # find Intersection
        tdf = pd.concat([df, ndf])
        df = tdf[~tdf.index.duplicated(keep="last")]
        # sort
        df = df.iloc[
            df.index.str.extract("\\d+_\\d+_(\\d+)", expand=False)
            .astype(int)
            .argsort(kind="mergesort")
        ]
        df = df.iloc[
            df.index.str.extract("\\d+_(\\d+)_\\d+", expand=False)
            .astype(int)
            .argsort(kind="mergesort")
        ]
        df = df.iloc[
            df.index.str.extract("(\\d+)_\\d+_\\d+", expand=False)
            .astype(int)
            .argsort(kind="mergesort")
        ]
    else:
        df = ndf

    print(df)
    return df


def batch_test(nl, tl, ml):
    data = []
    index = []
    for n, t, m in itertools.product(nl, tl, ml):
        if t >= n:
            continue
        data.append(unit_test(0, n, t, m, 1000))
        index.append("{}_{}_{}".format(n, t, m))
        print("finsih {}_{}_{}".format(n, t, m))
    return data, index


def unit_test(ttp, n, t, logm, inter, rep=10):
    data = []
    need_ttp = 0
    for _ in range(rep):
        retry_time = 5
        p = subprocess.Popen(
            ["./run.sh", "-mpsi", str(ttp), str(n), str(t), str(logm), str(inter)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        out_msg, err_msg = p.communicate()
        time.sleep(10)
        while err_msg != b"" and retry_time > 0 and need_ttp == 0:
            print("error run, turn to ttp mode in {} retries".format(retry_time))
            retry_time -= 1
            time.sleep(10)
            p = subprocess.Popen(
                ["./run.sh", "-mpsi", str(ttp), str(n), str(t), str(logm), str(inter)],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            out_msg, err_msg = p.communicate()
        if err_msg != b"" and retry_time == 0:
            print("turn to ttp mode")
            need_ttp = 1
            retry_time = 3
        while (
            err_msg != b''
            and retry_time > 0
            and need_ttp == 1
        ):
            print("finish this group in {} retries".format(retry_time))
            retry_time -= 1
            time.sleep(10)
            p = subprocess.Popen(
                ["./run.sh", "-mpsi", "1", str(n), str(t), str(logm), str(inter)],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            out_msg, err_msg = p.communicate()
        print("finsih group {}_{}_{} in {} mode".format(n, t, logm, need_ttp))
        if out_msg != b'':
            print("have result")
            out = out_msg
            data.append(out.decode("utf-8").strip())
            print(out)

    print("=" * 60)
    print("")
    return data


def to_file(df, file):
    df.to_csv(file, float_format="%.6f")

argv = sys.argv
opts, args = getopt.getopt(argv[1:], "n:t:m:r:o:")
nl = []
tl = []
ml = []
r = ""
o = ""
for opt, arg in opts:
    if opt == '-n':
        nl = list(map(int, arg.split(',')))
    elif opt == '-t':
        tl = list(map(int, arg.split(',')))
    elif opt == '-m':
        ml = list(map(int, arg.split(',')))
    elif opt == '-r':
        r = arg
    elif opt == '-o':
        o = arg

data, index = batch_test(nl, tl, ml)
if r != "":
    df = process(data, index, r)
    to_file(df, o)
else:
    df = process(data, index)
    to_file(df, o)
