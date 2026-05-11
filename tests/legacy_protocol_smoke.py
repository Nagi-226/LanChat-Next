import json
import struct


def encode_frame(payload: dict) -> bytes:
    body = json.dumps(payload, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
    return struct.pack(">I", len(body)) + body


def decode_frames(data: bytes):
    frames = []
    cursor = 0
    while cursor + 4 <= len(data):
        size = struct.unpack(">I", data[cursor:cursor + 4])[0]
        cursor += 4
        body = data[cursor:cursor + size]
        if len(body) != size:
            raise ValueError("truncated frame")
        frames.append(json.loads(body.decode("utf-8")))
        cursor += size
    if cursor != len(data):
        raise ValueError("trailing bytes")
    return frames


def test_frame_roundtrip():
    payloads = [
        {"type": 2, "id": 100000, "password": "admin123"},
        {"type": 5, "fromId": 100001, "toId": 100002, "msg": "中文 hello"},
        {"type": 16, "id": 100001, "headId": 0, "nickname": "Alice", "groupId": 200000, "msg": "hi"},
    ]
    wire = b"".join(encode_frame(item) for item in payloads)
    assert decode_frames(wire) == payloads


if __name__ == "__main__":
    test_frame_roundtrip()
    print("legacy protocol smoke ok")
