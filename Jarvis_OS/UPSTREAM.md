# Upstream provenance

Jarvis_OS integrates the soRound_os source tree from [soBigRice/soRound_os](https://github.com/soBigRice/soRound_os). The imported snapshot was based on upstream revision `1d388b5`.

The imported repository snapshot did not contain a LICENSE file. The project owner confirmed that permission was obtained from the author to reuse and modify the full source. This permission is the basis for this integration; it does not establish a public open-source license. Preserve this note and any attribution or conditions provided by the author when redistributing.

The imported documentation is retained in [`UPSTREAM_README.md`](UPSTREAM_README.md) and [`UPSTREAM_README.en.md`](UPSTREAM_README.en.md). Those files contain historical release notes and are kept as source documentation; current Jarvis build, flash, partition, and feature status are maintained in the repository [README](../README.md).

The LVGL9 touch adapter drops a failed CST9217 sample (with one retry for an invalid ACK) instead of aborting the firmware. The reproducible patch is [`GeekTool-IDF/tools/apply_lvgl_touch_patch.py`](GeekTool-IDF/tools/apply_lvgl_touch_patch.py). Run it after ESP-IDF resolves managed components and before building; it checks the upstream component revision and fails without changing files if that revision is different.
