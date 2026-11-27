# 为什么 OPI DDR 不能当作 QSPI 的一个“寄存器配置分支”

## 结论

`ST_390390_opi_ddr.c` 用的确实是同一套 LCDC/DBIC 寄存器块，所以“只是寄存器值不同”这句话在**最底层**是对的。但它和 8773G QSPI 面板的差异不只在寄存器值，而在：

- 物理协议：OPI（8 线）+ DDR（双沿） vs QSPI（4 线）+ SDR；
- 数据通路：LLI / 2 行交错 / infinite buffer vs 线性 DMA；
- 像素格式：ABGR888 输入、RGB888 输出 vs 常见 RGB565/RGB888；
- 软件契约：`lcd_st_opi_*` + RT-Thread API vs `rtk_sdi_hal_*` + panel descriptor。

因此建议：**OPI DDR 用独立接口/独立控制器，但共享 DBIC、DMA、时钟、复位这些底层原语**。不是“另起一套硬件”，而是不要在 QSPI 控制器里塞一个 OPI/DDR 分支。

## 1. 代码事实

| 项目 | QSPI（8773G） | OPI DDR（8772F `ST_390390_opi_ddr.c`） |
|---|---|---|
| 数据线/速率 | 4 线 SDR | OPI 8 线 + DDR |
| 命令阶段 | `DBIC->CTRLR0` 常规 QSPI 配置 | `DBIC->CTRLR0 = 0x8100001F`，见 [L31](../legacy/8772F/lcd/ST_390390_opi_ddr.c#L31) |
| 数据阶段 | `LCDC_SPICCmd(0x32)` + `0x002c00` | `CTRLR0 = 0x810C001F` + `BIT_DDR` + `LCDC_SPICCmd(0xB2)`，见 [L79-L81](../legacy/8772F/lcd/ST_390390_opi_ddr.c#L79-L81) |
| DBIC speed | 2（或 3） | 16，见 [L225](../legacy/8772F/lcd/ST_390390_opi_ddr.c#L225) |
| 输入/输出格式 | RGB565 / RGB888 | `LCDC_INPUT_ABGR888` + `LCDC_OUTPUT_RGB888`，见 [L211-L212](../legacy/8772F/lcd/ST_390390_opi_ddr.c#L211-L212) |
| DMA | 线性、Msize 16/64 | `DMA_LINKLIST=1`、Msize 8、LLI、2 行交错，见 [L129-L174](../legacy/8772F/lcd/ST_390390_opi_ddr.c#L129-L174) |
| 源地址更新 | 单次 DMA | `LCDC_DMA_SetSourceAddress()` + infinite buffer，见 [L192](../legacy/8772F/lcd/ST_390390_opi_ddr.c#L192) |
| API | `rtk_sdi_hal_*` + panel descriptor | `lcd_st_opi_*`，没有 `rtk_sdi_hal_*`，见 [ST_390390_opi_ddr.h](../legacy/8772F/lcd/ST_390390_opi_ddr.h#L10-L15) |
| OS/板级依赖 | Zephyr + `rtk_sdi_bus_*` | `board.h`、`rt_thread_mdelay`、`LCDC_LCD_SET_RST`，见 [L236-L241](../legacy/8772F/lcd/ST_390390_opi_ddr.c#L236-L241) |

## 2. 为什么不能只加一个 `is_opi` 标志

如果把它塞进当前 8773G QSPI 控制器，`T_RTK_SDI_BUS` 至少要增加：

- `lanes`（4/8）、`ddr`（SDR/DDR）；
- 命令/数据 opcode（0x02/0x32 vs 0x02/0xB2）；
- `CTRLR0` 原始值或更多位域；
- `input_format`（RGB565/RGB888/ABGR888）；
- `lli`/`infinite buffer`/`gpu_width`/`sar_offset`；
- 甚至 RT-Thread/board 依赖。

这样 QSPI 控制器就会变成“QSPI + OPI DDR 的并集”，每个 QSPI 面板的 bus 配置都要携带一堆它永远用不到的 OPI 字段。面板数据抽象会被破坏，QSPI 的测试也不再能覆盖 OPI 的时序。

更合适的边界是：

- `rtk_sdi_bus` 只描述 QSPI 的 bus（时钟 200/280、group、pin、Msize、DBIC speed、reset/BL）；
- OPI DDR 有自己的 `rtk_sdi_opi_ddr` 控制器和 bus 配置；
- 两者共享 `DBIC` 寄存器访问、`LCDC_DMA_LinkList_Init`、时钟/复位等底层函数。

## 3. 哪些确实可以共享

虽然接口分开，以下底层能力可以复用，不需要重写：

- `DBIC_Cmd/CmdLength/AddrLength/TX_NDF`；
- `LCDC_DMA_StructInit`、`LCDC_DMA_LinkList_Init`；
- `LCDC_Clock_Sel` / `RCC_DisplayClockConfig`；
- `Pad_Config` / `LCDC_LCD_SET_RST` 等板级原语；
- 面板的 init/power 命令表格式（如果 OPI 面板也使用 0x02 + 24-bit address）。

也就是说：**共享硬件原语，不共享协议控制器**。

## 4. 推荐接口

```c
typedef enum {
    RTK_SDI_IF_QSPI = 0,
    RTK_SDI_IF_SPI,
    RTK_SDI_IF_RGB,
    RTK_SDI_IF_DBIB,
    RTK_SDI_IF_OPI_DDR,   /* 新增，独立控制器 */
} T_RTK_SDI_IF;
```

- QSPI：`controller/lcdc/rtk_sdi_lcdc.c`，使用共享 LCDC/DBIC 主体及 `rtk_display_soc_*` 平台服务；
- OPI DDR：`rtk_sdi_opi_ddr_<soc>.c`，走自己的 bus/ops，不进入 QSPI 的 `LCDC_SPICCmd(0x32)` 路径；
- 如果将来 8773G 也出现 OPI DDR 面板，只增加 8773G 的 OPI 控制器，不修改 QSPI 控制器。

## 5. 对当前任务的影响

当前 8773G QSPI 任务（200/280 MHz、pin group、Msize、DBIC speed、reset/BL）**不包含 OPI DDR**。本文件只记录边界和理由；`ST_390390_opi_ddr.c` 不进入本次 QSPI 迁移，也不建议为了“统一”把它硬塞进 `T_RTK_SDI_BUS`。

RAMLESS QSPI（如 ST77903 RLSPI）同理：它没有 GRAM、需要连续帧流和 `LCDC_RamlessEn`，也不属于本次 GRAM QSPI 范围。
