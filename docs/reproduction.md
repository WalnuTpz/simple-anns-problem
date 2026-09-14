# 复现说明

以下说明对应当前 `src/` 中的分层图实现。基础编译及 SIFT small 示例见[主 README](../README.md#快速开始)，本页命令也从仓库根目录执行。

## 命令行接口

```text
build/build_knn <base.fvecs> <index_prefix> <K_build>
build/search <base.fvecs> <index_prefix> <query.fvecs> <groundtruth.ivecs> <k>
```

| 参数 | 含义 |
| --- | --- |
| `base.fvecs` | 基础向量；查询时应与构建索引所用文件保持一致，包括行顺序。 |
| `index_prefix` | 索引文件前缀，不是单个 `.graph` 文件；构建时其父目录必须存在。 |
| `K_build` | 用于设置构建时的邻居数上限，实际 `M = max(K_build, M(n))`；节点度数可以小于 `M`。 |
| `query.fvecs` | 待查询向量，维度与基础向量一致。 |
| `groundtruth.ivecs` | 与查询一一对应的精确近邻 ID，每行恰好 `k` 个，按距离从近到远排列。 |
| `k` | 查询返回的近邻数，与 `K_build` 独立。 |

当前源码将维度固定为 `DIM = 128`，距离为平方欧氏距离。输入数据必须非空，`K_build` 与 `k` 为正整数；`k` 不能超过可返回的候选数。现有命令行和二进制输入检查较简略，部分检查使用 `assert`，在 Release 构建中可能被关闭，因此应在运行前确认文件路径、格式及维度。

## 向量与索引格式

TexMex 文件按行存储，供 Linux / WSL 的常见小端平台读取：

| 文件 | 每行布局 |
| --- | --- |
| `.fvecs` | 一个 int32 维度，后接该数量的 float32 坐标。 |
| `.ivecs` | 一个 int32 数量，后接该数量的 int32 ID。 |

当前图索引由多个文件组成：

```text
indexes/example.meta       # 文本：entry、entry_level、M
indexes/example.levels     # 每个节点的层级，每行一个 int32 值
indexes/example.L0         # 基础层邻接表
indexes/example.L1         # 更高层邻接表
...
indexes/example.L<entry_level>
```

`.levels` 和各层文件使用带行宽前缀的整数向量格式。每层有 `n` 行，每行 `M` 个邻居 ID，不足的位置以 `-1` 补齐。基础向量单独保存，索引不复制向量坐标。课程早期提供的单文件 `.graph` 不能直接传给当前查询程序，应先重新构建索引。

## 准备 Top-k ground truth

查询程序按传入的 `k` 读取 ground truth。原文件若每行包含 100 个 ID，评测 `k=10` 时需要同时裁剪 ID 列表和每行的宽度前缀。

```bash
python3 scripts/prepare_groundtruth.py INPUT.ivecs OUTPUT.ivecs -k 10
```

脚本仅依赖 Python 3 标准库，保留每行前 `k` 个 ID，不重新计算或排序近邻；原始 ground truth 应已按距离排序。输入和输出必须是不同文件。脚本在写出前检查所有行，拒绝空文件、截断文件及超出原行宽的 `k`。

## SIFT small 示例

按[主 README](../README.md#快速开始) 完成编译、数据准备和索引构建后，使用以下命令评测 Recall@10。转换脚本保留每行前 10 个 ID，并更新行宽前缀；查询仍使用同一索引。

```bash
python3 scripts/prepare_groundtruth.py \
  data/siftsmall/siftsmall_groundtruth.ivecs \
  data/siftsmall/siftsmall_groundtruth_10.ivecs \
  -k 10

OMP_NUM_THREADS=4 ./build/search \
  data/siftsmall/siftsmall_base.fvecs \
  indexes/siftsmall \
  data/siftsmall/siftsmall_query.fvecs \
  data/siftsmall/siftsmall_groundtruth_10.ivecs \
  10
```

## SIFT1M 示例

在完成 README 中的编译后，从 [TexMex](http://corpus-texmex.irisa.fr/) 下载 SIFT1M 数据。官方压缩包名为 `sift.tar.gz`，解压目录为 `sift/`；原项目的 `siftbig/` 是同一数据集的本地目录名。以下命令使用 `data/sift/`，向量文件名保持官方的 `sift_base.fvecs`、`sift_query.fvecs` 和 `sift_groundtruth.ivecs`。

```bash
mkdir -p data indexes
wget -c ftp://ftp.irisa.fr/local/texmex/corpus/sift.tar.gz -O data/sift.tar.gz
tar -xzf data/sift.tar.gz -C data

python3 scripts/prepare_groundtruth.py \
  data/sift/sift_groundtruth.ivecs \
  data/sift/sift_groundtruth_10.ivecs \
  -k 10

OMP_NUM_THREADS=4 ./build/build_knn \
  data/sift/sift_base.fvecs \
  indexes/sift1m \
  32

OMP_NUM_THREADS=4 ./build/search \
  data/sift/sift_base.fvecs \
  indexes/sift1m \
  data/sift/sift_query.fvecs \
  data/sift/sift_groundtruth_10.ivecs \
  10
```

程序将向量与邻接表加载到内存，并为搜索创建访问标记数组；扩大数据规模或增加并发查询线程时，应同时关注内存占用。

若已有原目录 `siftbig/` 下的数据，可保留其目录名，并将命令中的 `data/sift/` 替换为实际路径。若 ground truth 已裁剪为每行 10 个 ID，可直接用于 `k=10` 的查询，无需再次转换。当前查询仍需使用分层索引前缀；原有单文件 `1m.graph` 应按本页说明重新构建为分层索引。

## 参数与随机性

参数选择在 [build_knn.cpp](../src/build_knn.cpp) 中，查询流程在 [search.cpp](../src/search.cpp) 中。

| 参数或行为 | 当前实现 |
| --- | --- |
| 基础 `M(n)` | `n ≤ 20,000` 为 16；`20,000 < n ≤ 200,000` 为 24；更大时为 32。 |
| 实际 `M` | `max(M(n), K_build)`。 |
| `ef_construction` | 对上述三个规模区间分别为 200、300、400。 |
| 构建时上层搜索宽度 | `max(ef_construction / 2, 4M)`。 |
| 构建时 L₀ 搜索宽度 | `ef_construction`。 |
| `max_level` | `max(1, floor(log2(n)))`。 |
| 层级分布系数 `ml` | `0.0315 × ln(n) + 0.144`。 |
| 节点层级 | `min(floor(-ln(U) × ml), max_level)`；实现将 `U` 下限截到 `1e-9`。 |
| 层级采样随机源 | `mt19937`，种子为 42。 |
| 查询 `ef_search` | `max(2k, 100)`。 |
| 构建并行范围 | 节点插入顺序执行，每层选定邻居的链接更新使用 OpenMP 静态调度与节点锁。 |
| 查询并行范围 | 跨查询并行，使用 OpenMP 动态调度。 |

固定随机种子不代表跨编译器或多线程运行的索引文件一定逐字节一致。比较结果时应固定构建环境与线程数，并记录多次运行的波动。

## 记录评测结果

建议为每组实验保存以下信息：

```text
Commit:
Dataset / base count / query count / dimension:
CPU / RAM / OS:
Compiler / CMake / build type:
K_build / actual M / k / ef_search:
OMP_NUM_THREADS:
Repetitions:
Recall@k:
QPS (individual runs, median, range):
Build time / index size (if measured):
```

当前 QPS 覆盖整个并行评测循环，包括 recall 统计；不要将其当成仅计算距离或仅执行图遍历的吞吐量。构建程序打印参数，但不直接报告构建耗时及索引大小；需要单独测量这两项。不同版本的索引、线程数和计时范围应分开记录。
