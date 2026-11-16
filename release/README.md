# Jarvis_OS 发布包

正式发布只输出一个固件：`Jarvis_OS.bin`。产物由 `.gitignore` 忽略，需要通过构建步骤重现；同目录的 `.sha256` 记录其 SHA-256 校验值。

构建、复制、校验与安全安装步骤见 [`docs/deployment/README.md`](../docs/deployment/README.md)。当前目标设备使用应用槽偏移 `0x20000`，更新时仅写该应用槽。不要将本文件夹中的应用镜像与 bootloader、分区表、ota_data_initial 或 storage 镜像组合刷写。
