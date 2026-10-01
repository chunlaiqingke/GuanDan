// 业务帧格式（小端）：[2B cmd][2B ver][4B len][protobuf body]
// 与服务端 src/net/Frame.cpp 保持一致。每个 WS 二进制帧 = 一条完整业务消息。

export const FRAME_VERSION = 1;
export const FRAME_HEADER_SIZE = 8; // 2 + 2 + 4

export interface FrameHeader {
  cmd: number;
  ver: number;
  len: number;
}

/** 组装完整业务帧：cmd + body -> 帧字节。 */
export function encodeFrame(cmd: number, body: Uint8Array): Uint8Array {
  const out = new Uint8Array(FRAME_HEADER_SIZE + body.byteLength);
  const dv = new DataView(out.buffer, out.byteOffset, out.byteLength);
  dv.setUint16(0, cmd, true); // 小端
  dv.setUint16(2, FRAME_VERSION, true);
  dv.setUint32(4, body.byteLength, true);
  out.set(body, FRAME_HEADER_SIZE);
  return out;
}

/** 解析帧头；数据不足返回 null。 */
export function decodeHeader(data: Uint8Array): FrameHeader | null {
  if (data.byteLength < FRAME_HEADER_SIZE) return null;
  const dv = new DataView(data.buffer, data.byteOffset, data.byteLength);
  return {
    cmd: dv.getUint16(0, true),
    ver: dv.getUint16(2, true),
    len: dv.getUint32(4, true),
  };
}
