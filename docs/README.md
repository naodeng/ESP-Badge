# 项目文档

## 当前使用与交付

- [架构总览](architecture/system-overview.md)：固件、iOS、网关和共享协议的边界。
- [开发指南](development/README.md)：工具链、目录、固件/iOS/后端开发入口。
- [固件部署指南](deployment/README.md)：唯一固件包、分区保护和设备刷写。
- [手机 App 配置指南](mobile-app/configuration.md)：网关地址、Xcode 签名、iOS 权限和真机安装。
- [后端服务部署指南](backend/deployment.md)：Node 本地服务、Cloudflare Worker、AMap Key、R2、Cron 和健康检查。
- [手机与圆屏连接指南](connection-guide.md)：首次 BLE 配对、导航同步和常见问题排查。
- [调试与验收](debugging/README.md)：两种本地调试工具、串口和设备安装检查。
- [过程与迭代](process/README.md)：范围变更、提交与发布记录约定。
- [当前项目状态](project-status.md)：已完成项目调整、构建产物和仍需真机验证的项目。
- [产品范围](../PRODUCT.md) · [系统架构](../design.md) · [许可声明](../THIRD_PARTY.md)

## 归档材料

- [`design/history/`](design/history/)：已完成或被当前设计替代的设计记录。
- [`plans/`](plans/)：历史实施计划，不代表尚未完成的当前任务。
- [`hardware/`](hardware/)：板卡硬件和接口资料。
- [`spikes/`](spikes/)：阶段性验证/探索记录。
- [`../archive/legacy-jarvis/`](../archive/legacy-jarvis/)：不进入当前产品构建的旧 App、工具和网站原型源码。

上游 soRound_os 历史 README 保留在 [`../Jarvis_OS/`](../Jarvis_OS/)；当前 Jarvis_OS 使用和维护说明以根目录与本索引为准。
