import type { ProtocolMessage } from '../../../../protocol/message_types';

const textEncoder = new TextEncoder();
const textDecoder = new TextDecoder();

export function encodeFrame(message: ProtocolMessage): Uint8Array {
  const body = textEncoder.encode(JSON.stringify(message));
  const frame = new Uint8Array(4 + body.length);
  frame[0] = (body.length >>> 24) & 0xff;
  frame[1] = (body.length >>> 16) & 0xff;
  frame[2] = (body.length >>> 8) & 0xff;
  frame[3] = body.length & 0xff;
  frame.set(body, 4);
  return frame;
}

export function decodeFrame(frame: Uint8Array): ProtocolMessage {
  if (frame.length < 4) {
    throw new Error('frame too short');
  }
  const length = (frame[0] << 24) | (frame[1] << 16) | (frame[2] << 8) | frame[3];
  if (length !== frame.length - 4) {
    throw new Error(`frame length mismatch: expected ${length}, got ${frame.length - 4}`);
  }
  return JSON.parse(textDecoder.decode(frame.slice(4))) as ProtocolMessage;
}

export function encodeFrameHexPreview(message: ProtocolMessage): string {
  return Array.from(encodeFrame(message))
    .slice(0, 24)
    .map((byte) => byte.toString(16).padStart(2, '0'))
    .join(' ');
}

