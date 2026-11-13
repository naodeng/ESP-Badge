#pragma once

#include "codex_data.h"

#include <stdbool.h>

void codex_client_set_visible(bool visible);
void codex_client_request_refresh(void);
void codex_client_snapshot(codex_snapshot_t *out);
