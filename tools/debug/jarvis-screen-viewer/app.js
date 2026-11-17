const WIDTH = 466;
const HEIGHT = 466;
const MAX_PATCH_BYTES = WIDTH * 40 * 2;
const MAX_RLE_BYTES = MAX_PATCH_BYTES + Math.ceil((MAX_PATCH_BYTES / 2) / 128) + 1;

const canvas = document.querySelector("#screen");
const context = canvas.getContext("2d", { alpha: false });
const hint = document.querySelector("#screenHint");
const statusText = document.querySelector("#statusText");
const statusDot = document.querySelector("#statusDot");
const detailText = document.querySelector("#detailText");
const logOutput = document.querySelector("#logOutput");
const connectButton = document.querySelector("#connectButton");
const disconnectButton = document.querySelector("#disconnectButton");
const previewButton = document.querySelector("#previewButton");
const snapshotButton = document.querySelector("#snapshotButton");
const recordButton = document.querySelector("#recordButton");

const encoder = new TextEncoder();
const decoder = new TextDecoder();
const patchAssemblies = new Map();
let port = null;
let reader = null;
let writer = null;
let readTask = null;
let writeChain = Promise.resolve();
let lineBuffer = "";
let connected = false;
let previewing = false;
let hasFrame = false;
let saveAtNextFrame = false;
let keyFrameFrame = null;
const keyFrameCoverage = new Uint8Array(WIDTH * HEIGHT);
const frameRectangles = new Map();
const incompleteFrames = new Set();
let heartbeatTimer = null;
let frameTimeoutTimer = null;
let recoveryTimer = null;
let recorder = null;
let recordingParts = [];

function setStatus(message, state = "") {
  statusText.textContent = message;
  statusDot.className = `status-dot ${state}`;
}

function setDetail(message) {
  detailText.textContent = message;
}

function clearFrameTimeout() {
  clearTimeout(frameTimeoutTimer);
  frameTimeoutTimer = null;
}

function clearRecoveryTimer() {
  clearTimeout(recoveryTimer);
  recoveryTimer = null;
}

function scheduleRecovery() {
  if (!connected || !previewing || recoveryTimer) return;
  recoveryTimer = setTimeout(() => {
    recoveryTimer = null;
    if (!connected || !previewing || keyFrameFrame !== null) return;
    void sendCommand("SNAP").then(armFrameTimeout).catch(() => {});
  }, 700);
}

function armFrameTimeout() {
  clearFrameTimeout();
  frameTimeoutTimer = setTimeout(() => {
    frameTimeoutTimer = null;
    if (!connected || !previewing || (hasFrame && !saveAtNextFrame && keyFrameFrame === null)) return;
    const receivingKeyframe = keyFrameFrame !== null;
    setStatus(receivingKeyframe ? "整屏刷新超时" : "未收到设备画面", "error");
    setDetail(receivingKeyframe
      ? "完整画面传输超时。检查 USB 连接后可重新请求截图。"
      : "设备没有返回镜像数据。请确认已安装屏幕镜像调试固件，并在 Chrome 或 Edge 中从 localhost 打开页面。");
  }, 20000);
}

function appendLog(line) {
  const cleaned = line.replace(/[\x1b\x07]/g, "").trim();
  if (!cleaned) return;
  const rows = logOutput.textContent ? logOutput.textContent.split("\n") : [];
  rows.push(cleaned);
  logOutput.textContent = rows.slice(-100).join("\n");
  logOutput.scrollTop = logOutput.scrollHeight;
}

function updateButtons() {
  connectButton.disabled = connected;
  disconnectButton.disabled = !connected;
  previewButton.disabled = !connected;
  previewButton.textContent = previewing ? "停止预览" : "开始预览";
  snapshotButton.disabled = !connected;
  recordButton.disabled = !hasFrame || !connected;
  recordButton.classList.toggle("recording", Boolean(recorder));
  recordButton.querySelector("span:last-child").textContent = recorder ? "停止录制" : "开始录制";
}

async function sendCommand(command) {
  if (!writer) return;
  const operation = writeChain.catch(() => {}).then(() => {
    if (!writer) throw new Error("串口已断开");
    return writer.write(encoder.encode(`${command}\n`));
  });
  writeChain = operation.catch(() => {});
  return operation;
}

function paintPatch(bytes, frame, x, y, width, height) {
  if (x < 0 || y < 0 || width < 1 || height < 1 || x + width > WIDTH || y + height > HEIGHT) {
    throw new Error("设备发送了越界的屏幕区域");
  }
  if (bytes.length !== width * height * 2 || bytes.length > MAX_PATCH_BYTES) {
    throw new Error("屏幕区域长度不正确");
  }

  const image = context.createImageData(width, height);
  for (let source = 0, target = 0; source < bytes.length; source += 2, target += 4) {
    const pixel = (bytes[source] << 8) | bytes[source + 1];
    const red = (pixel >> 11) & 0x1f;
    const green = (pixel >> 5) & 0x3f;
    const blue = pixel & 0x1f;
    image.data[target] = (red * 255 + 15) / 31;
    image.data[target + 1] = (green * 255 + 31) / 63;
    image.data[target + 2] = (blue * 255 + 15) / 31;
    image.data[target + 3] = 255;
  }
  context.putImageData(image, x, y);
  if (keyFrameFrame === frame) {
    for (let row = 0; row < height; row++) {
      keyFrameCoverage.fill(1, (y + row) * WIDTH + x, (y + row) * WIDTH + x + width);
    }
  }
  hint.classList.add("hidden");
}

function crc32(bytes) {
  let crc = 0xffffffff;
  for (const byte of bytes) {
    crc ^= byte;
    for (let bit = 0; bit < 8; bit++) {
      crc = (crc >>> 1) ^ ((crc & 1) ? 0xedb88320 : 0);
    }
  }
  return (~crc) >>> 0;
}

function decodeRle(encoded, expectedLength) {
  const output = new Uint8Array(expectedLength);
  let source = 0;
  let target = 0;
  while (source < encoded.length) {
    const control = encoded[source++];
    if (control & 0x80) {
      const pixels = (control & 0x7f) + 3;
      if (source + 2 > encoded.length || target + pixels * 2 > output.length) {
        throw new Error("RLE 屏幕区域长度不正确");
      }
      const first = encoded[source++];
      const second = encoded[source++];
      for (let pixel = 0; pixel < pixels; pixel++) {
        output[target++] = first;
        output[target++] = second;
      }
    } else {
      const bytes = (control + 1) * 2;
      if (source + bytes > encoded.length || target + bytes > output.length) {
        throw new Error("RLE 屏幕区域长度不正确");
      }
      output.set(encoded.subarray(source, source + bytes), target);
      source += bytes;
      target += bytes;
    }
  }
  if (target !== expectedLength) throw new Error("RLE 屏幕区域未完整解码");
  return output;
}

function noteRectangle(frame, rectangle) {
  let rectangles = frameRectangles.get(frame);
  if (!rectangles) {
    rectangles = new Set();
    frameRectangles.set(frame, rectangles);
  }
  rectangles.add(rectangle);
  while (frameRectangles.size > 32) frameRectangles.delete(frameRectangles.keys().next().value);
}

function markIncomplete(frame) {
  incompleteFrames.add(frame);
  while (incompleteFrames.size > 32) incompleteFrames.delete(incompleteFrames.values().next().value);
}

function handlePatch(fields) {
  const legacy = fields.length === 12;
  if (!legacy && fields.length !== 14) return;
  const frame = Number(fields[2]);
  const rectangle = Number(fields[3]);
  const x = Number(fields[4]);
  const y = Number(fields[5]);
  const width = Number(fields[6]);
  const height = Number(fields[7]);
  const offset = Number(fields[8]);
  const encodedTotal = Number(fields[9]);
  const total = legacy ? encodedTotal : Number(fields[10]);
  const parsedCrc = Number.parseInt(fields[legacy ? 10 : 11], 16);
  const encoding = legacy ? "N" : fields[12];
  const payload = fields[legacy ? 11 : 13];
  if (![frame, rectangle, x, y, width, height, offset, encodedTotal, total, parsedCrc].every(Number.isFinite)) return;
  const expectedCrc = parsedCrc >>> 0;
  if (width < 1 || width > WIDTH || height < 1 || height > 40 || total !== width * height * 2 ||
      total > MAX_PATCH_BYTES || encodedTotal < 1 || encodedTotal > MAX_RLE_BYTES ||
      (encoding !== "N" && encoding !== "R") || (encoding === "N" && encodedTotal !== total) ||
      offset < 0 || offset >= encodedTotal) return;

  let decoded;
  try {
    const binary = atob(payload);
    decoded = Uint8Array.from(binary, character => character.charCodeAt(0));
  } catch {
    return;
  }
  if (offset + decoded.length > encodedTotal) return;

  const key = `${frame}:${rectangle}:${x}:${y}:${width}:${height}:${encodedTotal}:${total}:${encoding}`;
  let assembly = patchAssemblies.get(key);
  if (!assembly) {
    if (offset !== 0) {
      markIncomplete(frame);
      return;
    }
    assembly = {
      bytes: new Uint8Array(encodedTotal), received: 0, nextOffset: 0,
      frame, rectangle, expectedCrc, x, y, width, height, total, encoding,
    };
    patchAssemblies.set(key, assembly);
    while (patchAssemblies.size > 32) {
      const oldestKey = patchAssemblies.keys().next().value;
      const oldest = patchAssemblies.get(oldestKey);
      if (oldest) markIncomplete(oldest.frame);
      patchAssemblies.delete(oldestKey);
    }
  }
  if (offset !== assembly.nextOffset || rectangle !== assembly.rectangle || expectedCrc !== assembly.expectedCrc) {
    patchAssemblies.delete(key);
    markIncomplete(frame);
    return;
  }
  assembly.bytes.set(decoded, offset);
  assembly.received += decoded.length;
  assembly.nextOffset += decoded.length;
  if (assembly.received === encodedTotal) {
    patchAssemblies.delete(key);
    try {
      const pixels = assembly.encoding === "R"
        ? decodeRle(assembly.bytes, assembly.total)
        : assembly.bytes;
      if (crc32(pixels) !== assembly.expectedCrc) throw new Error("CRC 校验失败");
      paintPatch(pixels, frame, assembly.x, assembly.y, assembly.width, assembly.height);
      noteRectangle(frame, rectangle);
    } catch (error) {
      markIncomplete(frame);
      setStatus("画面数据错误", "error");
      setDetail(error.message);
      scheduleRecovery();
    }
  }
}

async function savePng() {
  const blob = await new Promise(resolve => canvas.toBlob(resolve, "image/png"));
  if (!blob) {
    setDetail("浏览器无法生成 PNG。");
    return;
  }
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");
  link.href = url;
  link.download = `jarvis-os-${new Date().toISOString().replaceAll(":", "-")}.png`;
  link.click();
  URL.revokeObjectURL(url);
}

function handleMirrorLine(line) {
  const marker = line.indexOf("MIRROR|");
  if (marker < 0) return false;
  const fields = line.slice(marker).trim().split("|");
  if (fields[1] === "P") {
    handlePatch(fields);
  } else if (fields[1] === "K") {
    clearRecoveryTimer();
    const frame = Number(fields[2]);
    if (!Number.isFinite(frame)) return true;
    keyFrameFrame = frame;
    keyFrameCoverage.fill(0);
    frameRectangles.delete(frame);
    incompleteFrames.delete(frame);
    for (const [key, assembly] of patchAssemblies) {
      if (assembly.frame !== frame) patchAssemblies.delete(key);
    }
    hasFrame = false;
    setStatus("同步完整画面", "");
    setDetail("正在接收整屏刷新。");
    armFrameTimeout();
    updateButtons();
  } else if (fields[1] === "E") {
    if (fields.length !== 5) return true;
    const frame = Number(fields[2]);
    const rectangleCount = Number(fields[3]);
    const dropped = fields[4] === "1";
    const isKeyFrame = keyFrameFrame === frame;
    if (isKeyFrame) clearFrameTimeout();
    const rectangles = frameRectangles.get(frame);
    const incomplete = dropped || !Number.isInteger(rectangleCount) || incompleteFrames.has(frame) ||
      !rectangles || rectangles.size !== rectangleCount;
    if (incomplete) {
      setStatus("画面有丢帧", "error");
      setDetail("正在重新请求完整画面。");
      scheduleRecovery();
      if (keyFrameFrame === frame) keyFrameFrame = null;
    } else {
      if (isKeyFrame) {
        const complete = keyFrameCoverage.every(value => value === 1);
        keyFrameFrame = null;
        if (!complete) {
          setStatus("完整画面未收齐", "error");
          setDetail("正在重新请求完整画面。");
          scheduleRecovery();
        } else {
          hasFrame = true;
          setStatus(previewing ? "实时预览中" : "已收到画面", "ready");
          setDetail(`已同步完整画面（第 ${frame} 帧）。`);
          if (saveAtNextFrame) {
            saveAtNextFrame = false;
            void savePng();
          }
        }
      } else if (hasFrame) {
        setStatus(previewing ? "实时预览中" : "画面已更新", "ready");
        setDetail(`已收到第 ${frame} 帧更新。`);
      } else {
        setStatus("完整画面标记缺失", "error");
        setDetail("正在重新请求完整画面。");
        scheduleRecovery();
      }
    }
    frameRectangles.delete(frame);
    incompleteFrames.delete(frame);
    patchAssemblies.clear();
    updateButtons();
  } else if (fields[1] === "X") {
    setStatus("正在恢复画面", "");
    setDetail("正在清除积压的旧画面并同步当前页面。");
    scheduleRecovery();
  } else if (fields[1] === "S") {
    if (fields[2] === "STARTING") {
      setStatus("设备已响应", "ready");
      setDetail("设备正在刷新整屏并传输画面。");
    } else if (fields[2] === "CLIENT_TIMEOUT") {
      previewing = false;
      clearInterval(heartbeatTimer);
      heartbeatTimer = null;
      clearFrameTimeout();
      setStatus("预览已超时", "");
      updateButtons();
    } else if (fields[2] === "LOCK_TIMEOUT") {
      clearFrameTimeout();
      setStatus("屏幕刷新超时", "error");
      setDetail("设备暂时无法锁定显示刷新，请稍后重新请求截图。");
    }
  }
  return true;
}

function handleSerialText(text) {
  lineBuffer += text;
  if (lineBuffer.length > 16384) lineBuffer = lineBuffer.slice(-8192);
  let newline;
  while ((newline = lineBuffer.indexOf("\n")) >= 0) {
    const line = lineBuffer.slice(0, newline).replace(/\r$/, "");
    lineBuffer = lineBuffer.slice(newline + 1);
    if (!handleMirrorLine(line)) appendLog(line);
  }
}

async function readSerial() {
  try {
    while (connected && reader) {
      const { value, done } = await reader.read();
      if (done) break;
      if (value) handleSerialText(decoder.decode(value, { stream: true }));
    }
  } catch (error) {
    if (connected) setDetail(`串口读取中断：${error.message}`);
  }
}

async function connect() {
  if (!("serial" in navigator)) {
    setStatus("浏览器不支持 Web Serial", "error");
    setDetail("请使用最新版 Chrome 或 Edge，并从 localhost 打开此页面。");
    return;
  }
  try {
    port = await navigator.serial.requestPort();
    await port.open({ baudRate: 115200, bufferSize: 4096 });
    reader = port.readable.getReader();
    writer = port.writable.getWriter();
    connected = true;
    lineBuffer = "";
    previewing = false;
    hasFrame = false;
    saveAtNextFrame = false;
    keyFrameFrame = null;
    keyFrameCoverage.fill(0);
    patchAssemblies.clear();
    frameRectangles.clear();
    incompleteFrames.clear();
    context.fillStyle = "#000";
    context.fillRect(0, 0, WIDTH, HEIGHT);
    hint.classList.remove("hidden");
    setStatus("USB 串口已连接", "ready");
    setDetail("开始预览后设备会完整刷新一次屏幕。");
    updateButtons();
    readTask = readSerial();
  } catch (error) {
    setStatus("连接失败", "error");
    setDetail(error.message || "无法打开所选串口。");
  }
}

async function startPreview(snapshotAfterStart = false) {
  if (!connected) return;
  clearFrameTimeout();
  clearRecoveryTimer();
  previewing = true;
  hasFrame = false;
  saveAtNextFrame = snapshotAfterStart;
  keyFrameFrame = null;
  keyFrameCoverage.fill(0);
  frameRectangles.clear();
  incompleteFrames.clear();
  patchAssemblies.clear();
  context.fillStyle = "#000";
  context.fillRect(0, 0, WIDTH, HEIGHT);
  hint.classList.remove("hidden");
  setStatus("等待设备画面", "");
  setDetail("正在请求完整刷新。USB 传输速度会影响首帧时间。");
  updateButtons();
  try {
    await sendCommand("START");
    clearInterval(heartbeatTimer);
    heartbeatTimer = setInterval(() => { void sendCommand("PING"); }, 1500);
    armFrameTimeout();
  } catch (error) {
    clearFrameTimeout();
    previewing = false;
    saveAtNextFrame = false;
    setStatus("无法启动预览", "error");
    setDetail(error.message);
    updateButtons();
    return false;
  }
  return true;
}

async function stopPreview() {
  clearInterval(heartbeatTimer);
  heartbeatTimer = null;
  clearFrameTimeout();
  clearRecoveryTimer();
  previewing = false;
  try { await sendCommand("STOP"); } catch { /* The user may be disconnecting the port. */ }
  setStatus("预览已停止", "");
  setDetail("设备仍保持连接，可重新开始预览。");
  updateButtons();
}

async function takeSnapshot() {
  if (!connected) return;
  if (!previewing) {
    const started = await startPreview(true);
    if (started) setDetail("正在请求完整刷新，收到后会自动下载 PNG。");
    return;
  }
  saveAtNextFrame = true;
  setDetail("正在请求完整刷新，收到后会自动下载 PNG。");
  try {
    await sendCommand("SNAP");
    armFrameTimeout();
  }
  catch (error) {
    clearFrameTimeout();
    saveAtNextFrame = false;
    setDetail(`截图请求失败：${error.message}`);
  }
}

function toggleRecording() {
  if (recorder) {
    recorder.stop();
    return;
  }
  if (!canvas.captureStream || !window.MediaRecorder) {
    setDetail("当前浏览器不支持 Canvas 录制。");
    return;
  }
  const stream = canvas.captureStream(15);
  const mimeType = ["video/webm;codecs=vp9", "video/webm;codecs=vp8", "video/webm"]
    .find(type => MediaRecorder.isTypeSupported(type));
  if (!mimeType) {
    setDetail("当前浏览器不支持 WebM 录制。");
    stream.getTracks().forEach(track => track.stop());
    return;
  }
  recordingParts = [];
  recorder = new MediaRecorder(stream, { mimeType });
  recorder.ondataavailable = event => { if (event.data.size) recordingParts.push(event.data); };
  recorder.onstop = () => {
    const blob = new Blob(recordingParts, { type: mimeType });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.href = url;
    link.download = `jarvis-os-${new Date().toISOString().replaceAll(":", "-")}.webm`;
    link.click();
    URL.revokeObjectURL(url);
    stream.getTracks().forEach(track => track.stop());
    recorder = null;
    recordingParts = [];
    updateButtons();
    setDetail("WebM 录制已保存。");
  };
  recorder.start(500);
  setDetail("正在录制浏览器预览画面；停止后下载 WebM。");
  updateButtons();
}

async function disconnect() {
  if (!port) return;
  clearInterval(heartbeatTimer);
  heartbeatTimer = null;
  clearFrameTimeout();
  clearRecoveryTimer();
  try { if (previewing) await sendCommand("STOP"); } catch { /* Port may already be gone. */ }
  if (recorder && recorder.state !== "inactive") recorder.stop();
  previewing = false;
  connected = false;
  try { await reader?.cancel(); } catch { /* Ignore close races. */ }
  try { reader?.releaseLock(); } catch { /* Ignore close races. */ }
  try { writer?.releaseLock(); } catch { /* Ignore close races. */ }
  try { await port.close(); } catch { /* The browser may have closed it already. */ }
  port = null;
  reader = null;
  writer = null;
  patchAssemblies.clear();
  frameRectangles.clear();
  incompleteFrames.clear();
  keyFrameFrame = null;
  keyFrameCoverage.fill(0);
  hasFrame = false;
  setStatus("未连接", "");
  setDetail("等待连接电脑与设备。");
  updateButtons();
  void readTask;
}

connectButton.addEventListener("click", () => { void connect(); });
disconnectButton.addEventListener("click", () => { void disconnect(); });
previewButton.addEventListener("click", () => { void (previewing ? stopPreview() : startPreview()); });
snapshotButton.addEventListener("click", () => { void takeSnapshot(); });
recordButton.addEventListener("click", toggleRecording);
document.querySelector("#clearLogButton").addEventListener("click", () => { logOutput.textContent = ""; });

window.addEventListener("pagehide", () => {
  if (previewing) void sendCommand("STOP");
  clearInterval(heartbeatTimer);
  clearFrameTimeout();
  clearRecoveryTimer();
});

if ("serial" in navigator) {
  navigator.serial.addEventListener("disconnect", event => {
    if (port && event.port === port) void disconnect();
  });
}

context.fillStyle = "#000";
context.fillRect(0, 0, WIDTH, HEIGHT);
updateButtons();
