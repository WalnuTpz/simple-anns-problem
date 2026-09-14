# 数据准备

本目录用于存放 SIFT 数据集，仅此 README 纳入版本控制；下载的压缩包、向量文件和生成的 ground truth 均保留在本地。

| 数据集 | 基础向量数 | 维度 | 准备与运行命令 |
| --- | ---: | ---: | --- |
| SIFT small | 10,000 | 128 | [快速开始](../README.md#快速开始) |
| SIFT1M | 1,000,000 | 128 | [SIFT1M 示例](../docs/reproduction.md#sift1m-示例) |

## 下载数据

以下命令均在仓库根目录执行，按需选择数据集。数据来自 [TexMex](http://corpus-texmex.irisa.fr/)。

### SIFT small

```bash
mkdir -p data
wget -c ftp://ftp.irisa.fr/local/texmex/corpus/siftsmall.tar.gz -O data/siftsmall.tar.gz
tar -xzf data/siftsmall.tar.gz -C data
```

### SIFT1M

```bash
mkdir -p data
wget -c ftp://ftp.irisa.fr/local/texmex/corpus/sift.tar.gz -O data/sift.tar.gz
tar -xzf data/sift.tar.gz -C data
```

## 目录布局

解压后的目录结构如下：

```text
data/
├── README.md
├── siftsmall/
│   ├── siftsmall_base.fvecs
│   ├── siftsmall_query.fvecs
│   └── siftsmall_groundtruth.ivecs
└── sift/
    ├── sift_base.fvecs
    ├── sift_query.fvecs
    └── sift_groundtruth.ivecs
```

SIFT1M 的官方解压目录名为 `sift/`，对应原项目中的 `siftbig/`。`*_base.fvecs` 为基础向量，`*_query.fvecs` 为查询向量，`*_groundtruth.ivecs` 为精确近邻 ID。

查询时，ground truth 每行的 ID 数必须与参数 `k` 一致。评测 Recall@10 时，按上述运行说明使用 `scripts/prepare_groundtruth.py` 生成 `*_groundtruth_10.ivecs`。文件格式与转换说明见[复现文档](../docs/reproduction.md#准备-top-k-ground-truth)。
